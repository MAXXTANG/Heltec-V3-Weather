#pragma once
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>

static constexpr size_t DISASTER_LIMIT=8, DISASTER_BODY_LIMIT=65536;
struct DisasterCase {
    char sourceId[24]={};
    char received[20]={},category[49]={},detail[193]={},location[289]={},units[289]={},status[97]={};
};
struct DisasterList {
    DisasterCase cases[DISASTER_LIMIT];
    size_t count=0,total=0;
};
inline std::string disasterTrim(std::string s){
    const auto a=s.find_first_not_of(" \t\r\n"),b=s.find_last_not_of(" \t\r\n");
    return a==std::string::npos?"":s.substr(a,b-a+1);
}
inline std::string disasterLower(std::string s){
    for(char &c:s)c=static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return s;
}
inline void disasterUtf8(std::string &s,unsigned cp){
    if(cp<128)s+=char(cp);
    else if(cp<2048){s+=char(0xc0|(cp>>6));s+=char(0x80|(cp&63));}
    else if(cp<65536){s+=char(0xe0|(cp>>12));s+=char(0x80|((cp>>6)&63));s+=char(0x80|(cp&63));}
    else {s+=char(0xf0|(cp>>18));s+=char(0x80|((cp>>12)&63));s+=char(0x80|((cp>>6)&63));s+=char(0x80|(cp&63));}
}
inline bool disasterText(const std::string &html,std::string &out){
    out.clear();bool space=false;
    for(size_t i=0;i<html.size();++i){
        if(html[i]=='<'){
            const size_t end=html.find('>',i);if(end==std::string::npos)return false;
            i=end;continue;
        }
        if(html[i]=='&'){
            const size_t end=html.find(';',i);
            if(end!=std::string::npos && end-i<16){
                const std::string entity=html.substr(i+1,end-i-1);
                if(entity=="amp")out+='&';else if(entity=="lt")out+='<';
                else if(entity=="gt")out+='>';else if(entity=="quot")out+='"';
                else if(entity=="apos"||entity=="#39")out+='\'';
                else if(entity=="nbsp")out+=' ';
                else if(entity.size()>1 && entity[0]=='#'){
                    char *tail=nullptr;const bool hex=entity[1]=='x'||entity[1]=='X';
                    const unsigned long cp=std::strtoul(entity.c_str()+(hex?2:1),&tail,hex?16:10);
                    if(!tail||*tail||!cp||cp>0x10ffff||(cp>=0xd800&&cp<=0xdfff))return false;
                    if(cp==160)out+=' ';else disasterUtf8(out,static_cast<unsigned>(cp));
                }else return false;
                i=end;space=false;continue;
            }
        }
        const unsigned char c=html[i];
        if(c<128 && std::isspace(c)){if(!space)out+=' ';space=true;}
        else {out+=char(c);space=false;}
    }
    out=disasterTrim(out);return true;
}
template<size_t N> inline bool disasterCopy(char (&dst)[N],const std::string &s){
    if(s.empty()||s.size()>=N)return false;
    std::memcpy(dst,s.c_str(),s.size()+1);return true;
}
inline bool disasterDate(const std::string &s){
    if(s.size()!=19||s[4]!='/'||s[7]!='/'||s[10]!=' '||s[13]!=':'||s[16]!=':')return false;
    for(size_t i=0;i<s.size();++i)if(i!=4&&i!=7&&i!=10&&i!=13&&i!=16&&!std::isdigit(static_cast<unsigned char>(s[i])))return false;
    int y,m,d,h,n,t;if(std::sscanf(s.c_str(),"%d/%d/%d %d:%d:%d",&y,&m,&d,&h,&n,&t)!=6)return false;
    if(y<2000||y>2099||m<1||m>12||d<1||h>23||n>59||t>59)return false;
    const int days[]={31,28,31,30,31,30,31,31,30,31,30,31};
    return d<=days[m-1]+(m==2&&y%4==0&&(y%100!=0||y%400==0));
}
inline bool parseDisasterHtml(std::string html,DisasterList &out){
    out=DisasterList{};
    if(html.empty()||html.size()>DISASTER_BODY_LIMIT)return false;
    // The source includes a complete, commented-out demonstration table.
    for(size_t start;(start=html.find("<!--"))!=std::string::npos;){
        size_t end=html.find("-->",start+4);if(end==std::string::npos)return false;
        const std::string comment=disasterTrim(html.substr(start+4,end-start-4));
        std::string replacement;
        // Official rows carry a stable ID in a commented-out td. Keep only
        // that exact numeric form; whole commented demo rows remain excluded.
        if(comment.size()>7 && comment.compare(0,3,"td>")==0 &&
           comment.compare(comment.size()-4,4,"</td")==0){
            const std::string id=comment.substr(3,comment.size()-7);
            if(id.size()>=8 && id.size()<24 && std::all_of(id.begin(),id.end(),[](unsigned char c){return std::isdigit(c);}))
                replacement="<case-id>"+id+"</case-id>";
        }
        html.replace(start,end+3-start,replacement);
    }
    const std::string lower=disasterLower(html);
    size_t table=lower.find("<table"),begin=std::string::npos;
    for(;table!=std::string::npos;table=lower.find("<table",table+6)){
        const size_t end=lower.find('>',table);if(end==std::string::npos)return false;
        std::string attrs=lower.substr(table,end-table);
        attrs.erase(std::remove_if(attrs.begin(),attrs.end(),[](unsigned char c){return std::isspace(c);}),attrs.end());
        if(attrs.find("id=\"datatable\"")!=std::string::npos||attrs.find("id='datatable'")!=std::string::npos){begin=end+1;break;}
    }
    if(begin==std::string::npos)return false;
    const size_t end=lower.find("</table>",begin);if(end==std::string::npos)return false;
    bool header=false;
    for(size_t row=lower.find("<tr",begin);row<end;row=lower.find("<tr",row+4)){
        const size_t close=lower.find("</tr>",row);if(close==std::string::npos||close>end)return false;
        const bool isHeader=lower.find("<th",row)<close;
        const char *tag=isHeader?"<th":"<td",*closing=isHeader?"</th>":"</td>";
        std::vector<std::string> cells;
        for(size_t cell=lower.find(tag,row);cell<close;cell=lower.find(tag,cell+3)){
            const size_t a=lower.find('>',cell),b=lower.find(closing,a);
            if(a>=close||b>=close)return false;
            std::string text;if(!disasterText(html.substr(a+1,b-a-1),text))return false;
            cells.push_back(text);
        }
        if(cells.empty())continue;
        if(cells.size()!=7)return false;
        if(isHeader){
            const char *expected[]={"序號","受理時間","案類","案別","發生地點","派遣分隊","執行狀況"};
            for(size_t i=0;i<7;++i)if(cells[i]!=expected[i])return false;
            header=true;continue;
        }
        if(!header||!disasterDate(cells[1]))return false;
        DisasterCase item;
        const size_t idStart=lower.find("<case-id>",row);
        if(idStart<close){
            const size_t idEnd=lower.find("</case-id>",idStart);
            if(idEnd>=close || !disasterCopy(item.sourceId,html.substr(idStart+9,idEnd-idStart-9)))return false;
        }
        if(!disasterCopy(item.received,cells[1])||!disasterCopy(item.category,cells[2])||
           !disasterCopy(item.detail,cells[3])||!disasterCopy(item.location,cells[4])||
           !disasterCopy(item.units,cells[5])||!disasterCopy(item.status,cells[6]))return false;
        ++out.total;
        // Keep the eight newest records even when the website order changes.
        size_t pos=0;while(pos<out.count && std::strcmp(out.cases[pos].received,item.received)>=0)++pos;
        if(pos<DISASTER_LIMIT){
            if(out.count<DISASTER_LIMIT)++out.count;
            for(size_t i=out.count-1;i>pos;--i)out.cases[i]=out.cases[i-1];
            out.cases[pos]=item;
        }
        row=close;
    }
    return header;
}
