#pragma once
#include "clock_ui.h"
#include "countdown.h"

inline void countdownTime(char (&text)[6],int hundredths){
    if(hundredths<0)hundredths=0;
    if(hundredths>3000)hundredths=3000;
    const int seconds=hundredths/100, fraction=hundredths%100;
    text[0]='0'+seconds/10;text[1]='0'+seconds%10;text[2]=':';
    text[3]='0'+fraction/10;text[4]='0'+fraction%10;text[5]=0;
}
inline void sparkPixel(ClockFrame &f,int x,int y){
    // Particles naturally leave the viewport; only visible pixels are drawn.
    if(x>=0 && x<128 && y>=0 && y<64)f.pixel(x,y);
}
inline void sparkLine(ClockFrame &f,int x0,int y0,int x1,int y1){
    const int dx=x1>x0?x1-x0:x0-x1, sx=x0<x1?1:-1;
    const int dy=-(y1>y0?y1-y0:y0-y1), sy=y0<y1?1:-1;
    int error=dx+dy;
    for(;;){
        sparkPixel(f,x0,y0);
        if(x0==x1 && y0==y1)break;
        const int twice=2*error;
        if(twice>=dy){error+=dy;x0+=sx;}
        if(twice<=dx){error+=dx;y0+=sy;}
    }
}
inline void renderFireworks(ClockFrame &f,int64_t elapsedUs){
    struct Burst {int startMs,x,y,radius;};
    static const Burst bursts[]={
        {0,26,24,23},{550,94,27,26},{1100,60,19,25},
        {1650,30,35,24},{2200,101,34,25},{2750,64,25,28}
    };
    static const int16_t dx[]={256,237,181,98,0,-98,-181,-237,-256,-237,-181,-98,0,98,181,237};
    static const int16_t dy[]={0,98,181,237,256,237,181,98,0,-98,-181,-237,-256,-237,-181,-98};
    const int ms=(int)(elapsedUs/1000);
    for(int b=0;b<6;++b){
        const auto &burst=bursts[b];const int age=ms-burst.startMs;
        if(age<0 || age>=1250)continue;
        if(age<300){
            const int y=63-(63-burst.y)*age/300;
            sparkPixel(f,burst.x,y);sparkPixel(f,burst.x,y+1);
            for(int k=1;k<=3;++k)
                if(((age/40)+k)%3)sparkPixel(f,burst.x+(k%2?1:-1),y+2+k*2);
            continue;
        }
        const int t=age-300;
        const int radius=2+burst.radius*t/(t+250);
        const int fall=t*t/160000;
        const int trail=t<650?3:1;
        for(int ray=0;ray<16;++ray){
            if(t>500 && ((t/40+ray*3+b)%5)<(t-500)/140)continue;
            const int rr=radius-(ray%3);
            const int x=burst.x+dx[ray]*rr/256;
            const int y=burst.y+dy[ray]*rr/256+fall;
            sparkLine(f,burst.x+dx[ray]*(rr-trail)/256,
                      burst.y+dy[ray]*(rr-trail)/256+fall,x,y);
            if(t<180 && ray%4==0){sparkPixel(f,x+1,y);sparkPixel(f,x,y+1);}
        }
        if(t<120){
            sparkLine(f,burst.x-2,burst.y,burst.x+2,burst.y);
            sparkLine(f,burst.x,burst.y-2,burst.x,burst.y+2);
        }
    }
}
inline void renderCountdown(ClockFrame &f,const TimerView &view){
    f={};
    if(view.phase==TimerPhase::Fireworks){renderFireworks(f,view.effectUs);return;}
    if(view.phase==TimerPhase::Normal || !view.visible)return;
    char text[6];countdownTime(text,view.hundredths);
    clockDigitSized(f,1,3,text[0],26,58,4);
    clockDigitSized(f,31,3,text[1],26,58,4);
    f.box(62,21,4,4);f.box(62,39,4,4);
    clockDigitSized(f,71,3,text[3],26,58,4);
    clockDigitSized(f,101,3,text[4],26,58,4);
}
