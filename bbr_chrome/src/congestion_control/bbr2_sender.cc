// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "src/congestion_control/bbr2_sender.h"

#include <cstddef>
#include <fstream>
#include <iostream>

#include "src/congestion_control/bandwidth_sampler.h"
#include "src/congestion_control/bbr2_drain.h"
#include "src/congestion_control/bbr2_misc.h"
#include "src/quic/quic_bandwidth.h"
#include "src/quic/quic_types.h"
#include "src/quic/quic_chromium_clock.h"

namespace {
// Constants based on TCP defaults.
// The minimum CWND to ensure delayed acks don't reduce bandwidth measurements.
// Does not inflate the pacing rate.
const QuicByteCount kDefaultMinimumCongestionWindow = 4 * kMaxSegmentSize;

const float kInitialPacingGain = 2.885f;

const int kMaxModeChangesPerCongestionEvent = 4;
}  // namespace

// Call |member_function_call| based on the current Bbr2Mode we are in. e.g.
//
//   auto result = BBR2_MODE_DISPATCH(Foo());
//
// is equivalent to:
//
//   Bbr2ModeBase& Bbr2Sender::GetCurrentMode() {
//     if (mode_ == Bbr2Mode::STARTUP) { return startup_; }
//     if (mode_ == Bbr2Mode::DRAIN) { return drain_; }
//     ...
//   }
//   auto result = GetCurrentMode().Foo();
//
// Except that BBR2_MODE_DISPATCH guarantees the call to Foo() is non-virtual.
//
#define BBR2_MODE_DISPATCH(member_function_call)     \
  (mode_ == Bbr2Mode::STARTUP                        \
       ? (startup_.member_function_call)             \
       : (mode_ == Bbr2Mode::PROBE_BW                \
              ? (probe_bw_.member_function_call)     \
              : (mode_ == Bbr2Mode::DRAIN            \
                     ? (drain_.member_function_call) \
                     : (probe_rtt_or_die().member_function_call))))

Bbr2Sender::Bbr2Sender(QuicTime now,
                       const RttStats* rtt_stats,
                       const QuicUnackedPacketMap* unacked_packets,
                       QuicPacketCount initial_cwnd_in_packets,
                       QuicPacketCount max_cwnd_in_packets,
                       QuicConnectionStats* stats,
                       BbrSender* old_sender)
    : mode_(Bbr2Mode::STARTUP),
      rtt_stats_(rtt_stats),
      unacked_packets_(unacked_packets),
      connection_stats_(stats),
      params_(kDefaultMinimumCongestionWindow,
              max_cwnd_in_packets * kDefaultTCPMSS),
      model_(&params_,
             rtt_stats->SmoothedOrInitialRtt(),
             rtt_stats->last_update_time(),
             /*cwnd_gain=*/1.0,
             /*pacing_gain=*/kInitialPacingGain,
             /*old_sender ? &old_sender->sampler_ : */nullptr),
      initial_cwnd_(cwnd_limits().ApplyLimits(/*
          (old_sender) ? old_sender->GetCongestionWindow()
                       :*/ (initial_cwnd_in_packets * kDefaultTCPMSS))),
      cwnd_(initial_cwnd_),
      pacing_rate_(kInitialPacingGain * QuicBandwidth::FromBytesAndTimeDelta(
                                            cwnd_,
                                            rtt_stats->SmoothedOrInitialRtt())),
      startup_(this, &model_, now),
      drain_(this, &model_),
      probe_bw_(this, &model_),
      probe_rtt_(this, &model_),
      last_sample_is_app_limited_(false),
      clock_(new QuicChromiumClock()),
      start_bbr_timestamp_(now)
       {
      std::ofstream logger(BBR_LOG_FILENAME,std::ios::trunc); 
      // clock_ = new QuicChromiumClock();
      // std::cout<<"a";
}


