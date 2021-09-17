#include "src/server.h"

#include <iostream>

void server_test1(){
    Server* server =  new Server();
    std::cout<<"start to receive data"<<std::endl;
    while (1){
        server->ReceiveData();
    }
}
int main(int argc,char* argv[]){
    server_test1();
}

