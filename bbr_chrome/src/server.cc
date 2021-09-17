    #include "src/server.h"

    #include <stdio.h>
    #include <iostream>
    #include <string.h>
    #include <arpa/inet.h>

    #include "src/quic/quic_chromium_clock.h"
    #include "src/quic/quic_time.h"
    #include "src/common/utils.h"
    Server::Server()
        :
        last_print_timestamp_(QuicTime::Zero()),
        clock_(new QuicChromiumClock()),
        packets_received_in_one_second_(0),
        print_receive_info_duration_(QuicTime::Delta::FromMicroseconds(ONE_SECOND) )
        {
        last_print_timestamp_        = clock_->Now();
        local_addr_.sin_family      = AF_INET;
        // local_addr_.sin_port = htonl(SERVERPORT);
        local_addr_.sin_port        = htons(SERVERPORT);
        local_addr_.sin_addr.s_addr = htonl(SERVER_IP);

        Bind();
    }
    Server::~Server(){
        close(local_socket_);
    }
    void Server::Bind(){
        local_socket_ = socket(AF_INET, SOCK_DGRAM, 0);
        int num  = bind(local_socket_, (struct sockaddr*)&local_addr_, sizeof(struct sockaddr));
        if(num<0){
            pause_at_func(__FUNCTION__);
        }
        std::cout<<"bind port "<<SERVERPORT<<" succeed."<<std::endl;
    }

    void Server::SendACK(){
        ssize_t num;
        socklen_t addrlen = sizeof(struct sockaddr_in);
        num = sendto(local_socket_,receive_buf_,strlen(receive_buf_),MSG_DONTWAIT,(struct sockaddr *)&remote_addr_,addrlen);
        if(num<0){
            pause_at_func(__FUNCTION__);
        }
    }


    void Server::OnReceivePacket(){
        SendACK();
        // SendACKAndSimulateLoss();
    }

    void Server::PrintReceiveRate(){
        if(clock_->Now()>last_print_timestamp_+print_receive_info_duration_)
        {   // if mulitiply by MILLIONSECOND_TO_SECOND, unit is b/s, otherwise kb/s

            // double reveive_rate =MILLIONSECOND_TO_SECOND* (double)(packets_received_in_one_second_*DEFAULT_PACKET_SIZE*BITS_PER_BYTE)/print_receive_info_duration_;
            double reveive_rate = (packets_received_in_one_second_*DEFAULT_PACKET_SIZE*BITS_PER_BYTE);
            if(reveive_rate!=0){
                std::cout
                <<packets_received_in_one_second_
                <<" packets received in 1s. "
                <<"Receive data rate: "
                <<reveive_rate/1000.f
                <<" kb/s"
                <<std::endl;
                last_print_timestamp_ = clock_->Now();
                packets_received_in_one_second_ = 0;
            }
        }
    }

    void Server::ReceiveData(){
        // std::cout<<"RecvData entered"<<std::endl;
        memset(receive_buf_,'\0',RECEIVE_BUF_SIZE);
        ssize_t num;
        socklen_t addrlen = sizeof(struct sockaddr_in);
        num =recvfrom(local_socket_,receive_buf_,RECEIVE_BUF_SIZE,MSG_DONTWAIT,(struct sockaddr*)&remote_addr_,&addrlen); 
        // std::cout<<"buff received"<<std::endl;
        if (num < 0){
            // std::cout<<"No received packets yet"<<std::endl;
        }else{
            // recent_packet_number_ = parser_->GetPacketNumber(receive_buf_);
            ++packets_received_in_one_second_;
            // std::cout<<receive_buf_<<std::endl;
            OnReceivePacket();
        }
        PrintReceiveRate();
    }