Limits<QuicByteCount> Bbr2Sender::GetCwndLimitsByMode() const {
  switch (mode_) {
    case Bbr2Mode::STARTUP:
      return startup_.GetCwndLimits();
    case Bbr2Mode::PROBE_BW:
      return probe_bw_.GetCwndLimits();
    case Bbr2Mode::DRAIN:
      return drain_.GetCwndLimits();
    case Bbr2Mode::PROBE_RTT:
      return probe_rtt_.GetCwndLimits();
    default:
      return Unlimited<QuicByteCount>();
  }
}

const Limits<QuicByteCount>& Bbr2Sender::cwnd_limits() const {
  return params().cwnd_limits;
}

void Bbr2Sender::OnCongestionEvent(bool /*rtt_updated*/,
                                   QuicByteCount prior_in_flight,
                                   QuicTime event_time,
                                   const AckedPacketVector& acked_packets,
                                   const LostPacketVector& lost_packets) {
  prior_infly_ = prior_in_flight;
  Bbr2CongestionEvent congestion_event;
  congestion_event.prior_cwnd = cwnd_;
  congestion_event.prior_bytes_in_flight = prior_in_flight;
  congestion_event.is_probing_for_bandwidth =
      BBR2_MODE_DISPATCH(IsProbingForBandwidth());

  model_.OnCongestionEventStart(event_time, acked_packets, lost_packets,
                                &congestion_event);

  if (InSlowStart()) {
    if (!lost_packets.empty()) {
      connection_stats_->slowstart_packets_lost += lost_packets.size();
      connection_stats_->slowstart_bytes_lost += congestion_event.bytes_lost;
    }
    if (congestion_event.end_of_round_trip) {
      ++connection_stats_->slowstart_num_rtts;
    }
  }

  // Number of mode changes allowed for this congestion event.
  int mode_changes_allowed = kMaxModeChangesPerCongestionEvent;
  while (true) {
    Bbr2Mode next_mode = BBR2_MODE_DISPATCH(
        OnCongestionEvent(prior_in_flight, event_time, acked_packets,
                          lost_packets, congestion_event));

    if (next_mode == mode_) {
      break;
    }

    BBR2_MODE_DISPATCH(Leave(event_time, &congestion_event));
    mode_ = next_mode;
    BBR2_MODE_DISPATCH(Enter(event_time, &congestion_event));
    --mode_changes_allowed;
    if (mode_changes_allowed < 0) {

      break;
    }
  }

  UpdatePacingRate(congestion_event.bytes_acked);

  UpdateCongestionWindow(congestion_event.bytes_acked);

  model_.OnCongestionEventFinish(unacked_packets_->GetLeastUnacked(),
                                 congestion_event);
  last_sample_is_app_limited_ = congestion_event.last_sample_is_app_limited;
  if (congestion_event.bytes_in_flight == 0 &&
      params().avoid_unnecessary_probe_rtt) {
    OnEnterQuiescence(event_time);
  }
  BBRInfoOnReceiveACK(acked_packets,lost_packets);
}

