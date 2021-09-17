// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#ifndef QUIC_UNACKED_PACKET_MAP_H
#define QUIC_UNACKED_PACKET_MAP_H 1

#include "src/quic/quic_unacked_packet_map.h"

#include <cstddef>
#include <limits>
#include <type_traits>
#include <iostream>

#include "src/quic/quic_connection_stats.h"
#include "src/quic/quic_packet_number.h"
#include "src/quic/quic_types.h"
#include "src/quic/quic_constants.h"

bool WillStreamFrameLengthSumWrapAround(QuicPacketLength lhs,
                                        QuicPacketLength rhs) {
  static_assert(
      std::is_unsigned<QuicPacketLength>::value,
      "This function assumes QuicPacketLength is an unsigned integer type.");
  return std::numeric_limits<QuicPacketLength>::max() - lhs < rhs;
}

enum QuicFrameTypeBitfield : uint32_t {
  kInvalidFrameBitfield = 0,
  kPaddingFrameBitfield = 1,
  kRstStreamFrameBitfield = 1 << 1,
  kConnectionCloseFrameBitfield = 1 << 2,
  kGoawayFrameBitfield = 1 << 3,
  kWindowUpdateFrameBitfield = 1 << 4,
  kBlockedFrameBitfield = 1 << 5,
  kStopWaitingFrameBitfield = 1 << 6,
  kPingFrameBitfield = 1 << 7,
  kCryptoFrameBitfield = 1 << 8,
  kHandshakeDoneFrameBitfield = 1 << 9,
  kStreamFrameBitfield = 1 << 10,
  kAckFrameBitfield = 1 << 11,
  kMtuDiscoveryFrameBitfield = 1 << 12,
  kNewConnectionIdFrameBitfield = 1 << 13,
  kMaxStreamsFrameBitfield = 1 << 14,
  kStreamsBlockedFrameBitfield = 1 << 15,
  kPathResponseFrameBitfield = 1 << 16,
  kPathChallengeFrameBitfield = 1 << 17,
  kStopSendingFrameBitfield = 1 << 18,
  kMessageFrameBitfield = 1 << 19,
  kNewTokenFrameBitfield = 1 << 20,
  kRetireConnectionIdFrameBitfield = 1 << 21,
  kAckFrequencyFrameBitfield = 1 << 22,
};

QuicFrameTypeBitfield GetFrameTypeBitfield(QuicFrameType type) {
  switch (type) {
    case PADDING_FRAME:
      return kPaddingFrameBitfield;
    case RST_STREAM_FRAME:
      return kRstStreamFrameBitfield;
    case CONNECTION_CLOSE_FRAME:
      return kConnectionCloseFrameBitfield;
    case GOAWAY_FRAME:
      return kGoawayFrameBitfield;
    case WINDOW_UPDATE_FRAME:
      return kWindowUpdateFrameBitfield;
    case BLOCKED_FRAME:
      return kBlockedFrameBitfield;
    case STOP_WAITING_FRAME:
      return kStopWaitingFrameBitfield;
    case PING_FRAME:
      return kPingFrameBitfield;
    case CRYPTO_FRAME:
      return kCryptoFrameBitfield;
    case HANDSHAKE_DONE_FRAME:
      return kHandshakeDoneFrameBitfield;
    case STREAM_FRAME:
      return kStreamFrameBitfield;
    case ACK_FRAME:
      return kAckFrameBitfield;
    case MTU_DISCOVERY_FRAME:
      return kMtuDiscoveryFrameBitfield;
    case NEW_CONNECTION_ID_FRAME:
      return kNewConnectionIdFrameBitfield;
    case MAX_STREAMS_FRAME:
      return kMaxStreamsFrameBitfield;
    case STREAMS_BLOCKED_FRAME:
      return kStreamsBlockedFrameBitfield;
    case PATH_RESPONSE_FRAME:
      return kPathResponseFrameBitfield;
    case PATH_CHALLENGE_FRAME:
      return kPathChallengeFrameBitfield;
    case STOP_SENDING_FRAME:
      return kStopSendingFrameBitfield;
    case MESSAGE_FRAME:
      return kMessageFrameBitfield;
    case NEW_TOKEN_FRAME:
      return kNewTokenFrameBitfield;
    case RETIRE_CONNECTION_ID_FRAME:
      return kRetireConnectionIdFrameBitfield;
    case ACK_FREQUENCY_FRAME:
      return kAckFrequencyFrameBitfield;
    case NUM_FRAME_TYPES:
      return kInvalidFrameBitfield;
  }
  return kInvalidFrameBitfield;
}


