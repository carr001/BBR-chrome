
#include "src/congestion_control/send_algorithm_interface.h"
#include "src/congestion_control/chrome_bbr_sender.h"
#include "src/congestion_control/bbr2_sender.h"
#include "src/congestion_control/tcp_cubic_sender_bytes.h"
#include "src/quic/quic_unacked_packet_map.h"

// Factory for send side congestion control algorithm.
SendAlgorithmInterface* SendAlgorithmInterface::Create(
    const QuicClock* clock,
    const RttStats* rtt_stats,
    const QuicUnackedPacketMap* unacked_packets,
    CongestionControlType congestion_control_type,
    QuicConnectionStats* stats,
    QuicPacketCount initial_congestion_window,
    SendAlgorithmInterface* old_send_algorithm) {
  QuicPacketCount max_congestion_window = 100000;
  switch (congestion_control_type) {
    case kBBR:
      return new BbrSender(clock->ApproximateNow(), rtt_stats, unacked_packets,
                           initial_congestion_window, max_congestion_window,
                           stats);
    case kBBRv2:
      return new Bbr2Sender(
          clock->ApproximateNow(), rtt_stats, unacked_packets,
          initial_congestion_window, max_congestion_window, stats,
          /*old_send_algorithm &&
                  old_send_algorithm->GetCongestionControlType() == kBBR
              ? static_cast<BbrSender*>(old_send_algorithm)
              : */nullptr);
    case kCubicBytes:
      return new TcpCubicSenderBytes(
          clock, rtt_stats, false /* don't use Reno */,
          initial_congestion_window, max_congestion_window, stats);
  }
  return nullptr;
}