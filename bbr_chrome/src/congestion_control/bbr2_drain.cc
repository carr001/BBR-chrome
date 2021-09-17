// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/congestion_control/bbr2_drain.h"

#include "src/congestion_control/bbr2_sender.h"


Bbr2Mode Bbr2DrainMode::OnCongestionEvent(
    QuicByteCount /*prior_in_flight*/,
    QuicTime /*event_time*/,
    const AckedPacketVector& /*acked_packets*/,
    const LostPacketVector& /*lost_packets*/,
    const Bbr2CongestionEvent& congestion_event) {
  model_->set_pacing_gain(Params().drain_pacing_gain);

  // Only STARTUP can transition to DRAIN, both of them use the same cwnd gain.
  model_->set_cwnd_gain(Params().drain_cwnd_gain);

  QuicByteCount drain_target = DrainTarget();
  if (congestion_event.bytes_in_flight <= drain_target) {
    return Bbr2Mode::PROBE_BW;
  }

  return Bbr2Mode::DRAIN;
}

QuicByteCount Bbr2DrainMode::DrainTarget() const {
  QuicByteCount bdp = model_->BDP();
  return std::max<QuicByteCount>(bdp, sender_->GetMinimumCongestionWindow());
}

Bbr2DrainMode::DebugState Bbr2DrainMode::ExportDebugState() const {
  DebugState s;
  s.drain_target = DrainTarget();
  return s;
}

std::ostream& operator<<(std::ostream& os,
                         const Bbr2DrainMode::DebugState& state) {
  os << "[DRAIN] drain_target: " << state.drain_target << "\n";
  return os;
}

const Bbr2Params& Bbr2DrainMode::Params() const {
  return sender_->Params();
}

