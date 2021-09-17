
#ifndef CHROME_BBR_SENDER_H
#define CHROME_BBR_SENDER_H 1

#include "src/congestion_control/send_algorithm_interface.h"
#include "src/congestion_control/chrome_windowed_filter.h"
#include "src/congestion_control/bandwidth_sampler.h"
#include "src/quic/quic_connection_stats.h"
#include "src/quic/quic_types.h"
#include "src/quic/quic_time.h"
#include "src/quic/quic_chromium_clock.h"

class  BbrSender : public SendAlgorithmInterface {
 public:
  enum Mode {
    STARTUP,
    DRAIN,
    PROBE_BW,
    PROBE_RTT,
  };
  enum RecoveryState {
    NOT_IN_RECOVERY,
    CONSERVATION,
    GROWTH
  };

  BbrSender(QuicTime now ,
          const RttStats* rtt_stats,
          const QuicUnackedPacketMap* unacked_packets,
          QuicPacketCount initial_tcp_congestion_window,
          QuicPacketCount max_tcp_congestion_window,
          QuicConnectionStats* stats);
  BbrSender(const BbrSender&) = delete;
  BbrSender& operator=(const BbrSender&) = delete;
  ~BbrSender() override;
  bool InSlowStart() const override;
  bool InRecovery() const override;
  // Indicates an update to the congestion state, caused either by an incoming
  // ack or loss event timeout.  |rtt_updated| indicates whether a new
  // latest_rtt sample has been taken, |prior_in_flight| the bytes in flight
  // prior to the congestion event.  |acked_packets| and |lost_packets| are any
  // packets considered acked or lost as a result of the congestion event.
  void OnCongestionEvent(bool rtt_updated,
                      QuicByteCount prior_in_flight,
                      QuicTime event_time,
                      const AckedPacketVector& acked_packets,
                      const LostPacketVector& lost_packets) override;

  void OnPacketSent(QuicTime sent_time,
              QuicByteCount bytes_in_flight,
              QuicPacketNumber packet_number,
              QuicByteCount bytes) override;


  bool CanSend(QuicByteCount bytes_in_flight) override;
  QuicBandwidth PacingRate(QuicByteCount bytes_in_flight) const override;
  QuicBandwidth BandwidthEstimate() const override;
  QuicByteCount GetCongestionWindow() const override;
  CongestionControlType GetCongestionControlType() const override;


  // Gets the number of RTTs BBR remains in STARTUP phase.
  QuicRoundTripCount num_startup_rtts() const { return num_startup_rtts_; }
  bool has_non_app_limited_sample() const {
    return has_non_app_limited_sample_;
  }

  // Sets the pacing gain used in STARTUP.  Must be greater than 1.
  void set_high_gain(float high_gain) {
    high_gain_ = high_gain;
    if (mode_ == STARTUP) {
      pacing_gain_ = high_gain;
    }
  }

  // Sets the CWND gain used in STARTUP.  Must be greater than 1.
  void set_high_cwnd_gain(float high_cwnd_gain) {
    high_cwnd_gain_ = high_cwnd_gain;
    if (mode_ == STARTUP) {
      congestion_window_gain_ = high_cwnd_gain;
    }
  }

  // Sets the gain used in DRAIN.  Must be less than 1.
  void set_drain_gain(float drain_gain) {
    drain_gain_ = drain_gain;
  }
  QuicTime::Delta GetMinRtt() const;
private:

  bool IsAtFullBandwidth() const;
  QuicByteCount GetTargetCongestionWindow(float gain) const;
  QuicByteCount ProbeRttCongestionWindow() const;
  bool ShouldExtendMinRttExpiry() const;
  bool MaybeUpdateMinRtt(QuicTime now,QuicTime::Delta sample_min_rtt);

  void EnterStartupMode(QuicTime now);
  void EnterProbeBandwidthMode(QuicTime now);

  bool UpdateRoundTripCounter(QuicPacketNumber last_acked_packet);

