
#include "src/quic/quic_sent_packet_manager.h"

#include <iostream>

#include "src/congestion_control/general_loss_algorithm.h"
#include "src/congestion_control/send_algorithm_interface.h"
#include "src/packet/packet.h"
#include "src/quic/quic_constants.h"

static const int64_t kDefaultRetransmissionTimeMs = 500;
static const int64_t kMaxRetransmissionTimeMs = 60000;
// Maximum number of exponential backoffs used for RTO timeouts.
static const size_t kMaxRetransmissions = 10;
// Maximum number of packets retransmitted upon an RTO.
static const size_t kMaxRetransmissionsOnTimeout = 2;
// The path degrading delay is the sum of this number of consecutive RTO delays.
const size_t kNumRetransmissionDelaysForPathDegradingDelay = 2;

// Ensure the handshake timer isnt't faster than 10ms.
// This limits the tenth retransmitted packet to 10s after the initial CHLO.
static const int64_t kMinHandshakeTimeoutMs = 10;

// Sends up to two tail loss probes before firing an RTO,
// per draft RFC draft-dukkipati-tcpm-tcp-loss-probe.
static const size_t kDefaultMaxTailLossProbes = 2;

QuicSentPacketManager::QuicSentPacketManager(
    Perspective perspective,
    const QuicClock* clock,
    QuicConnectionStats* stats,
    CongestionControlType congestion_control_type)
    : unacked_packets_(perspective),
      clock_(clock),
      stats_(stats),
      initial_congestion_window_(kInitialCongestionWindow),
      packets_lost_(),
      min_rto_timeout_(QuicTime::Delta::FromMilliseconds(kMinRetransmissionTimeMs)),
      consecutive_rto_count_(0),
      loss_algorithm_(new GeneralLossAlgorithm()) {
        // packets_lost_.push_back(LostPacket(1,0));
  SetSendAlgorithm(congestion_control_type);
}

QuicSentPacketManager::~QuicSentPacketManager() {}

void QuicSentPacketManager::SetSendAlgorithm(
    CongestionControlType congestion_control_type) {
  if (send_algorithm_ &&
      send_algorithm_->GetCongestionControlType() == congestion_control_type) {
    return;
  }
  SetSendAlgorithm(SendAlgorithmInterface::Create(
      clock_, &rtt_stats_, &unacked_packets_, congestion_control_type, 
      stats_, initial_congestion_window_, send_algorithm_.get()));
}

void QuicSentPacketManager::SetSendAlgorithm(
    SendAlgorithmInterface* send_algorithm) {
  send_algorithm_.reset(send_algorithm);
}

bool QuicSentPacketManager::OnPacketSent(
    SerializedPacket* mutable_packet,
    QuicTime sent_time) {
  const SerializedPacket& packet = *mutable_packet;
  QuicPacketNumber packet_number = packet.packet_number;

  bool in_flight = true;

  send_algorithm_->OnPacketSent(sent_time, unacked_packets_.bytes_in_flight(),
                                packet_number, packet.encrypted_length);
                                
  unacked_packets_.AddSentPacket(mutable_packet, sent_time,in_flight);
  // Reset the retransmission timer anytime a pending packet is sent.
  return in_flight;
}

void QuicSentPacketManager::PostProcessNewlyAckedPackets(
    QuicPacketNumber ack_packet_number,
    QuicTime ack_receive_time,
    bool rtt_updated,
    QuicByteCount prior_bytes_in_flight) {
  InvokeLossDetection(ack_receive_time);

  MaybeInvokeCongestionEvent(rtt_updated, prior_bytes_in_flight,
                             ack_receive_time);

  if (rtt_updated) {
    if (consecutive_rto_count_ > 0) {
      // If the ack acknowledges data sent prior to the RTO,
      // the RTO was spurious.
      send_algorithm_->OnRetransmissionTimeout(true);
    }
    // Reset all retransmit counters any time a new packet is acked.
    consecutive_rto_count_ = 0;
  }
}

void QuicSentPacketManager::InvokeLossDetection(QuicTime time) {
  if (!packets_acked_.empty()) {
    largest_newly_acked_ = packets_acked_.back().packet_number;
  }

  loss_algorithm_->DetectLosses(unacked_packets_, time, rtt_stats_,
                                    largest_newly_acked_, packets_acked_,
                                    &packets_lost_);
  for (const LostPacket& packet : packets_lost_) {
    QuicTransmissionInfo* info =
        unacked_packets_.GetMutableTransmissionInfo(packet.packet_number);
    ++stats_->packets_lost;
    unacked_packets_.RemoveFromInFlight(info);
    info->state =  NEVER_SENT;// added by carr, so lost packets can be removed from front in test code 
  }
}

