#pragma once
#include <cstdint>
#include <cstring>
#include "disaster_data.h"

// Local USB diagnostic command only. No web endpoint, automatic boot demo,
// stored fake cases, or mutation of the official incident cache.
struct DisasterSimulation {
    bool armed=false,active=false;
    int64_t dueUs=0;
    DisasterCase item;
    char command[32]={};size_t length=0;bool overflow=false;
    bool input(char c,int64_t now){
        if(c=='\r')return false;
        if(c=='\n'){
            const bool accepted=!overflow&&!armed&&!active&&std::strcmp(command,"SIMULATE_NEW_CASE")==0;
            length=0;overflow=false;command[0]=0;
            if(accepted){armed=true;dueUs=now+8000000;}
            return accepted;
        }
        if(length+1<sizeof(command)&&!overflow){command[length++]=c;command[length]=0;}
        else overflow=true;
        return false;
    }
    bool due(int64_t now) const {return armed&&now>=dueUs;}
    void start(const char *received){
        armed=false;active=true;item=DisasterCase{};
        std::strcpy(item.sourceId,"LOCAL-SIMULATION");
        std::strncpy(item.received,received,sizeof(item.received)-1);
        std::strcpy(item.category,"測試");
        std::strcpy(item.location,"非真實災情");
        std::strcpy(item.detail,"這是通知功能測試");
        std::strcpy(item.units,"按BOOT返回時鐘");
        std::strcpy(item.status,"模擬");
    }
    void clear(){armed=active=false;dueUs=0;item=DisasterCase{};length=0;overflow=false;command[0]=0;}
};