  void UpdateGainCyclePhase(QuicTime now,
                            QuicByteCount prior_in_flight,
                            bool has_losses);
  // Tracks for how many round-trips the bandwidth has not increased
  // significantly.
  void CheckIfFullBandwidthReached(const SendTimeState& last_packet_send_state);
  // Transitions from STARTUP to DRAIN and from DRAIN to PROBE_BW if
  // appropriate.
  void MaybeExitStartupOrDrain(QuicTime now);
  // Decides whether to enter or exit PROBE_RTT.
  void MaybeEnterOrExitProbeRtt(QuicTime now,
                                bool is_round_start,
                                bool min_rtt_expired);
  // Determines whether BBR needs to enter, exit or advance state of the
  // recovery.
  void UpdateRecoveryState(QuicPacketNumber last_acked_packet,
                           bool has_losses,
                           bool is_round_start);

  // Updates the ack aggregation max filter in bytes.
  // Returns the most recent addition to the filter, or |newly_acked_bytes| if
  // nothing was fed in to the filter.
  QuicByteCount UpdateAckAggregationBytes(QuicTime ack_time,
                                          QuicByteCount newly_acked_bytes);

  // Determines the appropriate pacing rate for the connection.
  void CalculatePacingRate(QuicByteCount bytes_lost);
  // Determines the appropriate congestion window for the connection.
  void CalculateCongestionWindow(QuicByteCount bytes_acked,
                                 QuicByteCount excess_acked);
  // Determines the appropriate window that constrains the in-flight during
  // recovery.
  void CalculateRecoveryWindow(QuicByteCount bytes_acked,
                               QuicByteCount bytes_lost);

  // Called right before exiting STARTUP.
  void OnExitStartup(QuicTime now);

  // Return whether we should exit STARTUP due to excessive loss.
  bool ShouldExitStartupDueToLoss(
      const SendTimeState& last_packet_send_state) const;

  QuicByteCount GetSlowStartThreshold() const override { return 0; }
  void OnRetransmissionTimeout(bool /*packets_retransmitted*/) override {}


  using MaxBandwidthFilter = WindowedFilter<QuicBandwidth,
                                          MaxFilter<QuicBandwidth>,
                                          QuicRoundTripCount,
                                          QuicRoundTripCount>;


    const RttStats* rtt_stats_;
  const QuicUnackedPacketMap* unacked_packets_;
  QuicConnectionStats* stats_;

  Mode mode_;

  // Bandwidth sampler provides BBR with the bandwidth measurements at
  // individual points.
  BandwidthSampler sampler_;

  // The number of the round trips that have occurred during the connection.
  QuicRoundTripCount round_trip_count_;

  // The packet number of the most recently sent packet.
  QuicPacketNumber last_sent_packet_;
  // Acknowledgement of any packet after |current_round_trip_end_| will cause
  // the round trip counter to advance.
  QuicPacketNumber current_round_trip_end_;

  // Number of congestion events with some losses, in the current round.
  int64_t num_loss_events_in_round_;

  // Number of total bytes lost in the current round.
  QuicByteCount bytes_lost_in_round_;

  // The filter that tracks the maximum bandwidth over the multiple recent
  // round-trips.
  MaxBandwidthFilter max_bandwidth_;

  // Minimum RTT estimate.  Automatically expires within 10 seconds (and
  // triggers PROBE_RTT mode) if no new value is sampled during that period.
  QuicTime::Delta min_rtt_;
  // The time at which the current value of |min_rtt_| was assigned.
  QuicTime min_rtt_timestamp_;

  // The maximum allowed number of bytes in flight.
  QuicByteCount congestion_window_;

  // The initial value of the |congestion_window_|.
  QuicByteCount initial_congestion_window_;

  // The largest value the |congestion_window_| can achieve.
  QuicByteCount max_congestion_window_;

  // The smallest value the |congestion_window_| can achieve.
  QuicByteCount min_congestion_window_;

  // The pacing gain applied during the STARTUP phase.
  float high_gain_;

  // The CWND gain applied during the STARTUP phase.
  float high_cwnd_gain_;

  // The pacing gain applied during the DRAIN phase.
  float drain_gain_;

  // The current pacing rate of the connection.
  QuicBandwidth pacing_rate_;

  // The gain currently applied to the pacing rate.
  float pacing_gain_;
  // The gain currently applied to the congestion window.
  float congestion_window_gain_;

