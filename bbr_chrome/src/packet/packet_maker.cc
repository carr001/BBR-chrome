#include "src/packet/packet_maker.h"

#include <iostream>

#include "src/common/utils.h"
#include "src/quic/quic_packet_number.h"
#include "src/quic/quic_time.h"
#include "src/quic/quic_chromium_clock.h"
SerializedPacketMaker::SerializedPacketMaker()
    :recent_packet_create_time_(QuicTime::Zero()),
    recent_packet_number_(0),
    packet_template_(new SerializedPacket())
    {}


SerializedPacket* SerializedPacketMaker::MakePacket(){
    ResetTemplate();
    recent_packet_number_ ++;
    
    packet_template_->packet_number = QuicPacketNumber(recent_packet_number_) ;
    // if(recent_packet_number_== 20){
    //     std::cout<<packet_template_->Serialize() <<std::endl;
    //     pause_at_func(__FUNCTION__);
    // }
    return packet_template_;
}

QuicPacketNumber SerializedPacketMaker::GetRecentPacketNumber(){
    return recent_packet_number_;
}


void SerializedPacketMaker::ResetTemplate(){
    packet_template_->packet_number = QuicPacketNumber(0) ;
}


