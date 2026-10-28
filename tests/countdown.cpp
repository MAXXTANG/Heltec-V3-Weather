#include <cassert>
#include <cstdio>
#include <cstring>
#include "../src/countdown_ui.h"

static int lit(const ClockFrame &frame){
    int count=0;
    for(int y=0;y<64;++y)for(int x=0;x<128;++x)count+=frame.ink(x,y)?1:0;
    return count;
}
static void saveFrame(const ClockFrame &frame,const char *directory,int index){
    char path[512];std::snprintf(path,sizeof(path),"%s/frame-%04d.pbm",directory,index);
    FILE *out=std::fopen(path,"wb");assert(out);
    std::fprintf(out,"P4\n128 64\n");
    for(int y=0;y<64;++y)for(int x=0;x<128;x+=8){
        uint8_t byte=0;
        for(int bit=0;bit<8;++bit)if(frame.ink(x+bit,y))byte|=0x80>>bit;
        std::fputc(byte,out);
    }
    std::fclose(out);
}
static uint8_t tap(PrgGesture &button,int64_t at,int64_t length=100000){
    assert(button.update(at,true)==PrgGesture::None);
    return button.update(at+length,false);
}
static void gestures(){
    PrgGesture single;
    assert(tap(single,0)==PrgGesture::None);
    assert(single.update(499999,false)==PrgGesture::None);
    assert(single.update(500000,false)==PrgGesture::TogglePage);
    assert(single.update(900000,false)==PrgGesture::None);

    PrgGesture triple;
    assert(tap(triple,0)==PrgGesture::None);
    assert(tap(triple,200000)==PrgGesture::None);
    assert(tap(triple,400000)==PrgGesture::StartTimer);
    assert(triple.clicks==0);
    assert(triple.update(1000000,false)==PrgGesture::None);

    PrgGesture doubleClick;
    assert(tap(doubleClick,0)==PrgGesture::None);
    assert(tap(doubleClick,200000)==PrgGesture::None);
    assert(doubleClick.update(700000,false)==PrgGesture::None);
    assert(doubleClick.clicks==0);

    PrgGesture boundary;
    assert(tap(boundary,0)==PrgGesture::None);
    assert(boundary.update(500000,true)==PrgGesture::TogglePage);
    assert(boundary.update(600000,false)==PrgGesture::None);
    assert(boundary.update(1000000,false)==PrgGesture::TogglePage);

    PrgGesture noise;
    assert(tap(noise,0,29999)==PrgGesture::None);
    assert(noise.update(1000000,false)==PrgGesture::None);
    assert(tap(noise,1100000,30000)==PrgGesture::None);
    assert(noise.update(1530000,false)==PrgGesture::TogglePage);

    PrgGesture hold;
    assert(hold.update(0,true)==PrgGesture::None);
    assert(hold.update(2999999,true)==PrgGesture::None);
    assert(hold.update(3000000,true)==PrgGesture::OpenSetup);
    assert(hold.update(4000000,true)==PrgGesture::None);
    assert(hold.update(4100000,false)==PrgGesture::None);
    assert(hold.update(5000000,false)==PrgGesture::None);

    PrgGesture releasedAtThreshold;
    assert(releasedAtThreshold.update(0,true)==PrgGesture::None);
    assert(releasedAtThreshold.update(3000000,false)==PrgGesture::OpenSetup);

    PrgGesture clickThenHold;
    assert(tap(clickThenHold,0)==PrgGesture::None);
    assert(clickThenHold.update(200000,true)==PrgGesture::None);
    assert(clickThenHold.update(3200000,true)==PrgGesture::OpenSetup);
    assert(clickThenHold.update(3300000,false)==PrgGesture::None);
    assert(clickThenHold.clicks==0);
}
int main(int argc,char **argv){
    gestures();
    Countdown timer;
    constexpr int64_t start=INT64_C(4000000000000);
    assert(timer.view(start).phase==TimerPhase::Normal);
    timer.start(start);
    assert(timer.view(start).hundredths==3000);
    assert(timer.view(start+1).hundredths==3000);
    assert(timer.view(start+9999).hundredths==3000);
    assert(timer.view(start+10000).hundredths==2999);
    assert(timer.view(start+29999999).hundredths==1);
    assert(timer.view(start+30000000).phase==TimerPhase::Zero);
    assert(timer.view(start+30199999).phase==TimerPhase::Zero);
    assert(timer.view(start+30200000).phase==TimerPhase::Fireworks);
    assert(timer.view(start+34199999).phase==TimerPhase::Fireworks);
    assert(timer.view(start+34200000).phase==TimerPhase::Normal);
    // Steady above 10 seconds; four full on/off cycles per final second.
    assert(timer.view(start+14999999).visible);
    assert(timer.view(start+15000000).visible);
    assert(timer.view(start+15500000).visible);
    assert(timer.view(start+16000000).visible);
    assert(timer.view(start+19999999).visible);
    assert(timer.view(start+20000000).visible);
    assert(!timer.view(start+20125000).visible);
    assert(timer.view(start+20250000).visible);
    assert(timer.view(start+25000000).visible);
    assert(!timer.view(start+25125000).visible);
    assert(timer.view(start+25250000).visible);
    assert(timer.view(start+30000000).visible);
    // No off frames in the 15-to-10-second interval.
    for(int64_t elapsed=15000000;elapsed<20000000;elapsed+=1000)
        assert(timer.view(start+elapsed).visible);
    // Each complete final second contains exactly four falling/rising edges.
    for(int second=20;second<30;++second){
        int offEdges=0,onEdges=0;
        bool previous=timer.view(start+INT64_C(1000000)*second).visible;
        for(int ms=1;ms<=1000;++ms){
            bool visible=timer.view(start+INT64_C(1000000)*second+ms*1000).visible;
            if(previous&&!visible)++offEdges;
            if(!previous&&visible)++onEdges;
            previous=visible;
        }
        assert(offEdges==4&&onEdges==4);
    }
    // Delayed frames derive from elapsed microseconds, never frame count.
    assert(timer.view(start+12345678).hundredths==1766);
    timer.stop();assert(timer.view(start+1000).phase==TimerPhase::Normal);
    timer.start(start);

    char text[6];countdownTime(text,3000);assert(std::strcmp(text,"30:00")==0);
    countdownTime(text,1);assert(std::strcmp(text,"00:01")==0);
    countdownTime(text,0);assert(std::strcmp(text,"00:00")==0);
    ClockFrame frame;
    for(int value=0;value<=3000;++value){
        TimerView view;view.phase=TimerPhase::Counting;view.hundredths=value;
        renderCountdown(frame,view);assert(frame.clipped==0 && lit(frame)>0);
        for(int y=0;y<3;++y)for(int x=0;x<128;++x)assert(!frame.ink(x,y));
        view.visible=false;renderCountdown(frame,view);assert(lit(frame)==0);
    }
    int index=0,fireworkFrames=0;
    for(int64_t elapsed=0;elapsed<34200000;elapsed+=Countdown::FrameUs){
        const TimerView view=timer.view(start+elapsed);
        renderCountdown(frame,view);assert(frame.clipped==0);
        if(view.phase==TimerPhase::Fireworks){assert(lit(frame)>0);++fireworkFrames;}
        if(argc>1)saveFrame(frame,argv[1],index);
        ++index;
    }
    assert(index==855 && fireworkFrames==100);
    std::puts("countdown: gestures, microsecond deadlines, blink rates, 3001 numeral layouts and 100 firework frames passed");
}
