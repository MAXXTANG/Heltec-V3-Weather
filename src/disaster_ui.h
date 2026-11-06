#pragma once
#include "clock_ui.h"
#include "disaster_data.h"
#include "disaster_font.h"
#include "disaster_events.h"

inline uint32_t disasterNextChar(const char *&s){
    const unsigned char c=static_cast<unsigned char>(*s++);
    if(c<128)return c;
    int count=0;uint32_t cp=0;
    if(c>=0xc2&&c<=0xdf){count=1;cp=c&31;}
    else if(c>=0xe0&&c<=0xef){count=2;cp=c&15;}
    else if(c>=0xf0&&c<=0xf4){count=3;cp=c&7;}
    else return '?';
    for(int i=0;i<count;++i){
        if((static_cast<unsigned char>(*s)&0xc0)!=0x80)return '?';
        cp=(cp<<6)|(static_cast<unsigned char>(*s++)&63);
    }
    if((count==1&&cp<128)||(count==2&&cp<2048)||(count==3&&cp<65536)||
       cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return '?';
    return cp;
}
inline int disasterWidth(const char *s){
    int w=0;while(*s){uint32_t cp=disasterNextChar(s);w+=cp<127?6:12;}return w;
}
inline void disasterLine(ClockFrame &f,int y,const char *s,unsigned slice=0){
    // Whole-glyph pages prevent half a Chinese character at either screen edge.
    const char *p=s;int x=0;unsigned page=0;
    while(*p){
        const uint32_t cp=disasterNextChar(p);const int w=cp<127?6:12;
        if(x+w>128){x=0;++page;}
        if(page==slice){
            const uint8_t *glyph=disasterFontPixels+disasterGlyphIndex(cp)*24;
            for(int yy=0;yy<12;++yy)for(int xx=0;xx<w;++xx)
                if(glyph[yy*2+xx/8]&(0x80>>(xx%8)))f.pixel(x+xx,y+yy);
        }
        x+=w;
    }
}
inline unsigned disasterSlices(const char *s){
    unsigned pages=1;int x=0;
    while(*s){const uint32_t cp=disasterNextChar(s);const int w=cp<127?6:12;if(x+w>128){x=0;++pages;}x+=w;}
    return pages;
}
struct DisasterView {
    DisasterCase item;
    size_t index=0,count=0,total=0;
    bool valid=false,failed=false,stale=false,busy=false;
    bool notification=false;
    bool simulated=false;
    unsigned step=0;
    char checked[6]="--:--";
};
inline void renderDisasterPage(ClockFrame &f,const DisasterView &v){
    f={};char text[192];
    std::snprintf(text,sizeof(text),"%s %u/%u",v.simulated?"模擬測試":v.notification?"新案通知":"宜蘭災情",unsigned(v.count?v.index+1:0),unsigned(v.count));
    disasterLine(f,0,text);
    if(!v.valid){
        disasterLine(f,19,v.failed?"讀取失敗":"正在讀取");
        disasterLine(f,34,"尚無可用資料");
        disasterLine(f,52,v.busy?"連線中...":"每60秒重試");return;
    }
    if(!v.count){
        disasterLine(f,18,"網站目前無案件");
        disasterLine(f,34,"依消防局公開資料");
    }else{
        std::snprintf(text,sizeof(text),"%.10s %.5s %s",v.item.received,v.item.received+11,v.item.category);
        disasterLine(f,13,text,v.step%disasterSlices(text));
        const char *place=v.item.location;
        if(std::strncmp(place,"宜蘭縣",9)==0)place+=9;
        disasterLine(f,26,place,v.step%disasterSlices(place));
        // Alternate incident detail and dispatch units; the status remains visible.
        const char *detail=(v.step%2)?v.item.units:v.item.detail;
        disasterLine(f,39,detail,(v.step/2)%disasterSlices(detail));
    }
    // A failed check never replaces or relabels the last successful snapshot.
    const char *state=v.failed?"失敗舊":v.stale?"過期舊":v.busy?"查詢取":"取";
    std::snprintf(text,sizeof(text),"%s %s%s",v.count?v.item.status:"",state,v.checked);
    disasterLine(f,52,text,v.step%disasterSlices(text));
}
inline void renderDisasterNotification(ClockFrame &f,DisasterView v,const DisasterNavigation &nav,int64_t now){
    v.notification=nav.blinking(now);renderDisasterPage(f,v);
    if(nav.inverted(now))for(uint8_t &pixel:f.pixels)pixel^=0xff;
}
