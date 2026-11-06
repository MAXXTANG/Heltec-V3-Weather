#pragma once
#include <cstdint>
#include "disaster_data.h"

inline uint64_t disasterKey(const DisasterCase &item){
    uint64_t hash=14695981039346656037ULL;
    auto add=[&](const char *s){for(;*s;++s){hash^=static_cast<unsigned char>(*s);hash*=1099511628211ULL;}hash^=0xff;hash*=1099511628211ULL;};
    if(item.sourceId[0])add(item.sourceId);
    else {add(item.received);add(item.category);} // Stable across address/status edits.
    return hash?hash:1;
}
struct DisasterChanges {size_t count=0,first=0;};
struct DisasterTracker {
    bool initialized=false;
    uint64_t seen[256]={};size_t next=0;
    char baselineNewest[20]={};
    bool contains(uint64_t key) const {for(uint64_t value:seen)if(value==key)return true;return false;}
    DisasterChanges accept(const DisasterList &list){
        DisasterChanges changes;
        if(!initialized && list.count)std::strcpy(baselineNewest,list.cases[0].received);
        for(size_t i=0;i<list.count;++i){
            const uint64_t key=disasterKey(list.cases[i]);
            if(contains(key))continue;
            // Do not reannounce historical rows first exposed by the eight-row cap.
            if(initialized && (!baselineNewest[0]||std::strcmp(list.cases[i].received,baselineNewest)>=0)){
                if(!changes.count)changes.first=i;
                ++changes.count;
            }
            seen[next]=key;next=(next+1)%256;
        }
        initialized=true;return changes;
    }
};
enum class Page : uint8_t { Clock,Weather,Disaster };
struct DisasterNavigation {
    static constexpr int64_t BlinkUs=3000000,HalfCycleUs=250000;
    Page page=Page::Clock;
    size_t index=0;
    uint64_t selectedKey=0;
    bool alertHeld=false;
    int64_t alertStarted=0;
    void reconcile(const DisasterList &list){
        if(list.count){
            bool found=false;
            for(size_t i=0;selectedKey && i<list.count;++i)if(disasterKey(list.cases[i])==selectedKey){index=i;found=true;break;}
            if(!found && index>=list.count)index=list.count-1;
            selectedKey=disasterKey(list.cases[index]);
        }else {index=0;selectedKey=0;}
    }
    void press(size_t count){
        if(alertHeld){alertHeld=false;page=Page::Clock;index=0;}
        else if(page==Page::Clock)page=Page::Weather;
        else if(page==Page::Weather){page=Page::Disaster;index=0;}
        else if(count && index+1<count)++index;
        else {page=Page::Clock;index=0;}
        selectedKey=0;
    }
    void notify(uint64_t key,size_t at,int64_t now){
        page=Page::Disaster;index=at;selectedKey=key;
        alertHeld=true;alertStarted=now;
    }
    bool blinking(int64_t now) const {return alertHeld && now-alertStarted>=0 && now-alertStarted<BlinkUs;}
    bool inverted(int64_t now) const {return blinking(now) && ((now-alertStarted)/HalfCycleUs)%2==0;}
};