QuicUnackedPacketMap::QuicUnackedPacketMap(Perspective perspective)
    : perspective_(perspective),
      least_unacked_(FirstSendingPacketNumber()),
      bytes_in_flight_(0),
      bytes_in_flight_per_packet_number_space_{0, 0, 0},
      packets_in_flight_(0),
      last_inflight_packet_sent_time_(QuicTime::Zero()),
      last_inflight_packets_sent_time_{{QuicTime::Zero()},
                                       {QuicTime::Zero()},
                                       {QuicTime::Zero()}},
      last_crypto_packet_sent_time_(QuicTime::Zero()),
      supports_multiple_packet_number_spaces_(false) {
}

QuicUnackedPacketMap::~QuicUnackedPacketMap(){};

void QuicUnackedPacketMap::AddSentPacket(SerializedPacket* mutable_packet,
                                         QuicTime sent_time,
                                         bool set_in_flight) {
  const SerializedPacket& packet = *mutable_packet;
  QuicPacketNumber packet_number = packet.packet_number;
  QuicPacketLength bytes_sent = packet.encrypted_length;
  while (least_unacked_ + unacked_packets_.size() < packet_number) {
    unacked_packets_.push_back(QuicTransmissionInfo());
    unacked_packets_.back().state = NEVER_SENT;
  }

  QuicTransmissionInfo info(packet_number,
                            sent_time, bytes_sent);
  // info.largest_acked = packet.largest_acked;
  // largest_sent_largest_acked_.UpdateMax(packet.largest_acked);
  bool measure_rtt = true;// dedect from our case
  if (!measure_rtt) {
    info.state = NOT_CONTRIBUTING_RTT;
  }
  largest_sent_packet_ = packet_number;
  if (set_in_flight) {

    bytes_in_flight_ += bytes_sent;
    ++packets_in_flight_;
    info.in_flight = true;
    last_inflight_packet_sent_time_ = sent_time;
  }
  unacked_packets_.push_back(info);

}

void QuicUnackedPacketMap::IncreaseLargestAcked(
    QuicPacketNumber largest_acked) {
  largest_acked_ = largest_acked;
}

void QuicUnackedPacketMap::MaybeUpdateLargestAckedOfPacketNumberSpace(
    PacketNumberSpace packet_number_space,
    QuicPacketNumber packet_number) {
  largest_acked_packets_[packet_number_space].UpdateMax(packet_number);
}


bool QuicUnackedPacketMap::IsPacketUsefulForCongestionControl(
    const QuicTransmissionInfo& info) const {
  // Packet contributes to congestion control if it is considered inflight.
  return info.in_flight;
}

bool QuicUnackedPacketMap::IsUnacked(QuicPacketNumber packet_number) const {
  if (packet_number < least_unacked_ ||
      packet_number >= least_unacked_ + unacked_packets_.size()) {
    return false;
  }
  return !IsPacketUseless(packet_number,
                          unacked_packets_[packet_number - least_unacked_]);
}

void QuicUnackedPacketMap::RemoveFromInFlight(QuicTransmissionInfo* info) {
  if (info->in_flight) {
    bytes_in_flight_ -= info->bytes_sent;
    --packets_in_flight_;

    info->in_flight = false;
  }
  // std::cout<<unacked_packets_.size() <<std::endl;
}

void QuicUnackedPacketMap::RemoveFromInFlight(QuicPacketNumber packet_number) {
  QuicTransmissionInfo* info =
      &unacked_packets_[packet_number - least_unacked_];
  RemoveFromInFlight(info);
}

bool QuicUnackedPacketMap::HasInFlightPackets() const {
  return bytes_in_flight_ > 0;
}

const QuicTransmissionInfo& QuicUnackedPacketMap::GetTransmissionInfo(
    QuicPacketNumber packet_number) const {
  return unacked_packets_[packet_number - least_unacked_];
}

QuicTransmissionInfo* QuicUnackedPacketMap::GetMutableTransmissionInfo(
    QuicPacketNumber packet_number) {
  return &unacked_packets_[packet_number - least_unacked_];
}

QuicTime QuicUnackedPacketMap::GetLastInFlightPacketSentTime() const {
  return last_inflight_packet_sent_time_;
}

QuicTime QuicUnackedPacketMap::GetLastCryptoPacketSentTime() const {
  return last_crypto_packet_sent_time_;
}

size_t QuicUnackedPacketMap::GetNumUnackedPacketsDebugOnly() const {
  size_t unacked_packet_count = 0;
  QuicPacketNumber packet_number = least_unacked_;
  for (auto it = begin(); it != end(); ++it, ++packet_number) {
    if (!IsPacketUseless(packet_number, *it)) {
      ++unacked_packet_count;
    }
  }
  return unacked_packet_count;
}

