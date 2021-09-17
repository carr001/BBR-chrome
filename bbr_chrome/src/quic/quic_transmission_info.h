// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QUICHE_QUIC_CORE_QUIC_TRANSMISSION_INFO_H_
#define QUICHE_QUIC_CORE_QUIC_TRANSMISSION_INFO_H_

#include <list>

#include "src/quic/quic_types.h"


// Stores details of a single sent packet.
struct  QuicTransmissionInfo {
  // Used by STL when assigning into a map.
  QuicTransmissionInfo();

  // Constructs a Transmission with a new all_transmissions set
  // containing |packet_number|.
  QuicTransmissionInfo(QuicPacketNumber packet_number,
                       QuicTime sent_time,
                       QuicPacketLength bytes_sent);

  QuicTransmissionInfo(const QuicTransmissionInfo& other);

  ~QuicTransmissionInfo();

  // std::string DebugString() const;

  // QuicFrames retransmittable_frames;
  QuicPacketNumber packet_number;
  // SerializedPacket packet;

  QuicTime sent_time;
  QuicPacketLength bytes_sent;
  // In flight packets have not been abandoned or lost.
  bool in_flight;
  // State of this packet.
  SentPacketState state;
  // Records the first sent packet after this packet was detected lost. Zero if
  // this packet has not been detected lost. This is used to keep lost packet
  // for another RTT (for potential spurious loss detection)
  QuicPacketNumber first_sent_after_loss;
  // The largest_acked in the ack frame, if the packet contains an ack.
  // QuicPacketNumber largest_acked;
};
// TODO(ianswett): Add static_assert when size of this struct is reduced below
// 64 bytes.
// NOTE(vlovich): Existing static_assert removed because padding differences on
// 64-bit iOS resulted in an 88-byte struct that is greater than the 84-byte
// limit on other platforms.  Removing per ianswett's request.


#endif  // QUICHE_QUIC_CORE_QUIC_TRANSMISSION_INFO_H_
