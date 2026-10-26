#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include "ui_fonts.h"
#include "weather_config.h"
#include "weather_icons.h"

// SSD1306 page order: one byte contains eight vertical white pixels.
struct ClockFrame {
    uint8_t pixels[1024] = {};
    int clipped = 0;
    void pixel(int x, int y) {
        if (x<0 || x>=128 || y<0 || y>=64) { ++clipped; return; }
        pixels[x+(y/8)*128] |= 1<<(y%8);
    }
    bool ink(int x,int y) const { return pixels[x+(y/8)*128] & (1<<(y%8)); }
    void box(int x,int y,int w,int h) {
        for(int j=0;j<h;++j) for(int i=0;i<w;++i) pixel(x+i,y+j);
    }
    void bitmap(int x,int y,int w,int h,const uint8_t *data) {
        for(int j=0;j<h;++j) for(int i=0;i<w;++i)
            if(data[j*((w+7)/8)+i/8] & (0x80>>(i%8))) pixel(x+i,y+j);
    }
};
inline void clockText(ClockFrame &f,int x,int y,const char *s,const UiFont &font,int align=0,int tracking=0) {
    int pen=0,left=10000,top=10000,right=-10000;
    for(const char *p=s;*p;++p) if(*p>=32 && *p<=126) {
        const auto &g=font.glyphs[*p-32];
        if(g.w && g.h) {
            int xx=(pen+32)/64+g.left;
            if(xx<left)left=xx;
            if(g.top<top)top=g.top;
            if(xx+g.w>right)right=xx+g.w;
        }
        pen+=g.advance64+tracking*64;
    }
    if(left==10000)return;
    if(align==1)x-=(right-left)/2;
    if(align==2)x-=right-left;
    pen=(x-left)*64;
    for(const char *p=s;*p;++p) if(*p>=32 && *p<=126) {
        const auto &g=font.glyphs[*p-32];
        f.bitmap((pen+32)/64+g.left,y+g.top-top,g.w,g.h,font.pixels+g.offset);
        pen+=g.advance64+tracking*64;
    }
}
// Draw real seven-segment numerals at the target resolution (no bitmap stretch).
inline void clockDigitSized(ClockFrame &f,int x,int y,char digit,int w,int h,int stroke) {
    static const uint8_t masks[]={0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
    const uint8_t m=(digit>='0' && digit<='9')?masks[digit-'0']:0x40;
    const int middle=(h-stroke)/2;
    auto horizontal=[&](int yy) {
        f.box(x+stroke,yy,w-2*stroke,stroke);
        if(stroke>1)f.box(x+stroke-1,yy+1,w-2*stroke+2,stroke-2?stroke-2:1);
    };
    auto vertical=[&](int xx,int yy,int height) {
        if(stroke==1){f.box(xx,yy,1,height);return;}
        f.box(xx,yy+1,stroke,height-2);
        f.box(xx+1,yy,stroke>2?stroke-2:1,height);
    };
    if(m&1)horizontal(y);
    if(m&2)vertical(x+w-stroke,y+stroke,middle-stroke);
    if(m&4)vertical(x+w-stroke,y+middle+stroke,h-middle-2*stroke);
    if(m&8)horizontal(y+h-stroke);
    if(m&16)vertical(x,y+middle+stroke,h-middle-2*stroke);
    if(m&32)vertical(x,y+stroke,middle-stroke);
    if(m&64)horizontal(y+middle);
}
inline void clockDigit(ClockFrame &f,int x,int y,char digit) {
    clockDigitSized(f,x,y,digit,18,28,3);
}
inline int clockNumberWidth(const char *s,int digitWidth,int gap) {
    int width=0;
    for(const char *p=s;*p;++p)width+=((*p=='.'||*p=='\'')?3:digitWidth)+gap;
    return width?width-gap:0;
}
inline void clockNumber(ClockFrame &f,int x,int y,const char *s,int w,int h,int stroke,int gap) {
    for(;*s;++s) {
        if(*s=='.'){f.box(x,y+h-3,3,3);x+=3+gap;}
        else if(*s=='\''){f.box(x,y,2,4);x+=3+gap;}
        else {clockDigitSized(f,x,y,*s,w,h,stroke);x+=w+gap;}
    }
}
inline void clockWeatherIcon(ClockFrame &f,int x,int y,const WeatherIcon &icon) {
    f.bitmap(x,y,WEATHER_ICON_WIDTH,WEATHER_ICON_HEIGHT,icon.pixels);
}
inline void clockDegree(ClockFrame &f,int x,int y) {
    // A hollow degree symbol, independent of the ASCII-only number renderer.
    static const uint8_t ring[]={0x38,0x6c,0xc6,0xc6,0xc6,0x6c,0x38};
    f.bitmap(x,y,7,7,ring);
}
inline const char *conditionName(int code) {
    if(code==0)return "Clear"; if(code==1)return "Mostly clear";
    if(code==2)return "Cloudy"; if(code==3)return "Overcast";
    if(code==45 || code==48)return "Fog";
    if(code>=51 && code<=57)return "Drizzle";
    if(code>=61 && code<=67)return "Rain";
    if(code>=71 && code<=77)return "Snow";
    if(code>=80 && code<=82)return "Showers";
    if(code==85 || code==86)return "Snow showers";
    if(code>=95)return "Thunderstorm";
    return "Weather";
}
inline const WeatherIcon &conditionIcon(int code,bool day) {
    if(code==0)return day?weather_sun:weather_moon;
    if(code==1 || code==2)return day?weather_partly:weather_partly_night;
    if(code==45 || code==48)return weather_fog;
    if(code>=51 && code<=57)return weather_drizzle;
    if(code>=61 && code<=67)return weather_rain;
    if(code>=80 && code<=82)return weather_showers;
    if((code>=71 && code<=77)||code==85||code==86)return weather_snow;
    if(code>=95)return weather_storm;
    return weather_cloud;
}
struct ClockView {
    char time[6]="--:--",date[16]="--- --/--";
    float temperature=0;
    int humidity=0,code=0,batteryLevel=-1;
    bool valid=false,day=true,stale=false;
};
inline void clockBattery(ClockFrame &f,int level) {
    const int x=2,y=1;
    f.box(x,y,15,1);f.box(x,y+7,15,1);
    f.box(x,y+1,1,6);f.box(x+14,y+1,1,6);f.box(x+15,y+2,1,4);
    if(level<0) {
        const uint8_t question[]={0x60,0x10,0x20,0,0x20};
        f.bitmap(x+5,y+2,4,5,question);
    } else if(level==0) {
        f.box(x+7,y+2,1,2);f.pixel(x+7,y+5);
    } else {
        for(int i=0;i<level && i<4;++i)f.box(x+2+i*3,y+2,2,4);
    }
}
inline void renderClockHeader(ClockFrame &f,const ClockView &v) {
    clockText(f,23,1,WEATHER_LOCATION,ui_font11,0,1);
    clockBattery(f,v.batteryLevel);
    clockText(f,126,1,v.date,ui_font11,2,1);
}
inline void clockHumidity(ClockFrame &f,const char *humidity) {
    // Center the visible numerals and percent sign together, including values
    // beginning with a narrow 1, on the same 44px column as the weather icon.
    ClockFrame label;
    const int height=13;
    const int hw=clockNumberWidth(humidity,8,2);
    clockNumber(label,0,0,humidity,8,height,1,2);
    clockText(label,hw+2,height-ui_font13.glyphs['%'-32].h,"%",ui_font13);
    int left=44,right=-1;
    for(int y=0;y<height;++y)for(int x=0;x<44;++x)if(label.ink(x,y)) {
        if(x<left)left=x;if(x>right)right=x;
    }
    if(right<left)return;
    const int offset=(44-(right-left+1))/2-left;
    for(int y=0;y<height;++y)for(int x=left;x<=right;++x)
        if(label.ink(x,y))f.pixel(x+offset,50+y);
}
inline void renderClockPage(ClockFrame &f,const ClockView &v,bool weatherPage) {
    f={};
    // These exact pixels are shared by both pages; the top row never moves.
    renderClockHeader(f,v);
    if(!weatherPage) {
        clockDigitSized(f,1,13,v.time[0],26,50,4);
        clockDigitSized(f,31,13,v.time[1],26,50,4);
        f.box(62,27,4,4);f.box(62,45,4,4);
        clockDigitSized(f,71,13,v.time[3],26,50,4);
        clockDigitSized(f,101,13,v.time[4],26,50,4);
        return;
    }
    char temp[16],humidity[8];
    if(v.valid) {
        std::snprintf(temp,sizeof(temp),"%.1f",v.temperature);
        std::snprintf(humidity,sizeof(humidity),"%d",v.humidity);
        clockWeatherIcon(f,0,12,conditionIcon(v.code,v.day));
    } else {
        std::strcpy(temp,"--.-");std::strcpy(humidity,"--");
        clockText(f,22,27,"?",ui_font18,1);
    }
    // The right column is 80 px wide, including a 3 px gap and 7 px degree ring.
    const int digitWidth=clockNumberWidth(temp,17,3)+10>80?13:17;
    const int tw=clockNumberWidth(temp,digitWidth,3);
    const int tx=86-(tw+10)/2;
    clockNumber(f,tx,23,temp,digitWidth,30,3,3);
    clockDegree(f,tx+tw+3,23);
    clockHumidity(f,humidity);
    if(v.stale)clockText(f,124,55,"!",ui_font10);
}
inline void renderClock(ClockFrame &f,const ClockView &v) {
    renderClockPage(f,v,false);
}
