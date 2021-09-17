// note : all size are calculated in byte


// common
#define UNSIGNED_INT_MAX 4294967295
#define SIZE_T_MAX UNSIGNED_INT_MAX
#define CREATE_OBJ_FLAG 1
#define BITS_PER_BYTE 8
#define A_LARGE_NUMBER 2147483640
//we simulation Link_RTT 200 ms
#define LINK_RTT 200.0
#define MILLIONSECOND_TO_SECOND 1000

// packet maker
#define TYPE_DATA_STR '0'
#define TYPE_INFO_STR '1'
#define TYPE_LEN 1
#define SIZE_T_LEN 10
#define SIZE_T_UNINITIALIZED 0
#define MAXDGMSIZE (1472)
#define DEFAULT_PACKET_SIZE MAXDGMSIZE
#define TIME_SRTING_LEN 20
// minus 1 means '\0'
#define SENDER_PADDING_LEN (DEFAULT_PACKET_SIZE-TYPE_LEN-SIZE_T_LEN-SIZE_T_LEN-SIZE_T_LEN-1)


// bbr_time.h
#define ONE_SECOND 1000000.0
#define ONE_MILLION_SECOND 1.0
#define CLOCK_GRANULARITY 1.0

// bbr
#define BBR_MAX_WINDOW_SIZE 10
#define BBR_MIN_PIPE_CWND 4
#define BBR_GAIN_CYCLE_LENGTH 8
#define BBR_PACING_RATE_HIGH_GAIN 2.0
#define BBR_CWND_HIGH_GAIN 2.0
#define BBR_PROBERTT_PACING_GAIN (1/BBR_PACING_RATE_HIGH_GAIN)
// #define BBR_PROBERTT_PACING_GAIN 0.8
// Rtprop filter len 10s
#define BBR_RTPROP_FILTER_LEN 10000000.0
#define BBR_PROBERTT_INTERVAL BBR_RTPROP_FILTER_LEN
#define BBR_PROBE_RTT_DURATION 200.0
#define BBR_INITIAL_CWND 1
#define BBR_ONE_MILLION_SECOND ONE_MILLION_SECOND
#define BBR_CHECK_FULL_PIPE_ROUNDS 3
#define BBR_DEFAULT_PACKET_THRES 3
#define BBR_INITIAL_TIME_THRES BAD_SRTT
#define BBR_KTIME_THRES_HOLD (1.125)
#define LOG_PATH (std::string("/home/carr/Documents/BBR/bbr_chrome/log/"))
#define BBR_LOG_FILENAME (LOG_PATH+std::string("10000_500_500_50.txt"))

// clinet_simple.h client.h
// 1500 - 20(ip header) - 8(upd header)= 1472 bytes = 1472*8 bits
#define MAXDGMSIZE (1472)
#define MAXDATASIZE MAXDGMSIZE
#define MAXNUMBERLEN 10
// pacing rate (bit/second) , 1024*10 means 10 kbits/s
#define INITIAL_PACING_RATE (1000*800)
// pacing rate = 100M at maximum
#define MAX_PACING_RATE (100000000) 
#define INITIAL_CWND_SIZE BBR_MIN_PIPE_CWND
#define DEFAULT_WAIT_ACK_DURATION 5000.0
//when detect and HLB, we double halve the sending rate so we can observe obvious slowing-down
#define WAIT_ACK_FACTOR (4)
// The following means we add 1 to total_delivered after receive ack
#define PACKET_SIZE_UNIT 1
// The following means we add DEFAULT_PACKET_SIZE to total_delivered after receive ack
// #define PACKET_SIZE_UNIT DEFAULT_PACKET_SIZE
#define INITIAL_RS_DELIVERY_RATE INITIAL_PACING_RATE
#define RTO_MIN 1000.0
#define BAD_SRTT 200.0
#define RTO_MAX (1000.0*60)
#define DEFAULT_BETA (0.25)
#define DEFAULT_ALPHA (0.125)

// packet_maker.h
#define DEFAULT_PACKET_SIZE MAXDGMSIZE
#define PACKET_START_NUMBER 1

// server_simple.h server.h
// #define SERVER_IP INADDR_ANY
// 192.168.189.128 -> c0 a8 bd 80
#define SERVER_IP (0xc0a8bd80)
#define CLIENT_IP (0xc0a8bd81)
#define SERVERPORT 53767
#define CLIENTPORT 53766

#define RECEIVE_BUF_SIZE 10000
#define SEND_BUF_SIZE RECEIVE_BUF_SIZE

// window_fiter
#define TEST_SAMPLES 10


#define MIN(x,y)  ((x)<(y)?(x):(y))
#define MAX(x,y)  ((x)>(y)?(x):(y))
// template<typename T,typename T>
// T max(T x,T y){
//     return  ((x)>(y)?(x):(y));
// }
// template<typename T,typename T>
// T min(T x,T y){
//     return  ((x)<(y)?(x):(y));
// }
