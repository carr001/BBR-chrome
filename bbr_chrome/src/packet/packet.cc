#include "src/packet/packet.h"

#include <string>
#include <iostream>

#include "src/common/utils.h"
#include "src/common/defs.h"
#include "src/quic/quic_time.h"
Packet::Packet(){}

SerializedPacket::SerializedPacket()
    :packet_number(0),send_time(QuicTime::Zero()){}

const std::string SerializedPacket::Serialize(){
    std::string content("");
    content += "0";
    content += size_t_to_string(packet_number.ToUint64());
    content += size_t_to_string(send_time.GetUint64());
    content += size_t_to_string(encrypted_length);
    content += number_to_fix_len_string(0,SENDER_PADDING_LEN);
    return content;
}


