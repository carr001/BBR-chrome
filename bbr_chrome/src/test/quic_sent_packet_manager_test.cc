#include <iostream>

#include "src/quic/quic_sent_packet_manager.h"
#include "src/quic/quic_clock.h"
#include "src/quic/quic_types.h"
#include "src/quic/quic_connection_stats.h"
#include "src/quic/quic_chromium_clock.h"

void quic_sent_packet_manager_test1(){
    const QuicChromiumClock* clock = new QuicChromiumClock();
    QuicConnectionStats* rtt_stats = new QuicConnectionStats();

    QuicSentPacketManager* manager = 
        new QuicSentPacketManager(Perspective::IS_CLIENT,clock,rtt_stats,CongestionControlType::kBBRv2);
    std::cout<<"in"<<std::endl;
}


int main(int argc,char* argv[]){
    std::cout<<"in"<<std::endl;
    quic_sent_packet_manager_test1();
    std::cout<<"end"<<std::endl;
}


