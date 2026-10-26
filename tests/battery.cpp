#include <cassert>
#include <cstdio>
#include "../src/battery_status.h"
int main(){
    assert(batteryControlLevel(4100,0)==0);
    assert(batteryControlLevel(4100,3800)==0);
    assert(batteryControlLevel(0,4100)==1);
    assert(batteryControlLevel(0,0)==-1);
    assert(batteryControlLevel(5000,4100)==-1);
    assert(batterySegments(0)==-1 && batterySegments(5000)==-1);
    assert(batterySegments(3300)==0 && batterySegments(3450)==0);
    assert(batterySegments(3500)==1 && batterySegments(3750)==2);
    assert(batterySegments(3900)==3 && batterySegments(4200)==4);
    assert(batterySegments(3990,4)==4 && batterySegments(3950,4)==3);
    assert(batterySegments(4005,3)==3 && batterySegments(4050,3)==4);
    assert(batterySegments(3440,4)==0);
    for(int previous=-1;previous<=4;++previous)for(int mv=0;mv<=5000;++mv){
        const int level=batterySegments(mv,previous);
        assert(level>=-1 && level<=4);
        assert((level>=0)==batteryVoltageValid(mv));
    }
    std::puts("battery: polarity, voltage bounds, low alert and hysteresis passed");
}
