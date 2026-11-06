#include <cassert>
#include <fstream>
#include <iostream>
#include <iterator>
#include "../src/disaster_ui.h"

std::string readFile(const char *path){std::ifstream in(path);assert(in);return {std::istreambuf_iterator<char>(in),{}};}
void pbm(const char *path,const ClockFrame &f){
    std::ofstream out(path,std::ios::binary);out<<"P4\n128 64\n";
    for(int y=0;y<64;++y)for(int x=0;x<128;x+=8){unsigned char b=0;for(int i=0;i<8;++i)if(f.ink(x+i,y))b|=128>>i;out.put(b);}
}
int main(){
    const std::string fixture=readFile("tests/fixtures/yilan-cases.html");
    DisasterList list;assert(parseDisasterHtml(fixture,list));
    assert(list.count==3 && list.total==3);
    assert(std::string(list.cases[0].location)=="宜蘭縣礁溪鄉武暖路");
    assert(std::string(list.cases[0].status)=="返隊中");
    assert(std::string(list.cases[2].received)=="2026/10/24 00:50:42");
    // An empty official table is a valid snapshot; an error page is not.
    std::string empty=fixture.substr(0,fixture.find("<tr>",fixture.find("</tr>")+5))+"</table></html>";
    assert(parseDisasterHtml(empty,list) && list.count==0);
    assert(!parseDisasterHtml("<html>Maintenance</html>",list));
    assert(!parseDisasterHtml(fixture.substr(0,fixture.find("</table>")),list));
    assert(!parseDisasterHtml(fixture+"<!-- unfinished",list));
    std::string bad=fixture;bad.replace(bad.find("執行狀況"),12,"新欄位");assert(!parseDisasterHtml(bad,list));
    bad=fixture;bad.replace(bad.find("2026/11/05"),10,"2026/02/30");assert(!parseDisasterHtml(bad,list));
    assert(!parseDisasterHtml(std::string(DISASTER_BODY_LIMIT+1,'x'),list));
    std::string decoded;
    assert(disasterText(" <span>礁溪</span>&nbsp;&#x鄉;",decoded)==false);
    assert(disasterText(" <span>礁溪</span>&nbsp;&#37129;&amp;路 ",decoded));
    assert(decoded=="礁溪 鄉&路");
    assert(disasterDate("2024/02/29 23:59:59")&&!disasterDate("2025/02/29 23:59:59"));
    // Bound the cache while keeping the newest records from any source order.
    std::string many=empty.substr(0,empty.find("</table>"));
    for(int i=1;i<=12;++i){char row[800];std::snprintf(row,sizeof(row),"<tr><td>%d</td><td>2026/11/%02d 00:00:00</td><td>火災</td><td>住宅</td><td>宜蘭縣礁溪鄉</td><td>礁溪分隊</td><td>已返隊</td></tr>",i,i);many+=row;}
    many+="</table>";assert(parseDisasterHtml(many,list));
    assert(list.count==8 && list.total==12 && std::string(list.cases[0].received)=="2026/11/12 00:00:00");
    assert(std::string(list.cases[7].received)=="2026/11/05 00:00:00");
    // All real rows and whole-character text segments must fit 128 x 64.
    assert(parseDisasterHtml(fixture,list));
    for(size_t i=0;i<list.count;++i)for(unsigned step=0;step<30;++step){
        DisasterView v;v.item=list.cases[i];v.index=i;v.count=list.count;v.valid=true;v.step=step;
        std::strcpy(v.checked,"23:36");ClockFrame frame;renderDisasterPage(frame,v);assert(frame.clipped==0);
        if(i==0&&step==0)pbm("build/disaster.pbm",frame);
        if(i==0&&step==1)pbm("build/disaster-units.pbm",frame);
        if(i==0&&step==0){v.failed=true;renderDisasterPage(frame,v);assert(frame.clipped==0);pbm("build/disaster-failed.pbm",frame);}
    }
    for(int state=0;state<4;++state){
        DisasterView v;v.valid=state>=2;v.failed=state==1;ClockFrame frame;renderDisasterPage(frame,v);assert(frame.clipped==0);
    }
    const char *malformed="\xff\xc0\xe1";
    while(*malformed)assert(disasterNextChar(malformed)=='?');
    std::cout<<"Disaster parser: live fixture, commented examples, malformed/empty HTML, entities, dates, bounded sorting; 90 renderer frames passed\n";
}