AckResult QuicSentPacketManager::OnAckFrameEnd(
    QuicTime ack_receive_time,
    QuicPacketNumber ack_packet_number) {
  QuicByteCount prior_bytes_in_flight = unacked_packets_.bytes_in_flight();
  // Reverse packets_acked_ so that it is in ascending order.
  std::reverse(packets_acked_.begin(), packets_acked_.end());
  for (AckedPacket& acked_packet : packets_acked_) {
    QuicTransmissionInfo* info =
        unacked_packets_.GetMutableTransmissionInfo(acked_packet.packet_number);

    // If data is associated with the most recent transmission of this
    // packet, then inform the caller.
    if (info->in_flight) {
      acked_packet.bytes_acked = info->bytes_sent;
    } else {
      // Unackable packets are skipped earlier.
      largest_newly_acked_ = acked_packet.packet_number;
    }
    MarkPacketHandled(acked_packet.packet_number, info, ack_receive_time,
                      acked_packet.receive_timestamp);
  }

  const bool acked_new_packet = !packets_acked_.empty();
  PostProcessNewlyAckedPackets(ack_packet_number, ack_receive_time,rtt_updated_,
                               prior_bytes_in_flight);

  return acked_new_packet ? PACKETS_NEWLY_ACKED : NO_PACKETS_NEWLY_ACKED;
}

void QuicSentPacketManager::MarkPacketHandled(QuicPacketNumber packet_number,
                                              QuicTransmissionInfo* info,
                                              QuicTime ack_receive_time,
                                              QuicTime receive_timestamp) {
  if (info->state == LOST) {
    // Record as a spurious loss as a packet previously declared lost gets
    // acked.
    const QuicPacketNumber previous_largest_acked = unacked_packets_.largest_acked();
    loss_algorithm_->SpuriousLossDetected(unacked_packets_, rtt_stats_,
                                          ack_receive_time, packet_number,
                                          previous_largest_acked);
    ++stats_->packet_spuriously_detected_lost;
  }
  unacked_packets_.RemoveFromInFlight(info);
  // unacked_packets_.RemoveRetransmittability(info);
  info->state = ACKED;
}

void QuicSentPacketManager::MaybeInvokeCongestionEvent(
    bool rtt_updated,
    QuicByteCount prior_in_flight,
    QuicTime event_time) {
  if (!rtt_updated && packets_acked_.empty() && packets_lost_.empty()) {
    return;
  }
  const bool overshooting_detected =
      stats_->overshooting_detected_with_network_parameters_adjusted;
  send_algorithm_->OnCongestionEvent(rtt_updated, prior_in_flight, event_time,
                                      packets_acked_, packets_lost_);
  unacked_packets_.RemoveObsoletePackets();// remove from deque
  packets_acked_.clear();
  packets_lost_.clear();
}

void QuicSentPacketManager::OnAckRange(QuicPacketNumber start,
                                       QuicPacketNumber end) {
  packets_acked_.push_back(AckedPacket(start, DEFAULT_PACKET_SIZE, clock_->Now()));

}


void QuicSentPacketManager::OnAckFrameStart(QuicPacketNumber largest_acked,
                                            QuicTime ack_receive_time) {
  rtt_updated_ =
      MaybeUpdateRTT(largest_acked, ack_receive_time);
}
bool QuicSentPacketManager::MaybeUpdateRTT(QuicPacketNumber largest_acked,
                                           QuicTime ack_receive_time) {
  // We rely on ack_delay_time to compute an RTT estimate, so we
  // only update rtt when the largest observed gets acked and the acked packet
  // is not useless.
  if (!unacked_packets_.IsUnacked(largest_acked)) {
    return false;
  }
  // We calculate the RTT based on the highest ACKed packet number, the lower
  // packet numbers will include the ACK aggregation delay.
  const QuicTransmissionInfo& transmission_info =
      unacked_packets_.GetTransmissionInfo(largest_acked);
  // Ensure the packet has a valid sent time.
  if (transmission_info.sent_time == QuicTime::Zero()) {
    return false;
  }
  if (transmission_info.state == NOT_CONTRIBUTING_RTT) {
    return false;
  }
  if (transmission_info.sent_time > ack_receive_time) {
  }

  QuicTime::Delta send_delta = ack_receive_time - transmission_info.sent_time;
  const bool min_rtt_available = !rtt_stats_.min_rtt().IsZero();

  QuicTime::Delta delay =  QuicTime::Delta::FromMicroseconds(0);
  rtt_stats_.UpdateRtt(send_delta, delay ,ack_receive_time);

  return true;
}

