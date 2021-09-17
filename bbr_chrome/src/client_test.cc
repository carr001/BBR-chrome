#include "src/client.h"

#include "src/common/defs.h"

void client_test1(){
    Client* client = new Client();
    for(int i=0;i<INTMAX_MAX;i++){// use for loop so we can control runing time
        client->SendPacket();
        client->ReceivePacket();
    }
}


int main(int argc,char* argv[]){
    client_test1();
}