void Bbr2Sender::BBRInfoOnReceiveACK(const AckedPacketVector& acked_packets,const LostPacketVector& lost_packets){
    std::ofstream logger(BBR_LOG_FILENAME,std::ios::app);
    logger<<"  "
        <<"state="<<ModeToString(mode_)<<", "
        <<"btlbw="<< model_.MaxBandwidth().ToKBitsPerSecond()<<"kb/s, "
        <<"p_rate="<<(double)pacing_rate_.ToKBitsPerSecond()<<"kb/s, "
        // <<"rs_rate="<<model_.recent_sample.sample_max_bandwidth.ToKBitsPerSecond()<<"kb/s, "
        <<"rtprop="<<model_.MinRtt().ToMilliseconds()<<"ms, "
        // <<"pkt_rtt_="<<model_.recent_sample.sample_rtt.ToMilliseconds()<<"ms, "
        // <<"time="<<(clock_->Now()-start_bbr_timestamp_).ToSeconds()<<"s"
        <<"\n";
    logger.close();
    std::cout<<"  "
        <<"state="<<ModeToString(mode_)<<", "
        <<"btlbw="<< model_.MaxBandwidth().ToKBitsPerSecond()<<"kb/s, "
        <<"p_rate="<<(double)pacing_rate_.ToKBitsPerSecond()<<"kb/s, "
        // <<"rs_rate="<<model_.recent_sample.sample_max_bandwidth.ToKBitsPerSecond()<<"kb/s, "
        <<"rtprop="<<model_.MinRtt().ToMilliseconds()<<"ms, "
        // <<"pkt_rtt_="<<model_.recent_sample.sample_rtt.ToMilliseconds()<<"ms, "
        // <<"time="<<(clock_->Now()-start_bbr_timestamp_).ToSeconds()<<"s "
        <<"prio_fly="<<prior_infly_/DEFAULT_PACKET_SIZE<<", "
        <<"pkts_lost="<<lost_packets.size()<<", "
        <<"cwnd_sz="<<GetCongestionWindow()/DEFAULT_PACKET_SIZE<<", "
        <<"tgt_cwnd_="<<model_.MinRtt() * BandwidthEstimate()/DEFAULT_PACKET_SIZE<<", "
        <<"pkts_delivered="<<acked_packets.size()<<", "
        <<"\n"
        <<std::endl;
}


void Bbr2Sender::UpdatePacingRate(QuicByteCount bytes_acked) {
  if (BandwidthEstimate().IsZero()) {
    return;
  }

  if (model_.total_bytes_acked() == bytes_acked) {
    // After the first ACK, cwnd_ is still the initial congestion window.
    pacing_rate_ = QuicBandwidth::FromBytesAndTimeDelta(cwnd_, model_.MinRtt());
    return;
  }

  QuicBandwidth target_rate = model_.pacing_gain() * model_.BandwidthEstimate();
  if (model_.full_bandwidth_reached()) {
    pacing_rate_ = target_rate;
    return;
  }
  if (params_.decrease_startup_pacing_at_end_of_round &&
      model_.pacing_gain() < Params().startup_pacing_gain) {
    pacing_rate_ = target_rate;
    return;
  }
  if (params_.bw_lo_mode_ != Bbr2Params::DEFAULT &&
      model_.loss_events_in_round() > 0) {
    pacing_rate_ = target_rate;
    return;
  }

  // By default, the pacing rate never decreases in STARTUP.
  if (target_rate > pacing_rate_) {
    pacing_rate_ = target_rate;
  }
}

void Bbr2Sender::UpdateCongestionWindow(QuicByteCount bytes_acked) {
  QuicByteCount target_cwnd = GetTargetCongestionWindow(model_.cwnd_gain());

  const QuicByteCount prior_cwnd = cwnd_;
  if (model_.full_bandwidth_reached()) {
    target_cwnd += model_.MaxAckHeight();
    cwnd_ = std::min(prior_cwnd + bytes_acked, target_cwnd);
  } else if (prior_cwnd < target_cwnd || prior_cwnd < 2 * initial_cwnd_) {
    cwnd_ = prior_cwnd + bytes_acked;
  }
  const QuicByteCount desired_cwnd = cwnd_;

  cwnd_ = GetCwndLimitsByMode().ApplyLimits(cwnd_);
  const QuicByteCount model_limited_cwnd = cwnd_;

  cwnd_ = cwnd_limits().ApplyLimits(cwnd_);

}

QuicByteCount Bbr2Sender::GetTargetCongestionWindow(float gain) const {
  return std::max(model_.BDP(model_.BandwidthEstimate(), gain),
                  cwnd_limits().Min());
}

void Bbr2Sender::OnPacketSent(QuicTime sent_time,
                              QuicByteCount bytes_in_flight,
                              QuicPacketNumber packet_number,
                              QuicByteCount bytes) {

  if (InSlowStart()) {
    ++connection_stats_->slowstart_packets_sent;
    connection_stats_->slowstart_bytes_sent += bytes;
  }
  if (bytes_in_flight == 0 && params().avoid_unnecessary_probe_rtt) {
    OnExitQuiescence(sent_time);
  }
  model_.OnPacketSent(sent_time, bytes_in_flight, packet_number, bytes);
}