QuicSentPacketManager::RetransmissionTimeoutMode
QuicSentPacketManager::OnRetransmissionTimeout() {
  switch (GetRetransmissionMode()) {
    case LOSS_MODE: {
      ++stats_->loss_timeout_count;
      QuicByteCount prior_in_flight = unacked_packets_.bytes_in_flight();
      const QuicTime now = clock_->Now();
      InvokeLossDetection(now);
      MaybeInvokeCongestionEvent(false, prior_in_flight, now);
      return LOSS_MODE;
    }
    case RTO_MODE:
      ++stats_->rto_count;
      RetransmitRtoPackets();
      return RTO_MODE;
  }
  return GetRetransmissionMode();
}
void QuicSentPacketManager::RetransmitRtoPackets() {
  // Mark two packets for retransmission.
  std::vector<QuicPacketNumber> retransmissions;
  if (!unacked_packets_.empty()) {
    QuicPacketNumber packet_number = unacked_packets_.GetLeastUnacked();
    QuicPacketNumber largest_sent_packet =
        unacked_packets_.largest_sent_packet();
    for (; packet_number <= largest_sent_packet; ++packet_number) {
      QuicTransmissionInfo* transmission_info =
          unacked_packets_.GetMutableTransmissionInfo(packet_number);
      if (transmission_info->state == OUTSTANDING/* &&
          unacked_packets_.HasRetransmittableFrames(*transmission_info) &&
          pending_timer_transmission_count_ < max_rto_packets_*/) {
        unacked_packets_.RemoveFromInFlight(transmission_info);
        transmission_info->state = NEVER_SENT;
        // retransmissions.push_back(packet_number);
        // ++pending_timer_transmission_count_;
      }
    }
  }
  unacked_packets_.RemoveObsoletePackets();
  ++consecutive_rto_count_;
  // // for (QuicPacketNumber retransmission : retransmissions) {
  // //   MarkForRetransmission(retransmission, RTO_RETRANSMISSION);
  // // }
  // if (retransmissions.empty()) {
  //   // No packets to be RTO retransmitted, raise up a credit to allow
  //   // connection to send.
  //   pending_timer_transmission_count_ = 1;
  // }
}

QuicSentPacketManager::RetransmissionTimeoutMode
QuicSentPacketManager::GetRetransmissionMode() const {

  if (loss_algorithm_->GetLossTimeout() != QuicTime::Zero()) {
    return LOSS_MODE;
  }

  return RTO_MODE;
}


const QuicTime QuicSentPacketManager::GetRetransmissionTime() const {
  if (!unacked_packets_.HasInFlightPackets()) {
    return QuicTime::Zero();
  }
  switch (GetRetransmissionMode()) {

    case LOSS_MODE:
      return loss_algorithm_->GetLossTimeout();
    case RTO_MODE: {
      // The RTO is based on the first outstanding packet.
      const QuicTime sent_time =
          unacked_packets_.GetLastInFlightPacketSentTime();
      QuicTime rto_time = sent_time + GetRetransmissionDelay();
      // Wait for TLP packets to be acked before an RTO fires.
      // QuicTime tlp_time = sent_time + GetTailLossProbeDelay();
      // return std::max(tlp_time, rto_time);
      return rto_time;
    }

  }
  return QuicTime::Zero();
}

const QuicTime::Delta QuicSentPacketManager::GetRetransmissionDelay() const {
  QuicTime::Delta retransmission_delay = QuicTime::Delta::Zero();
  if (rtt_stats_.smoothed_rtt().IsZero()) {
    // We are in the initial state, use default timeout values.
    retransmission_delay =
        QuicTime::Delta::FromMilliseconds(kDefaultRetransmissionTimeMs);
  } else {
    retransmission_delay =
        rtt_stats_.smoothed_rtt() + 4 * rtt_stats_.mean_deviation();
    if (retransmission_delay < min_rto_timeout_) {
      retransmission_delay = min_rto_timeout_;
    }
  }

  // Calculate exponential back off.
  retransmission_delay =
      retransmission_delay *
      (1 << std::min<size_t>(consecutive_rto_count_, kMaxRetransmissions));

  if (retransmission_delay.ToMilliseconds() > kMaxRetransmissionTimeMs) {
    return QuicTime::Delta::FromMilliseconds(kMaxRetransmissionTimeMs);
  }
  return retransmission_delay;
}

