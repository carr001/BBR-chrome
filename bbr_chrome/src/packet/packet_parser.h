
#include "src/packet/packet.h"

class PacketParser{
public:
    virtual Packet* UnSerialize(const char*)=0;
};

class ClientPacketParser:public PacketParser{
public:
    ClientPacketParser()=default;
    ~ClientPacketParser()=default;
    Packet* UnSerialize(const char*) override;
};


class GCCReceiverPacketParser:public PacketParser{
public:
    GCCReceiverPacketParser()=default;
    ~GCCReceiverPacketParser()=default;

    Packet* UnSerialize(const char*) override;
};



