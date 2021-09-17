#ifndef QUICHE_QUIC_CORE_QUIC_SENT_PACKET_MANAGER_H_
#define QUICHE_QUIC_CORE_QUIC_SENT_PACKET_MANAGER_H_

#include <memory>
#include "src/congestion_control/send_algorithm_interface.h"
#include "src/congestion_control/loss_detection_interface.h"
#include "src/quic/quic_unacked_packet_map.h"


class QuicSentPacketManager {
enum RetransmissionTimeoutMode {
  RTO_MODE,
  TLP_MODE,
  HANDSHAKE_MODE,
  LOSS_MODE,
  PTO_MODE,
};

public:
  QuicSentPacketManager(Perspective perspective,
                        const QuicClock* clock,
                        QuicConnectionStats* stats,
                        CongestionControlType congestion_control_type);
  QuicSentPacketManager(const QuicSentPacketManager&) = delete;
  QuicSentPacketManager& operator=(const QuicSentPacketManager&) = delete;
  ~QuicSentPacketManager();
  // Called when we have sent bytes to the peer.  This informs the manager both
  // the number of bytes sent and if they were retransmitted and if this packet
  // is used for rtt measuring.  Returns true if the sender should reset the
  // retransmission timer.
  bool OnPacketSent(SerializedPacket* mutable_packet,
                    QuicTime sent_time);
  void OnAckFrameStart(QuicPacketNumber largest_acked,
                       QuicTime ack_receive_time);
  void OnAckRange(QuicPacketNumber start, QuicPacketNumber end);

  AckResult OnAckFrameEnd(QuicTime ack_receive_time,
                        QuicPacketNumber ack_packet_number);

  // Called after packets have been marked handled with last received ack frame.
  void PostProcessNewlyAckedPackets(QuicPacketNumber ack_packet_number,
                                    QuicTime ack_receive_time,
                                    bool rtt_updated,
                                    QuicByteCount prior_bytes_in_flight);

  RetransmissionTimeoutMode OnRetransmissionTimeout();
  const QuicTime GetRetransmissionTime() const;
  const QuicTime::Delta GetRetransmissionDelay() const;

  void SetSendAlgorithm(CongestionControlType congestion_control_type);
  void SetSendAlgorithm(SendAlgorithmInterface* send_algorithm);

  const QuicUnackedPacketMap& unacked_packets() const {
    return unacked_packets_;
  }
  const LostPacketVector& packets_lost() const {
    return packets_lost_;
  }
  QuicByteCount GetCongestionWindowInBytes() const {
    return send_algorithm_->GetCongestionWindow();
  }

  QuicBandwidth GetPacingRate() const {
    return send_algorithm_->PacingRate(GetBytesInFlight());
  }
  QuicByteCount GetBytesInFlight() const {
    return unacked_packets_.bytes_in_flight();
  }
private:
  RetransmissionTimeoutMode GetRetransmissionMode() const;
  void RetransmitRtoPackets();

  void InvokeLossDetection(QuicTime time);
  // Invokes OnCongestionEvent if |rtt_updated| is true, there are pending acks,
  // or pending losses.  Clears pending acks and pending losses afterwards.
  // |prior_in_flight| is the number of bytes in flight before the losses or
  // acks, |event_time| is normally the timestamp of the ack packet which caused
  // the event, although it can be the time at which loss detection was
  // triggered.
  void MaybeInvokeCongestionEvent(bool rtt_updated,
                                  QuicByteCount prior_in_flight,
                                  QuicTime event_time);
  // Removes the retransmittability and in flight properties from the packet at
  // |info| due to receipt by the peer.
  void MarkPacketHandled(QuicPacketNumber packet_number,
                         QuicTransmissionInfo* info,
                         QuicTime ack_receive_time,
                         QuicTime receive_timestamp);
  bool MaybeUpdateRTT(QuicPacketNumber largest_acked,
                    QuicTime ack_receive_time);


  std::unique_ptr<SendAlgorithmInterface> send_algorithm_;
  LossDetectionInterface* loss_algorithm_;

  QuicUnackedPacketMap unacked_packets_;
  LostPacketVector packets_lost_;
  // Vectors packets acked and lost as a result of the last congestion event.
  AckedPacketVector packets_acked_;
  // Largest newly acknowledged packet.
  QuicPacketNumber largest_newly_acked_;
  QuicTime::Delta min_rto_timeout_;

  const QuicClock* clock_;
  QuicConnectionStats* stats_;
  size_t consecutive_rto_count_;

  QuicPacketCount initial_congestion_window_;
  RttStats rtt_stats_;
  bool rtt_updated_;
};



#endif

