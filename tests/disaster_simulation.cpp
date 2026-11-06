#include <cassert>
#include <iostream>
#include "../src/disaster_simulation.h"
#include "../src/disaster_ui.h"
bool send(DisasterSimulation &s,const char *text,int64_t now){bool result=false;for(;*text;++text)result=s.input(*text,now)||result;return result;}
int main(){
    DisasterSimulation s;assert(!s.armed&&!s.active);
    assert(!send(s,"OTHER\n",100));assert(!s.armed);
    assert(!send(s,"SIMULATE_NEW_CASE_TOO_LONG_TO_ACCEPT_ANY_MORE\n",100));
    assert(send(s,"SIMULATE_NEW_CASE\r\n",100));
    assert(s.armed&&!s.active&&!s.due(8000099)&&s.due(8000100));
    assert(!send(s,"SIMULATE_NEW_CASE\n",200));assert(s.dueUs==8000100);
    s.start("2026/11/06 11:00:00");assert(!s.armed&&s.active);
    assert(std::string(s.item.category)=="測試"&&std::string(s.item.location)=="非真實災情");
    DisasterTracker isolated;DisasterList fixture;assert(isolated.accept(fixture).count==0);
    fixture.count=fixture.total=1;fixture.cases[0]=s.item;assert(isolated.accept(fixture).count==1);
    assert(isolated.accept(fixture).count==0);
    DisasterNavigation nav;nav.notify(disasterKey(s.item),0,8000100);
    DisasterView v;v.valid=v.simulated=true;v.item=s.item;v.count=v.total=1;
    for(int i=0;i<=12;++i){ClockFrame f;renderDisasterNotification(f,v,nav,8000100+i*250000LL);assert(f.clipped==0);}
    assert(nav.alertHeld&&!nav.blinking(11000100)&&nav.page==Page::Disaster);
    nav.press(1);s.clear();assert(nav.page==Page::Clock&&!nav.alertHeld&&!s.active&&!s.armed);
    assert(send(s,"SIMULATE_NEW_CASE\n",20000000));s.clear();assert(!s.due(30000000));
    std::cout<<"USB simulation: exact command, bounded input, single arm, isolated new-case detection, labeled render, 3s hold and manual cleanup passed\n";
}
