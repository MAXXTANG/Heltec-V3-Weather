#include <cassert>
#include <fstream>
#include <iterator>
#include <iostream>
#include "../src/disaster_ui.h"
#include "../src/countdown.h"
void pbm(const std::string &path,const ClockFrame &f){
    std::ofstream out(path,std::ios::binary);out<<"P4\n128 64\n";
    for(int y=0;y<64;++y)for(int x=0;x<128;x+=8){unsigned char b=0;for(int i=0;i<8;++i)if(f.ink(x+i,y))b|=128>>i;out.put(b);}
}
int main(){
    std::ifstream in("tests/fixtures/yilan-cases.html");
    const std::string html{std::istreambuf_iterator<char>(in),{}};
    DisasterList list;assert(parseDisasterHtml(html,list));
    assert(std::string(list.cases[0].sourceId)=="20261105231835003");
    assert(list.count==3);list.count=2;list.total=2;
    DisasterTracker tracker;
    assert(tracker.accept(list).count==0); // First fetch is a silent baseline.
    assert(tracker.accept(list).count==0);
    std::strcpy(list.cases[0].status,"已返隊");std::strcpy(list.cases[0].location,"宜蘭縣礁溪鄉更正路名");
    assert(tracker.accept(list).count==0); // Stable official ID survives edits.
    DisasterNavigation nav;
    nav.press(2);assert(nav.page==Page::Weather);
    nav.press(2);assert(nav.page==Page::Disaster&&nav.index==0);
    nav.reconcile(list);const auto selected=nav.selectedKey;
    std::swap(list.cases[0],list.cases[1]);nav.reconcile(list);
    assert(nav.index==1&&nav.selectedKey==selected); // Background reorder preserves selected case.
    std::swap(list.cases[0],list.cases[1]);nav.reconcile(list);
    nav.press(2);assert(nav.page==Page::Disaster&&nav.index==1);
    nav.press(2);assert(nav.page==Page::Clock);
    nav.press(0);nav.press(0);nav.press(0);assert(nav.page==Page::Clock);
    nav.press(8);nav.press(8);for(size_t i=0;i<8;++i){assert(nav.page==Page::Disaster&&nav.index==i);nav.press(8);}
    assert(nav.page==Page::Clock);
    DisasterList empty;assert(tracker.accept(empty).count==0);
    assert(tracker.accept(list).count==0); // Disappear/reappear and reconnect do not re-alert.
    DisasterList updated=list;
    updated.cases[2]=updated.cases[1];updated.cases[1]=updated.cases[0];updated.count=updated.total=3;
    auto &fresh=updated.cases[0];std::strcpy(fresh.sourceId,"20261106103000001");
    std::strcpy(fresh.received,"2026/11/06 10:30:00");std::strcpy(fresh.location,"宜蘭縣礁溪鄉測試路");std::strcpy(fresh.status,"已出動");
    const auto changes=tracker.accept(updated);assert(changes.count==1&&changes.first==0);
    assert(tracker.accept(updated).count==0);
    nav.page=Page::Weather;nav.notify(disasterKey(fresh),0,1000000);
    assert(nav.page==Page::Disaster&&nav.alertHeld);
    assert(nav.blinking(1000000)&&nav.inverted(1000000));
    assert(nav.blinking(1250000)&&!nav.inverted(1250000));
    assert(nav.blinking(3999999)&&!nav.blinking(4000000));
    assert(nav.alertHeld&&nav.page==Page::Disaster); // Blink ending is not a navigation event.
    DisasterView view;view.item=fresh;view.valid=true;view.count=3;std::strcpy(view.checked,"10:30");
    for(int step=0;step<=12;++step){
        const int64_t at=1000000+step*250000LL;ClockFrame frame;
        renderDisasterNotification(frame,view,nav,at);assert(frame.clipped==0);
        pbm("build/alert-frames/"+std::to_string(step)+".pbm",frame);
        if(step<12){ClockFrame base;view.notification=true;renderDisasterPage(base,view);
            for(size_t i=0;i<sizeof(frame.pixels);++i)assert(frame.pixels[i]==uint8_t(base.pixels[i]^(nav.inverted(at)?255:0)));
        }
    }
    assert(!nav.blinking(3601000000LL)&&nav.alertHeld&&nav.page==Page::Disaster);
    nav.press(updated.count);assert(!nav.alertHeld&&nav.page==Page::Clock);
    // A second distinct new case retriggers a full three-second interval.
    std::strcpy(updated.cases[0].sourceId,"20261106103000002");
    assert(tracker.accept(updated).count==1);
    nav.notify(disasterKey(updated.cases[0]),0,5000000);
    assert(nav.blinking(7999999)&&!nav.blinking(8000000));
    DisasterTracker firstEmpty;assert(firstEmpty.accept(empty).count==0);assert(firstEmpty.accept(list).count==2);
    // Fallback identity ignores mutable address and status.
    DisasterCase fallback=fresh;fallback.sourceId[0]=0;auto key=disasterKey(fallback);
    std::strcpy(fallback.location,"更正地址");std::strcpy(fallback.status,"已返隊");assert(disasterKey(fallback)==key);
    std::cout<<"Disaster events: stable IDs, silent baseline, edits, reconnect, manual 2/2 and 8/8, new-case preemption state, 3s blink, hold and BOOT acknowledgement passed\n";
}