  // The gain used for the congestion window during PROBE_BW.  Latched from
  // quic_bbr_cwnd_gain flag.
  const float congestion_window_gain_constant_;
  // The number of RTTs to stay in STARTUP mode.  Defaults to 3.
  QuicRoundTripCount num_startup_rtts_;

  // Number of round-trips in PROBE_BW mode, used for determining the current
  // pacing gain cycle.
  int cycle_current_offset_;
  // The time at which the last pacing gain cycle was started.
  QuicTime last_cycle_start_;

  // Indicates whether the connection has reached the full bandwidth mode.
  bool is_at_full_bandwidth_;
  // Number of rounds during which there was no significant bandwidth increase.
  QuicRoundTripCount rounds_without_bandwidth_gain_;
  // The bandwidth compared to which the increase is measured.
  QuicBandwidth bandwidth_at_last_round_;

  // Set to true upon exiting quiescence.
  bool exiting_quiescence_;

  // Time at which PROBE_RTT has to be exited.  Setting it to zero indicates
  // that the time is yet unknown as the number of packets in flight has not
  // reached the required value.
  QuicTime exit_probe_rtt_at_;
  // Indicates whether a round-trip has passed since PROBE_RTT became active.
  bool probe_rtt_round_passed_;

  // Indicates whether the most recent bandwidth sample was marked as
  // app-limited.
  bool last_sample_is_app_limited_;
  // Indicates whether any non app-limited samples have been recorded.
  bool has_non_app_limited_sample_;

  // Current state of recovery.
  RecoveryState recovery_state_;
  // Receiving acknowledgement of a packet after |end_recovery_at_| will cause
  // BBR to exit the recovery mode.  A value above zero indicates at least one
  // loss has been detected, so it must not be set back to zero.
  QuicPacketNumber end_recovery_at_;
  // A window used to limit the number of bytes in flight during loss recovery.
  QuicByteCount recovery_window_;
  // If true, consider all samples in recovery app-limited.
  bool is_app_limited_recovery_;

  // When true, pace at 1.5x and disable packet conservation in STARTUP.
  bool slower_startup_;
  // When true, disables packet conservation in STARTUP.
  bool rate_based_startup_;

  // When true, add the most recent ack aggregation measurement during STARTUP.
  bool enable_ack_aggregation_during_startup_;
  // When true, expire the windowed ack aggregation values in STARTUP when
  // bandwidth increases more than 25%.
  bool expire_ack_aggregation_in_startup_;

  // If true, will not exit low gain mode until bytes_in_flight drops below BDP
  // or it's time for high gain mode.
  bool drain_to_target_;

  // If true, slow down pacing rate in STARTUP when overshooting is detected.
  bool detect_overshooting_;
  // Bytes lost while detect_overshooting_ is true.
  QuicByteCount bytes_lost_while_detecting_overshooting_;
  // Slow down pacing rate if
  // bytes_lost_while_detecting_overshooting_ *
  // bytes_lost_multiplier_while_detecting_overshooting_ > IW.
  uint8_t bytes_lost_multiplier_while_detecting_overshooting_;
  // When overshooting is detected, do not drop pacing_rate_ below this value /
  // min_rtt.
  QuicByteCount cwnd_to_calculate_min_pacing_rate_;

  // Max congestion window when adjusting network parameters.
  QuicByteCount max_congestion_window_with_network_parameters_adjusted_;


  ////////// add by carr
  BandwidthSamplerInterface::CongestionEventSample recent_sample_;
  QuicTime start_bbr_timestamp_;
  QuicClock* clock_;
  QuicByteCount prior_infly_;
  void BBRInfoOnReceiveACK(const AckedPacketVector& acked_packets,const LostPacketVector&);// for debug 
  static std::string ModeToString(BbrSender::Mode mode) {
    switch (mode) {
      case BbrSender::STARTUP:
        return "STARTUP";
      case BbrSender::DRAIN:
        return "DRAIN";
      case BbrSender::PROBE_BW:
        return "PROBE_BW";
      case BbrSender::PROBE_RTT:
        return "PROBE_RTT";
    }
    return "???";
  }

};

#endif

