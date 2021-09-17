#ifndef PACKET_MAKER_H
#define PACKET_MAKER_H 1

#include "src/packet/packet.h"
#include "src/quic/quic_time.h"
#include "src/quic/quic_packet_number.h"
class  SerializedPacketMaker
{
public:
    SerializedPacketMaker(/* args */);
    ~ SerializedPacketMaker()=default;

    SerializedPacket* MakePacket();
    QuicPacketNumber GetRecentPacketNumber();

private:
    void ResetTemplate();

    QuicTime recent_packet_create_time_;
    QuicPacketNumber recent_packet_number_;
    SerializedPacket* packet_template_;
};




#endif
