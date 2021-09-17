
#include <netinet/ip.h>
#include <iostream>

#include "src/quic/quic_sent_packet_manager.h"
#include "src/quic/quic_chromium_clock.h"
#include "src/quic/quic_packet_number.h"
#include "src/quic/quic_unacked_packet_map.h"
#include "src/packet/packet_maker.h"
#include "src/packet/packet_parser.h"
class Client{

struct SendControl{
    SendControl()
    :cwnd(INITIAL_CWND_SIZE),
    pacing_rate(INITIAL_PACING_RATE),
    next_send_timestamp(QuicTime::Zero())
    {};
    uint64_t cwnd;
    uint64_t pacing_rate;
    QuicTime next_send_timestamp;
};

class ClientDebuger{
public:
    ClientDebuger():
        pring_info_duration(QuicTime::Delta::FromMicroseconds(ONE_SECOND)),
        clock(new QuicChromiumClock()),
        last_print_timestamp(clock->Now())
        {}
    void OnReceivePacket(SendControl send_control){
        if(clock->Now()>last_print_timestamp+pring_info_duration){
            std::cout
            <<"Pacing Rate: "
            <<send_control.pacing_rate/1000.f
            <<" kbit/s"
            <<" cwnd: "
            <<send_control.cwnd
            <<std::endl;
            last_print_timestamp = clock->Now();
        }
    };
    QuicTime::Delta pring_info_duration;
    QuicClock* clock ;
    QuicTime last_print_timestamp;
};

public:
    Client();
    ~Client(){};

    void SendPacket();
    void ReceivePacket();

private:
    void SetNextSendTime();
    void MakePacketAndSendFromBuf();
    bool IsTimeToSendNextPacket();
    void Bind();
    void OnPacketReceived();

    struct sockaddr_in local_addr_;
    struct sockaddr_in remote_addr_;
    int local_socket_;
    char send_buf_[SEND_BUF_SIZE] = {'\0'};
    char receive_buf_[RECEIVE_BUF_SIZE] = {'\0'};

    SerializedPacketMaker packet_maker_;
    ClientPacketParser packet_parser_;
    QuicTime rto_start_timestamp_;

    const QuicChromiumClock* clock_;
    QuicSentPacketManager* send_manager_;
    const QuicUnackedPacketMap* unacked_packets_;
    const LostPacketVector* packets_lost_;

    QuicPacketNumber least_unacked_packet_number_;
    QuicPacketNumber next_send_number_;
    SendControl send_control_;
    ClientDebuger* debuger_;
};





