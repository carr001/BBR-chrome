#include "src/packet/packet.h"
#include "src/packet/packet_maker.h"
#include "src/quic/quic_unacked_packet_map.h"
#include "src/quic/quic_chromium_clock.h"
void quic_unacked_packet_map_test1(){
    QuicUnackedPacketMap map(Perspective::IS_CLIENT);

    SerializedPacketMaker packet_maker;
    QuicChromiumClock clock;
    for(int i=0;i<10000;i++){
        SerializedPacket* p = packet_maker.MakePacket();
        p->send_time = clock.Now();
        map.AddSentPacket(p,p->send_time,1);
    }
}
int main(int argc, char*argv[]){
    quic_unacked_packet_map_test1();
}