bool Bbr2Sender::CanSend(QuicByteCount bytes_in_flight) {
  const bool result = bytes_in_flight < GetCongestionWindow();
  return result;
}

QuicByteCount Bbr2Sender::GetCongestionWindow() const {
  // TODO(wub): Implement Recovery?
  return cwnd_;
}

QuicBandwidth Bbr2Sender::PacingRate(QuicByteCount /*bytes_in_flight*/) const {
  return pacing_rate_;
}


QuicByteCount Bbr2Sender::GetTargetBytesInflight() const {
  QuicByteCount bdp = model_.BDP(model_.BandwidthEstimate());
  return std::min(bdp, GetCongestionWindow());
}

void Bbr2Sender::OnEnterQuiescence(QuicTime now) {
  last_quiescence_start_ = now;
}

void Bbr2Sender::OnExitQuiescence(QuicTime now) {
  if (last_quiescence_start_ != QuicTime::Zero()) {
    Bbr2Mode next_mode = BBR2_MODE_DISPATCH(
        OnExitQuiescence(now, std::min(now, last_quiescence_start_)));
    if (next_mode != mode_) {
      BBR2_MODE_DISPATCH(Leave(now, nullptr));
      mode_ = next_mode;
      BBR2_MODE_DISPATCH(Enter(now, nullptr));
    }
    last_quiescence_start_ = QuicTime::Zero();
  }
}

Bbr2Sender::DebugState Bbr2Sender::ExportDebugState() const {
  DebugState s;
  s.mode = mode_;
  s.round_trip_count = model_.RoundTripCount();
  s.bandwidth_hi = model_.MaxBandwidth();
  s.bandwidth_lo = model_.bandwidth_lo();
  s.bandwidth_est = BandwidthEstimate();
  s.inflight_hi = model_.inflight_hi();
  s.inflight_lo = model_.inflight_lo();
  s.max_ack_height = model_.MaxAckHeight();
  s.min_rtt = model_.MinRtt();
  s.min_rtt_timestamp = model_.MinRttTimestamp();
  s.congestion_window = cwnd_;
  s.pacing_rate = pacing_rate_;
  s.last_sample_is_app_limited = last_sample_is_app_limited_;
  s.end_of_app_limited_phase = model_.end_of_app_limited_phase();

  s.startup = startup_.ExportDebugState();
  s.drain = drain_.ExportDebugState();
  s.probe_bw = probe_bw_.ExportDebugState();
  s.probe_rtt = probe_rtt_.ExportDebugState();

  return s;
}

std::ostream& operator<<(std::ostream& os, const Bbr2Sender::DebugState& s) {
  os << "mode: " << s.mode << "\n";
  os << "round_trip_count: " << s.round_trip_count << "\n";
  os << "bandwidth_hi ~ lo ~ est: " << s.bandwidth_hi << " ~ " << s.bandwidth_lo
     << " ~ " << s.bandwidth_est << "\n";
  os << "min_rtt: " << s.min_rtt << "\n";
  os << "min_rtt_timestamp: " << s.min_rtt_timestamp << "\n";
  os << "congestion_window: " << s.congestion_window << "\n";
  os << "pacing_rate: " << s.pacing_rate << "\n";
  os << "last_sample_is_app_limited: " << s.last_sample_is_app_limited << "\n";

  if (s.mode == Bbr2Mode::STARTUP) {
    os << s.startup;
  }

  if (s.mode == Bbr2Mode::DRAIN) {
    os << s.drain;
  }

  if (s.mode == Bbr2Mode::PROBE_BW) {
    os << s.probe_bw;
  }

  if (s.mode == Bbr2Mode::PROBE_RTT) {
    os << s.probe_rtt;
  }

  return os;
}

