#include "src/common/utils.h"

#include <string>
#include <iostream>
#include <unistd.h>

#include "src/common/defs.h"

std::string size_t_to_string(size_t number){
    char c[SIZE_T_LEN+1];
    number = number%10000000000;
    for(int i=0;i<SIZE_T_LEN+1;i++){
        c[i] = '0';
    }
    std::string s = std::to_string(number);
    if(s.length()<=SIZE_T_LEN){
        for(int i=SIZE_T_LEN-1,j=s.length()-1;j>-1;i--,j--){
            c[i] = s[j];
        }
    }else{
        std::cout<<"Number to big, conversion not completed"<<std::endl;
    }
    c[SIZE_T_LEN] = '\0';
    return std::string(c);
}

std::string number_to_fix_len_string(size_t number,size_t len){
    char* c = (char*) malloc(len);
    for(int i=0;i<len+1;i++){
        c[i] = '0';
    }
    std::string s = std::to_string(number);
    if(s.length()<=len){
        for(int i=len-1,j=s.length()-1;j>-1;i--,j--){
            c[i] = s[j];
        }
    }else{
        std::cout<<"Number to big, conversion not completed"<<std::endl;
    }
    c[len] = '\0';
    s = std::string(c);
    delete c;
    return s;
}

void pause_at_func(std::string s){
    std::cout<<" #############        We are in " <<s<<"   #################"<<std::endl;
    std::cout<<" ############# Fatal error occured, The program is paused for debug #################"<<std::endl;
    // ClientPrintDebugInfo();
    pause();
}

