#pragma once
#include "clock_ui.h"

inline int readableWidth(const char *s,const UiFont &font){
    int pen=0,left=10000,right=-10000;
    for(;*s;++s)if(*s>=32&&*s<=126){
        const UiGlyph &g=font.glyphs[*s-32];
        if(g.w){const int x=(pen+32)/64+g.left;if(x<left)left=x;if(x+g.w>right)right=x+g.w;}
        pen+=g.advance64;
    }
    return right<left?0:right-left;
}
inline const UiFont &readableMessageFont(const char *s){
    if(readableWidth(s,readable_font12)<=124)return readable_font12;
    if(readableWidth(s,readable_font11)<=124)return readable_font11;
    if(readableWidth(s,readable_font10)<=124)return readable_font10;
    return readable_font9;
}

inline void renderSetupPage(ClockFrame &f,const char *ssid,const char *password){
    f={};
    clockText(f,2,0,"CONNECT WI-FI",readable_font10);
    clockText(f,64,12,ssid,readable_font13,1);
    clockText(f,2,27,"PASSWORD",readable_font10);
    clockText(f,64,38,password,readable_font18,1);
    clockText(f,64,57,"192.168.4.1",readable_font10,1);
}
