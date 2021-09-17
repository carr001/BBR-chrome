#include "src/packet/packet_parser.h"

#include <string>
#include <iostream>

#include "src/common/defs.h"
#include "src/quic/quic_time.h"
#include "src/quic/quic_packet_number.h"

Packet* ClientPacketParser::UnSerialize(const char* data){
    std::string s(data);
    // std::cout<<s<<std::endl;
    // std::cout<<s.substr(TYPE_LEN,SIZE_T_LEN)<<std::endl;


    SerializedPacket* packet =  new SerializedPacket();
    packet->packet_number    = QuicPacketNumber(std::stoul( s.substr(TYPE_LEN,SIZE_T_LEN)));
    packet->send_time        = QuicTime(std::stoul( s.substr(TYPE_LEN+SIZE_T_LEN,SIZE_T_LEN)));
    packet->encrypted_length = std::stoul( s.substr(TYPE_LEN+2*SIZE_T_LEN,SIZE_T_LEN));

    // std::cout<<packet->timestamp  <<std::endl;
    // std::cout<<std::to_string(std::stod( s.substr(TYPE_LEN+SIZE_T_LEN,TIME_SRTING_LEN))) <<std::endl;
    return (Packet*)packet;
}





