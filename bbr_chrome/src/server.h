#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h> /* superset of previous */
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "src/common/defs.h"
#include "src/quic/quic_chromium_clock.h"
#include "src/packet/packet_parser.h"

class Server{
public:
    Server();
    ~Server();

    void ReceiveData();
    void SendACK();
    
private:
    void OnReceivePacket();
    void Bind();
    //debug
    void PrintReceiveRate();

    struct sockaddr_in local_addr_;
    struct sockaddr_in remote_addr_;

    int local_socket_;
    char receive_buf_[RECEIVE_BUF_SIZE] = {'0'};

    QuicClock* clock_;

    // for print info
    size_t packets_received_in_one_second_;
    // size_t totoal_bytes_received_in_one_second;
    QuicTime last_print_timestamp_;
    QuicTime::Delta print_receive_info_duration_;
};





