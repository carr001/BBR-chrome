#include "src/client.h"

#include <iostream>
#include <fstream>

#include "src/common/defs.h"
#include "src/quic/quic_sent_packet_manager.h"
#include "src/quic/quic_chromium_clock.h"
#include "src/quic/quic_time.h"
#include "src/common/utils.h"

Client::Client()
    :clock_(new QuicChromiumClock()),
    send_manager_(new QuicSentPacketManager(Perspective::IS_CLIENT,
        clock_,new QuicConnectionStats(),CongestionControlType::kBBRv2)),
    debuger_(new ClientDebuger()),
    unacked_packets_(&send_manager_->unacked_packets()),
    packets_lost_(&send_manager_->packets_lost()),
    rto_start_timestamp_(QuicTime::Zero())
    {
        remote_addr_.sin_family      = AF_INET;
        remote_addr_.sin_port        = htons(SERVERPORT);
        remote_addr_.sin_addr.s_addr = htonl(SERVER_IP);

        local_addr_.sin_family      = AF_INET;
        local_addr_.sin_port        = htons(CLIENTPORT);
        local_addr_.sin_addr.s_addr = htonl(CLIENT_IP);
        Bind();
    }

void Client::Bind(){
    local_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
    int num  =bind(local_socket_, (struct sockaddr*)&local_addr_, sizeof(struct sockaddr));
    if(num<0){
        pause_at_func(__FUNCTION__);
    }
    std::cout<<"bind port "<<CLIENTPORT<<" succeed."<<std::endl;
}

void Client::SendPacket(){
    if(IsTimeToSendNextPacket()){
        // QuicTime sta = clock_->Now();
        MakePacketAndSendFromBuf();
        // std::cout<<clock_->Now()-sta<<std::endl;
    }
}

void Client::MakePacketAndSendFromBuf(){
    // SerializedPacket packet(*packet_maker_.MakePacket());
    SerializedPacket* packet = (SerializedPacket*)packet_maker_.MakePacket();
    packet->send_time = clock_->Now();
    const std::string to_buf= packet->Serialize();

    int num = sendto(local_socket_, to_buf.c_str(),to_buf.length(),MSG_DONTWAIT,(struct sockaddr *)&remote_addr_,sizeof(remote_addr_));
    // std::cout<< to_buf<<std::endl;
    // std::cout<< strlen(to_buf)<<std::endl;
    if(num<0){
        // to make sure the logic right, the send operation be done
        pause_at_func(__FUNCTION__);
    }
    send_manager_->OnPacketSent(packet,packet->send_time);
}

void Client::SetNextSendTime(){
    if(send_control_.pacing_rate>0){
        QuicTime::Delta duration_between_next_send = 
            QuicTime::Delta::FromMicroseconds(uint64_t(DEFAULT_PACKET_SIZE*BITS_PER_BYTE*ONE_SECOND/double(send_control_.pacing_rate))) ;
        // std::cout<<"do="<<DEFAULT_PACKET_SIZE*BITS_PER_BYTE*ONE_SECOND<<
        //     " no="<<double(send_control_.pacing_rate)<<std::endl;
        // std::cout<<uint64_t(DEFAULT_PACKET_SIZE*BITS_PER_BYTE*ONE_SECOND/double(send_control_.pacing_rate))<<std::endl;
        QuicTime now = clock_->Now();
        send_control_.next_send_timestamp   = now + duration_between_next_send;
        // std::cout<<"now:"<<now<<" dura:"<<duration_between_next_send<<std::endl;
    }else{
        pause_at_func(__FUNCTION__);
    }
}

bool Client::IsTimeToSendNextPacket(){
    if(unacked_packets_->bytes_in_flight()/DEFAULT_PACKET_SIZE<send_control_.cwnd
       && clock_->Now()>send_control_.next_send_timestamp){
        // std::cout <<unacked_packets_->bytes_in_flight()<<std::endl;
        // std::cout <<packets_lost_->size()<<std::endl;
            SetNextSendTime();
            return true;
       }
    return false;
}

void Client::ReceivePacket(){
    memset(receive_buf_,'\0',SEND_BUF_SIZE);
    socklen_t addrlen = sizeof(struct sockaddr_in);
    int num =recvfrom(local_socket_,receive_buf_,SEND_BUF_SIZE,MSG_DONTWAIT,(struct sockaddr*)&remote_addr_,&addrlen); 
    // std::cout<<num<<std::endl;
    // std::cout<<receive_buf_<<std::endl;
    if (num > 0){
        OnPacketReceived();
    }
    else if(clock_->Now()>send_manager_->GetRetransmissionTime()){
        send_manager_->OnRetransmissionTimeout();
    }
    // else{
    //     std::ofstream logger(BBR_LOG_FILENAME,std::ios::app);
    //     logger<<"  "
    //         <<"pacing_rate="<<send_control_.pacing_rate/1000<<"kbit/s, "
    //         <<"cwnd="<<send_control_.cwnd<<" "
    //         <<"\n";
    //     logger.close();
    // }
    // debuger_->OnReceivePacket(send_control_);
}

void Client::OnPacketReceived(){
    SerializedPacket* packet = (SerializedPacket*)packet_parser_.UnSerialize(receive_buf_);
    QuicTime time =  clock_->Now();
    send_manager_->OnAckFrameStart(packet->packet_number,time);
    send_manager_->OnAckRange(packet->packet_number,packet->packet_number+1);
    send_manager_->OnAckFrameEnd(time,packet->packet_number);

    // Get feed back
    send_control_.pacing_rate = send_manager_->GetPacingRate().ToBitsPerSecond();
    if(send_control_.pacing_rate>MAX_PACING_RATE)
        send_control_.pacing_rate = MAX_PACING_RATE;
    send_control_.cwnd = send_manager_->GetCongestionWindowInBytes()/DEFAULT_PACKET_SIZE;
    // debuger_->PrintPacketInfo(packet);
}





