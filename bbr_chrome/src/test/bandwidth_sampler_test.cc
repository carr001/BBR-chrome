#include <iostream>

#include "src/packet/packet.h"
#include "src/packet/packet_maker.h"
#include "src/congestion_control/bandwidth_sampler.h"
#include "src/quic/packet_number_indexed_queue.h"
#include "src/quic/quic_packet_number.h"
#include "src/quic/quic_unacked_packet_map.h"
#include "src/quic/quic_chromium_clock.h"
void bandwidth_sampler_test1(){
    // PacketNumberIndexedQueue<int> a;
    // for(int i=1;i<10;i++){
    //     QuicPacketNumber p(i);
    //     a.Emplace(p);
    // }
}
void bandwidth_sampler_test2(){
    QuicUnackedPacketMap* map= new QuicUnackedPacketMap(Perspective::IS_CLIENT);

    SerializedPacketMaker packet_maker;
    QuicChromiumClock clock;
    for(int i=0;i<10000;i++){
        SerializedPacket* p = packet_maker.MakePacket();
        p->send_time = clock.Now();
        map->AddSentPacket(p,p->send_time,1);
    }
    BandwidthSampler sampler_(map,QuicRoundTripCount(1));
    std::cout<< "hell"<<std::endl;
}
int main(int argc, char*argv[]){
    // bandwidth_sampler_test1();
    bandwidth_sampler_test2();
}

