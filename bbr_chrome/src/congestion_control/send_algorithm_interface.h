#ifndef SEND_ALGORITHM_INTERFACE_H
#define SEND_ALGORITHM_INTERFACE_H 1



#include "src/quic/quic_connection_stats.h"
#include "src/quic/quic_unacked_packet_map.h"
#include "src/quic/quic_time.h"
#include "src/quic/quic_clock.h"
#include "src/congestion_control/rtt_stats.h"
class  SendAlgorithmInterface {
 public:

  static SendAlgorithmInterface* Create(
      const QuicClock* clock,
      const RttStats* rtt_stats,
      const QuicUnackedPacketMap* unacked_packets,
      CongestionControlType type,
      QuicConnectionStats* stats,
      QuicPacketCount initial_congestion_window,
      SendAlgorithmInterface* old_send_algorithm);

  virtual ~SendAlgorithmInterface() {}


  // Indicates an update to the congestion state, caused either by an incoming
  // ack or loss event timeout.  |rtt_updated| indicates whether a new
  // latest_rtt sample has been taken, |prior_in_flight| the bytes in flight
  // prior to the congestion event.  |acked_packets| and |lost_packets| are any
  // packets considered acked or lost as a result of the congestion event.
  virtual void OnCongestionEvent(bool rtt_updated,
                                 QuicByteCount prior_in_flight,
                                 QuicTime event_time,
                                 const AckedPacketVector& acked_packets,
                                 const LostPacketVector& lost_packets) = 0;

  // Inform that we sent |bytes| to the wire, and if the packet is
  // retransmittable.  |bytes_in_flight| is the number of bytes in flight before
  // the packet was sent.
  // Note: this function must be called for every packet sent to the wire.
  virtual void OnPacketSent(QuicTime sent_time,
                            QuicByteCount bytes_in_flight,
                            QuicPacketNumber packet_number,
                            QuicByteCount bytes) = 0;

  // Make decision on whether the sender can send right now.  Note that even
  // when this method returns true, the sending can be delayed due to pacing.
  virtual bool CanSend(QuicByteCount bytes_in_flight) = 0;

  // The pacing rate of the send algorithm.  May be zero if the rate is unknown.
  virtual QuicBandwidth PacingRate(QuicByteCount bytes_in_flight) const = 0;

  // What's the current estimated bandwidth in bytes per second.
  // Returns 0 when it does not have an estimate.
  virtual QuicBandwidth BandwidthEstimate() const = 0;

  // Returns the size of the current congestion window in bytes.  Note, this is
  // not the *available* window.  Some send algorithms may not use a congestion
  // window and will return 0.
  virtual QuicByteCount GetCongestionWindow() const = 0;

  // Whether the send algorithm is currently in slow start.  When true, the
  // BandwidthEstimate is expected to be too low.
  virtual bool InSlowStart() const = 0;

  // Whether the send algorithm is currently in recovery.
  virtual bool InRecovery() const = 0;
  virtual CongestionControlType GetCongestionControlType() const = 0;
 
  virtual QuicByteCount GetSlowStartThreshold() const = 0;
  virtual void OnRetransmissionTimeout(bool packets_retransmitted) = 0;

};

#endif