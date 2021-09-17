// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/quic/quic_transmission_info.h"


QuicTransmissionInfo::QuicTransmissionInfo()
    : sent_time(QuicTime::Zero()),
      bytes_sent(0),
      in_flight(false),
      state(OUTSTANDING){}

QuicTransmissionInfo::QuicTransmissionInfo(QuicPacketNumber number,
                                           QuicTime sent_time,
                                           QuicPacketLength bytes_sent)
    : packet_number(number),
      sent_time(sent_time),
      bytes_sent(bytes_sent),
      in_flight(false),
      state(OUTSTANDING) {}

QuicTransmissionInfo::QuicTransmissionInfo(const QuicTransmissionInfo& other) =
    default;

QuicTransmissionInfo::~QuicTransmissionInfo() {}

// std::string QuicTransmissionInfo::DebugString() const {
//   return absl::StrCat(
//       "{sent_time: ", sent_time.ToDebuggingValue(),
//       ", bytes_sent: ", bytes_sent,
//       ", encryption_level: ", EncryptionLevelToString(encryption_level),
//       ", transmission_type: ", TransmissionTypeToString(transmission_type),
//       ", in_flight: ", in_flight, ", state: ", state,
//       ", has_crypto_handshake: ", has_crypto_handshake,
//       ", has_ack_frequency: ", has_ack_frequency,
//       ", first_sent_after_loss: ", first_sent_after_loss.ToString(),
//       ", largest_acked: ", largest_acked.ToString(),
//       ", retransmittable_frames: ", QuicFramesToString(retransmittable_frames),
//       "}");
// }

