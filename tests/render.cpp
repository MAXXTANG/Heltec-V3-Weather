#include <fstream>
#include <cassert>
#include <string>
#include "../src/clock_ui.h"

static void writePbm(const char *path,const ClockFrame &frame) {
    std::ofstream out(path,std::ios::binary);out<<"P4\n128 64\n";
    for(int y=0;y<64;++y)for(int xb=0;xb<16;++xb){
        uint8_t b=0;for(int bit=0;bit<8;++bit)if(frame.ink(xb*8+bit,y))b|=0x80>>bit;
        out.put(b);
    }
    assert(out.good());
}
static void checkPages(const ClockView &view) {
    ClockFrame clock,weather,header;
    renderClockPage(clock,view,false);renderClockPage(weather,view,true);
    renderClockHeader(header,view);
    assert(clock.clipped==0 && weather.clipped==0 && header.clipped==0);
    for(int y=0;y<=10;++y)for(int x=0;x<128;++x) {
        assert(clock.ink(x,y)==header.ink(x,y));
        assert(weather.ink(x,y)==header.ink(x,y));
    }
    assert(std::memcmp(clock.pixels,weather.pixels,sizeof(clock.pixels))!=0);
    // Weather art must finish above the humidity row.
    ClockFrame icon;
    if(view.valid)clockWeatherIcon(icon,0,12,conditionIcon(view.code,view.day));
    for(int y=50;y<64;++y)for(int x=0;x<44;++x)assert(!icon.ink(x,y));
    int rhLeft=44,rhRight=-1;
    for(int y=50;y<64;++y)for(int x=0;x<44;++x)if(weather.ink(x,y)) {
        if(x<rhLeft)rhLeft=x;if(x>rhRight)rhRight=x;
    }
    assert(rhRight>=rhLeft && std::abs(rhLeft-(43-rhRight))<=1);
    if(view.valid) {
        int left=44,right=-1,top=50,bottom=-1;
        for(int y=12;y<50;++y)for(int x=0;x<44;++x)if(icon.ink(x,y)) {
            if(x<left)left=x;if(x>right)right=x;
            if(y<top)top=y;if(y>bottom)bottom=y;
        }
        assert(right>=left && bottom>=top);
        assert(std::abs(left-(43-right))<=1);
        assert(std::abs((top-12)-(49-bottom))<=1);
    }
    // Header groups keep visible space even with the longer weekday/date strings.
    ClockFrame place,date,battery;
    clockText(place,23,1,"JIAOXI",ui_font11,0,1);
    clockText(date,126,1,view.date,ui_font11,2,1);
    clockBattery(battery,view.batteryLevel);
    for(int y=0;y<11;++y)for(int x=0;x<128;++x){
        assert(!(place.ink(x,y)&&date.ink(x,y)));
        assert(!(battery.ink(x,y)&&(place.ink(x,y)||date.ink(x,y))));
    }
}
int main(int argc,char **argv){
    ClockView view;std::strcpy(view.time,"18:26");std::strcpy(view.date,"Mon 10/26");
    view.valid=true;view.temperature=24.0f;view.humidity=85;view.code=3;view.batteryLevel=3;
    checkPages(view);
    if(argc>=3){
        ClockFrame frame;renderClockPage(frame,view,false);writePbm(argv[1],frame);
        renderClockPage(frame,view,true);writePbm(argv[2],frame);
    }
    if(argc==4){
        struct Example {const char *name;int code;bool day;};
        const Example examples[]={{"clear",0,true},{"night",0,false},{"partly-cloudy",2,true},
            {"partly-night",2,false},{"overcast",3,true},{"fog",45,true},{"drizzle",51,true},{"rain",61,true},{"showers",80,true},{"snow",71,true},{"storm",95,true}};
        for(const auto &example:examples){
            view.code=example.code;view.day=example.day;
            ClockFrame icon;clockWeatherIcon(icon,42,10,conditionIcon(view.code,view.day));
            writePbm((std::string(argv[3])+"/icons-"+example.name+".pbm").c_str(),icon);
        }
    }
    const int codes[]={0,1,2,3,45,48,51,55,61,67,71,77,80,82,85,86,95,99};
    for(int code:codes)for(int rh:{0,9,10,99,100})for(float t:{-90.0f,-89.9f,-1.0f,0.0f,9.9f,25.0f,65.0f}){
        view.code=code;view.humidity=rh;view.temperature=t;view.stale=true;
        std::strcpy(view.time,"23:59");std::strcpy(view.date,"Wed 12/31");
        view.day=true;checkPages(view);view.day=false;checkPages(view);
    }
    // Every clock digit combination stays in the full-size body, below the header.
    for(int hour=0;hour<24;++hour)for(int minute=0;minute<60;++minute) {
        std::snprintf(view.time,sizeof(view.time),"%02d:%02d",hour,minute);checkPages(view);
    }
    view={};checkPages(view);view.stale=true;checkPages(view);
    for(int level=-1;level<=4;++level){view.batteryLevel=level;checkPages(view);}
    std::puts("carousel UI: fixed headers, full day and weather boundaries passed");
}
