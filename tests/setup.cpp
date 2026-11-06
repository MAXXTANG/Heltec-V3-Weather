#include <cassert>
#include <fstream>
#include <cstdio>
#include "../src/setup_ui.h"
void pbm(const char *path,const ClockFrame &f){
    std::ofstream out(path,std::ios::binary);out<<"P4\n128 64\n";
    for(int y=0;y<64;++y)for(int x=0;x<128;x+=8){unsigned char b=0;for(int i=0;i<8;++i)if(f.ink(x+i,y))b|=128>>i;out.put(b);}
}
int main(){
    ClockFrame f;
    for(const char *suffix:{"000000","111111","888888","AAAAAA","BBBBBB","CCCCCC","DDDDDD","EEEEEE","FFFFFF","A1B2C3"}){
        char ssid[24],password[20];std::snprintf(ssid,sizeof(ssid),"Heltec-V3-%s",suffix);std::snprintf(password,sizeof(password),"clock%s",suffix);
        renderSetupPage(f,ssid,password);assert(f.clipped==0);
    }
    renderSetupPage(f,"Heltec-V3-A1B2C3","clockA1B2C3");pbm("build/setup-readable.pbm",f);
    f={};clockText(f,64,3,"Wi-Fi setup",ui_font12,1);clockText(f,64,22,"Heltec-V3-A1B2C3",ui_font10,1);
    clockText(f,64,37,"PW clockA1B2C3",ui_font10,1);clockText(f,64,52,"192.168.4.1",ui_font10,1);pbm("build/setup-before.pbm",f);
    f={};clockText(f,64,2,"0O 1Il",readable_font18,1);assert(f.clipped==0);pbm("build/character-legibility.pbm",f);
    for(const char *line:{"Wi-Fi setup failed","Press RESET to retry","Setup timed out","Hold BOOT 3 sec","to set up Wi-Fi"}){
        f={};clockText(f,64,52,line,readableMessageFont(line),1);assert(f.clipped==0);
    }
    std::puts("Setup: 10 SSID/password samples fit; enlarged credentials, distinct character specimens rendered");
}