bool QuicUnackedPacketMap::HasMultipleInFlightPackets() const {
  if (bytes_in_flight_ > kDefaultTCPMSS) {
    return true;
  }
  size_t num_in_flight = 0;
  for (auto it = rbegin(); it != rend(); ++it) {
    if (it->in_flight) {
      ++num_in_flight;
    }
    if (num_in_flight > 1) {
      return true;
    }
  }
  return false;
}


// bool QuicUnackedPacketMap::HasUnackedRetransmittableFrames() const {
//   for (auto it = rbegin(); it != rend(); ++it) {
//     if (it->in_flight && HasRetransmittableFrames(*it)) {
//       return true;
//     }
//   }
//   return false;
// }

QuicPacketNumber QuicUnackedPacketMap::GetLeastUnacked() const {
  return least_unacked_;
}


QuicPacketNumber QuicUnackedPacketMap::GetLargestAckedOfPacketNumberSpace(
    PacketNumberSpace packet_number_space) const {
  if (packet_number_space >= NUM_PACKET_NUMBER_SPACES) {
    return QuicPacketNumber();
  }
  return largest_acked_packets_[packet_number_space];
}

QuicTime QuicUnackedPacketMap::GetLastInFlightPacketSentTime(
    PacketNumberSpace packet_number_space) const {
  if (packet_number_space >= NUM_PACKET_NUMBER_SPACES) {
    return QuicTime::Zero();
  }
  return last_inflight_packets_sent_time_[packet_number_space];
}

QuicPacketNumber
QuicUnackedPacketMap::GetLargestSentRetransmittableOfPacketNumberSpace(
    PacketNumberSpace packet_number_space) const {
  if (packet_number_space >= NUM_PACKET_NUMBER_SPACES) {
    return QuicPacketNumber();
  }
  return largest_sent_retransmittable_packets_[packet_number_space];
}

const QuicTransmissionInfo*
QuicUnackedPacketMap::GetFirstInFlightTransmissionInfo() const {
  for (auto it = begin(); it != end(); ++it) {
    if (it->in_flight) {
      return &(*it);
    }
  }
  return nullptr;
}

const QuicTransmissionInfo*
QuicUnackedPacketMap::GetFirstInFlightTransmissionInfoOfSpace(
    PacketNumberSpace packet_number_space) const {
  // TODO(fayang): Optimize this part if arm 1st PTO with first in flight sent
  // time works.
  for (auto it = begin(); it != end(); ++it) {
    if (it->in_flight ) {
      return &(*it);
    }
  }
  return nullptr;
}

void QuicUnackedPacketMap::EnableMultiplePacketNumberSpacesSupport() {
  if (supports_multiple_packet_number_spaces_) {
    return;
  }
  if (largest_sent_packet_.IsInitialized()) {
    return;
  }

  supports_multiple_packet_number_spaces_ = true;
}

// int32_t QuicUnackedPacketMap::GetLastPacketContent() const {
//   if (empty()) {
//     // Use -1 to distinguish with packets with no retransmittable frames nor
//     // acks.
//     return -1;
//   }
//   int32_t content = 0;
//   const QuicTransmissionInfo& last_packet = unacked_packets_.back();
//   if (last_packet.largest_acked.IsInitialized()) {
//     content |= GetFrameTypeBitfield(ACK_FRAME);
//   }
//   return content;
// }

void QuicUnackedPacketMap::RemoveObsoletePackets() {
  while (!unacked_packets_.empty()) {
    if (!IsPacketUseless(least_unacked_, unacked_packets_.front())) {
      break;
    }
    // DeleteFrames(&unacked_packets_.front().retransmittable_frames);
    unacked_packets_.pop_front();
    ++least_unacked_;
  }
}

static bool IsAckable(SentPacketState state) {
  return state != NEVER_SENT && state != ACKED && state != UNACKABLE;
}

bool QuicUnackedPacketMap::IsPacketUsefulForMeasuringRtt(
    QuicPacketNumber packet_number,
    const QuicTransmissionInfo& info) const {
  // Packet can be used for RTT measurement if it may yet be acked as the
  // largest observed packet by the receiver.
  // std::cout<< (!largest_acked_.IsInitialized() || packet_number > largest_acked_) <<
  //   " " <<(info.state != NOT_CONTRIBUTING_RTT )<< std::endl;
  // return IsAckable(info.state) &&
  //        (!largest_acked_.IsInitialized() || packet_number > largest_acked_) &&
  //        info.state != NOT_CONTRIBUTING_RTT;
  return IsAckable(info.state);
}


bool QuicUnackedPacketMap::IsPacketUseless(
    QuicPacketNumber packet_number,
    const QuicTransmissionInfo& info) const {
  return !IsPacketUsefulForMeasuringRtt(packet_number, info) &&
         !IsPacketUsefulForCongestionControl(info) ;
}

#endif