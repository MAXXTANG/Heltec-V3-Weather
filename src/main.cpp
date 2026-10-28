#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <time.h>
#include <sys/time.h>
#include <cmath>
#include <esp_sleep.h>
#include <esp_sntp.h>
#include <esp_timer.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include "clock_ui.h"
#include "countdown_ui.h"
#include "network_schedule.h"
#include "battery_status.h"
#include "weather_ca.h"
#include "weather_config.h"

static constexpr uint32_t MAGIC=0x56334332;
static constexpr int OLED_SDA=17,OLED_SCL=18,OLED_RESET=21,EXT_POWER=36,STATUS_LED=35,BUTTON=0;
static constexpr int BATTERY_ADC=1,BATTERY_CTRL=37;
static constexpr int RADIO_CS=8,RADIO_SCK=9,RADIO_MOSI=10,RADIO_MISO=11,RADIO_RST=12,RADIO_BUSY=13;
static constexpr uint8_t OLED_ADDR=0x3c;
static constexpr int HELD_PINS[]={EXT_POWER,OLED_RESET,STATUS_LED,RADIO_CS,RADIO_RST,RADIO_SCK,RADIO_MOSI,BATTERY_CTRL};
struct WeatherCache {
    uint32_t magic;
    float temperature;
    int humidity,code;
    bool valid,day;
    time_t fetched,observed,nextNetwork;
    uint32_t wakes;
};
RTC_DATA_ATTR WeatherCache cached={};
ClockFrame frame;
bool oledReady=false;
String savedSsid,savedPass;
portMUX_TYPE weatherMux=portMUX_INITIALIZER_UNLOCKED;
TaskHandle_t networkTask=nullptr;
bool networkBusy=false;
uint32_t networkGeneration=0;
bool weatherPage=false,setupRequested=false;
PrgGesture prg;
Countdown countdown;
int64_t nextTimerFrameUs=0;
TimerPhase lastTimerPhase=TimerPhase::Normal;
int lastCountdownSecond=-1;
uint32_t timerFrames=0,timerFailures=0;
time_t shownMinute=-1;
uint32_t seenGeneration=0;
uint8_t dumpPages=3;
int batteryEnable=-1,batteryMv=0,batteryLevel=-1;
int64_t nextBatteryUs=0;

WeatherCache cacheSnapshot(bool *busy=nullptr,uint32_t *generation=nullptr){
    portENTER_CRITICAL(&weatherMux);
    WeatherCache snapshot=cached;
    if(busy)*busy=networkBusy;
    if(generation)*generation=networkGeneration;
    portEXIT_CRITICAL(&weatherMux);
    return snapshot;
}

bool clockValid(){return time(nullptr)>1704067200 && time(nullptr)<4102444800LL;}
void outputLevel(int pin,int level){digitalWrite(pin,level);pinMode(pin,OUTPUT);}
void restoreOutputs(){
    // Configure the output latch before releasing a retained pin, avoiding OLED resets.
    outputLevel(EXT_POWER,LOW);outputLevel(OLED_RESET,HIGH);outputLevel(STATUS_LED,LOW);
    outputLevel(RADIO_CS,HIGH);outputLevel(RADIO_RST,HIGH);
    outputLevel(RADIO_SCK,LOW);outputLevel(RADIO_MOSI,LOW);
    for(int pin:HELD_PINS)gpio_hold_dis((gpio_num_t)pin);
    gpio_deep_sleep_hold_dis();
    rtc_gpio_deinit((gpio_num_t)BUTTON);
    pinMode(BUTTON,INPUT_PULLUP);
}
int sampleBatteryAt(int level){
    outputLevel(BATTERY_CTRL,level);delay(20);
    analogReadMilliVolts(BATTERY_ADC); // Discard the first conversion after switching.
    uint32_t total=0,smallest=UINT32_MAX,largest=0;
    for(int i=0;i<16;++i){
        uint32_t mv=analogReadMilliVolts(BATTERY_ADC);total+=mv;
        if(mv<smallest)smallest=mv;if(mv>largest)largest=mv;
        delay(1);
    }
    // Trim two outliers; calibrated ADC millivolts times (390k+100k)/100k.
    return ((total-smallest-largest)*49+70)/140;
}
void readBattery(){
    if(batteryEnable<0){batteryLevel=-1;return;}
    batteryMv=sampleBatteryAt(batteryEnable);
    digitalWrite(BATTERY_CTRL,!batteryEnable);
    batteryLevel=batterySegments(batteryMv,batteryLevel);
    nextBatteryUs=esp_timer_get_time()+60000000;
    Serial.printf("BATTERY mv=%d segments=%d low=%d valid=%d ctrl_enable=%d\n",
        batteryMv,batteryLevel,batteryLevel==0,batteryLevel>=0,batteryEnable);
}
void startBattery(){
    pinMode(BATTERY_ADC,INPUT);analogReadResolution(12);
    analogSetPinAttenuation(BATTERY_ADC,ADC_0db);
    const int lowMv=sampleBatteryAt(LOW),highMv=sampleBatteryAt(HIGH);
    batteryEnable=batteryControlLevel(lowMv,highMv);
    Serial.printf("BATTERY_PROBE low_mv=%d high_mv=%d enable=%d\n",lowMv,highMv,batteryEnable);
    if(batteryEnable<0){
        pinMode(BATTERY_CTRL,INPUT);Serial.println("BATTERY_UNAVAILABLE");return;
    }
    readBattery();
}
bool oledCommands(const uint8_t *data,size_t count){
    Wire.beginTransmission(OLED_ADDR);Wire.write(0);Wire.write(data,count);
    return Wire.endTransmission()==0;
}
bool startOled(bool warm){
    if(!warm){digitalWrite(OLED_RESET,LOW);delay(10);digitalWrite(OLED_RESET,HIGH);delay(20);}
    Wire.begin(OLED_SDA,OLED_SCL,400000);Wire.setTimeOut(100);
    Wire.beginTransmission(OLED_ADDR);
    if(Wire.endTransmission()!=0){Serial.println("OLED_NOT_FOUND");return false;}
    if(!warm){
        const uint8_t init[]={0xae,0xd5,0x80,0xa8,0x3f,0xd3,0,0x40,0x8d,0x14,0x20,0,
                              0xa1,0xc8,0xda,0x12,0x81,0x02,0xd9,0xf1,0xdb,0x40,0xa4,0xa6,0xaf};
        if(!oledCommands(init,sizeof(init)))return false;
    }
    Serial.printf("OLED_READY retained=%d\n",warm);
    return true;
}
bool flushOled(bool diagnostic=true){
    if(!oledReady)return false;
    tm local={};time_t now=time(nullptr);localtime_r(&now,&local);
    const uint8_t level[]={0x81,(uint8_t)((clockValid() && (local.tm_hour>=22||local.tm_hour<7))?0:2)};
    const uint8_t range[]={0x21,0,127,0x22,0,7};
    bool ok=oledCommands(level,sizeof(level)) && oledCommands(range,sizeof(range));
    for(size_t i=0;ok && i<sizeof(frame.pixels);i+=16){
        Wire.beginTransmission(OLED_ADDR);Wire.write(0x40);Wire.write(frame.pixels+i,16);
        ok=Wire.endTransmission()==0;
    }
    if(diagnostic || !ok || frame.clipped)Serial.printf("OLED_FRAME ok=%d clipped=%d\n",ok,frame.clipped);
    return ok;
}
void dumpFrame(){
    // One UART write prevents network-task diagnostics from splitting the snapshot.
    char line[sizeof(frame.pixels)*2+12];
    const char hex[]="0123456789abcdef";std::memcpy(line,"FRAME_HEX=",10);
    size_t pos=10;
    for(uint8_t b:frame.pixels){line[pos++]=hex[b>>4];line[pos++]=hex[b&15];}
    line[pos++]='\n';line[pos]=0;Serial.write((const uint8_t *)line,pos);
}
void message(const char *a,const char *b="",const char *c="",const char *d=""){
    frame={};clockText(frame,64,3,a,ui_font12,1);clockText(frame,64,22,b,ui_font10,1);
    clockText(frame,64,37,c,ui_font10,1);clockText(frame,64,52,d,ui_font10,1);flushOled();
}
void sleepRadio(){
    pinMode(RADIO_BUSY,INPUT);
    digitalWrite(RADIO_RST,LOW);delay(2);digitalWrite(RADIO_RST,HIGH);delay(10);
    uint32_t started=millis();
    while(digitalRead(RADIO_BUSY) && millis()-started<500)delay(1);
    if(digitalRead(RADIO_BUSY)){Serial.println("RADIO_BUSY_TIMEOUT");return;}
    SPI.begin(RADIO_SCK,RADIO_MISO,RADIO_MOSI,RADIO_CS);
    SPI.beginTransaction(SPISettings(1000000,MSBFIRST,SPI_MODE0));
    digitalWrite(RADIO_CS,LOW);SPI.transfer(0x84);SPI.transfer(0x00);digitalWrite(RADIO_CS,HIGH);
    SPI.endTransaction();delay(2);SPI.end();
    outputLevel(RADIO_CS,HIGH);outputLevel(RADIO_SCK,LOW);outputLevel(RADIO_MOSI,LOW);
    Serial.println("RADIO_SLEEP command=0x84 cold=1");
}
void stopWifi(){
    WiFi.setAutoReconnect(false);
    WiFi.disconnect(true,false);WiFi.mode(WIFI_OFF);
    Serial.println("WIFI_OFF");
}
void deepSleep(bool forever=false){
    // Avoid an immediate wake loop when BOOT is still pressed.
    uint32_t start=millis();
    while(digitalRead(BUTTON)==LOW && millis()-start<10000)delay(10);
    bool buttonReleased=digitalRead(BUTTON)==HIGH;
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    if(buttonReleased){
        esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON,0);
        rtc_gpio_pullup_en((gpio_num_t)BUTTON);rtc_gpio_pulldown_dis((gpio_num_t)BUTTON);
    }
    timeval tv;gettimeofday(&tv,nullptr);
    uint64_t sleepUs=60000000ULL;
    if(clockValid())sleepUs=(60-(tv.tv_sec%60))*1000000ULL-tv.tv_usec;
    if(!forever || !buttonReleased)esp_sleep_enable_timer_wakeup(sleepUs);
    if(!oledReady)cached.magic=0; // Reinitialize the controller after any failed transfer.
    for(int pin:HELD_PINS)gpio_hold_en((gpio_num_t)pin);
    gpio_deep_sleep_hold_en();
    Serial.printf("SLEEP_PLAN seconds=%.3f button=%d forever=%d cpuMHz=%u wifiMode=%d\n",
        sleepUs/1000000.0,buttonReleased,forever,getCpuFrequencyMhz(),(int)WiFi.getMode());
    Serial.flush();esp_deep_sleep_start();
}
void readSettings(){
    Preferences prefs;prefs.begin("v3-clock",true);
    savedSsid=prefs.getString("ssid","");savedPass=prefs.getString("password","");prefs.end();
}
void provision(){
    WiFi.persistent(false);WiFi.mode(WIFI_AP);
    uint64_t id=ESP.getEfuseMac();
    // Derive the suffix from the final three bytes in the displayed MAC order.
    char suffix[7];snprintf(suffix,sizeof(suffix),"%02X%02X%02X",(uint8_t)(id>>24),(uint8_t)(id>>32),(uint8_t)(id>>40));
    String ap="Heltec-V3-"+String(suffix),password="clock"+String(suffix);
    if(!WiFi.softAP(ap.c_str(),password.c_str())){message("Wi-Fi setup failed","Press RESET to retry");stopWifi();deepSleep(true);}
    DNSServer dns;dns.start(53,"*",WiFi.softAPIP());WebServer server(80);
    bool restart=false;
    const char *html=R"HTML(<!doctype html><html lang="zh-Hant"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1"><title>Heltec V3 Clock</title><style>body{font:18px system-ui;max-width:480px;margin:30px auto;padding:16px;background:#f3f4f5}input,button{box-sizing:border-box;width:100%;padding:14px;margin:8px 0}button{background:#152535;color:white;border:0;border-radius:8px}</style><h1>Heltec V3 時鐘</h1><p>網路氣象 · 七段式時鐘</p><form method="post" action="/save"><label>家中 2.4 GHz Wi-Fi 名稱<input name="ssid" maxlength="32" required autocomplete="off"></label><label>Wi-Fi 密碼<input name="password" type="password" maxlength="63" autocomplete="new-password"></label><button>儲存並連線</button></form><p>短按 PRG／BOOT 切換頁面，連按三下啟動 30 秒倒數；每 15 分鐘更新天氣。長按 PRG／BOOT 3 秒可重新配網。</p><p>Weather data by <a href="https://open-meteo.com/">Open-Meteo.com</a> · <a href="https://creativecommons.org/licenses/by/4.0/">CC BY 4.0</a></p></html>)HTML";
    server.on("/",HTTP_GET,[&](){server.send(200,"text/html; charset=utf-8",html);});
    server.on("/save",HTTP_POST,[&](){
        String ssid=server.arg("ssid"),pass=server.arg("password");
        if(ssid.length()<1||ssid.length()>32||pass.length()>63||(pass.length()>0&&pass.length()<8)){
            server.send(400,"text/plain; charset=utf-8","請檢查 Wi-Fi 名稱與密碼長度。密碼至少 8 字元，開放網路可留空。");return;
        }
        Preferences prefs;
        bool ok=prefs.begin("v3-clock",false);
        if(ok){ok=prefs.putString("ssid",ssid)>0;prefs.putString("password",pass);ok=prefs.isKey("password") && prefs.getString("password")==pass && ok;prefs.end();}
        if(!ok){server.send(500,"text/plain","Settings write failed; retry.");return;}
        server.send(200,"text/html; charset=utf-8","<meta name=viewport content='width=device-width,initial-scale=1'><h2>已儲存，裝置正在重新啟動。</h2><p>請看 OLED 畫面；若無法連線，長按 BOOT 3 秒重新設定。</p>");restart=true;
    });
    server.onNotFound([&](){server.sendHeader("Location","http://192.168.4.1/",true);server.send(302,"text/plain","");});
    server.begin();
    String pw="PW "+password;
    message("Wi-Fi setup",ap.c_str(),pw.c_str(),"192.168.4.1");
    Serial.printf("SETUP_AP=%s PASSWORD=%s URL=http://192.168.4.1 timeout=600s\n",ap.c_str(),password.c_str());
    dumpFrame();uint32_t started=millis();
    while(millis()-started<600000){
        dns.processNextRequest();server.handleClient();
        if(restart){delay(700);ESP.restart();}
        delay(5);
    }
    server.stop();dns.stop();stopWifi();message("Setup timed out","Hold BOOT 3 sec","to set up Wi-Fi");deepSleep(true);
}
bool updateWeather(){
    WiFiClientSecure tls;tls.setCACert(WEATHER_CA);tls.setHandshakeTimeout(20);
    HTTPClient http;http.setConnectTimeout(10000);http.setTimeout(10000);http.useHTTP10(true);
    char url[384];
    snprintf(url,sizeof(url),"https://api.open-meteo.com/v1/forecast?latitude=%.5f&longitude=%.5f&current=temperature_2m,relative_humidity_2m,weather_code,is_day&timeformat=unixtime&forecast_days=1",WEATHER_LATITUDE,WEATHER_LONGITUDE);
    if(!http.begin(tls,url))return false;
    int status=http.GET();
    if(status!=200){
        char detail[160]={};int tlsError=tls.lastError(detail,sizeof(detail));
        Serial.printf("WEATHER_HTTP_ERROR=%d TLS_ERROR=%d detail=%s\n",status,tlsError,detail);
        http.end();return false;
    }
    JsonDocument doc,filter;filter["current"]=true;
    DeserializationError error=deserializeJson(doc,http.getStream(),DeserializationOption::Filter(filter));http.end();
    if(error){Serial.printf("WEATHER_JSON_ERROR=%s\n",error.c_str());return false;}
    JsonObject current=doc["current"];
    if(!current["temperature_2m"].is<float>()||!current["relative_humidity_2m"].is<int>()||
       !current["weather_code"].is<int>()||!current["is_day"].is<int>()||!current["time"].is<long>()){
        Serial.println("WEATHER_FIELDS_MISSING");return false;
    }
    float temp=current["temperature_2m"];int rh=current["relative_humidity_2m"],code=current["weather_code"];
    time_t observed=current["time"].as<long>(),now=time(nullptr);int day=current["is_day"];
    if(!std::isfinite(temp)||temp< -90||temp>65||rh<0||rh>100||code<0||code>99||day<0||day>1||
        observed>now+3600 || observed<now-7200){Serial.println("WEATHER_FIELDS_INVALID");return false;}
    portENTER_CRITICAL(&weatherMux);
    cached.temperature=temp;cached.humidity=rh;cached.code=code;cached.day=day==1;
    cached.fetched=now;cached.observed=observed;cached.valid=true;
    portEXIT_CRITICAL(&weatherMux);
    Serial.printf("WEATHER_OK temperature=%.1f humidity=%d code=%d observed=%lld\n",temp,rh,code,(long long)observed);
    return true;
}
struct WifiSignalStats {
    int minimum=0,maximum=-127,total=0,count=0;
    uint32_t lastMs=0;
    void sample(bool force=false){
        const uint32_t now=millis();
        if(!force && count && now-lastMs<250)return;
        if(WiFi.status()!=WL_CONNECTED)return;
        const int rssi=WiFi.RSSI();if(rssi>=0 || rssi< -126)return;
        if(rssi<minimum)minimum=rssi;if(rssi>maximum)maximum=rssi;
        total+=rssi;++count;lastMs=now;
    }
};
void networkUpdate(){
    WiFi.persistent(false);WiFi.mode(WIFI_STA);WiFi.setAutoReconnect(true);WiFi.setSleep(false);
    Serial.printf("WIFI_BEGIN status=%d\n",(int)WiFi.begin(savedSsid.c_str(),savedPass.c_str()));
    uint32_t start=millis();while(WiFi.status()!=WL_CONNECTED && millis()-start<25000)delay(50);
    bool success=false;
    if(WiFi.status()==WL_CONNECTED){
        Serial.println("WIFI_CONNECTED");
        const uint32_t connectedAt=millis(),connectMs=connectedAt-start;
        const int channel=WiFi.channel();WifiSignalStats signal;signal.sample(true);
        const uint32_t syncBudget=clockValid()?5000:15000;
        sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
        // Configure SNTP on the TCP/IP thread without touching the UI's timezone.
        esp_sntp_stop();esp_sntp_setoperatingmode(ESP_SNTP_OPMODE_POLL);
        esp_sntp_setservername(0,"pool.ntp.org");
        esp_sntp_setservername(1,"time.google.com");
        esp_sntp_setservername(2,"time.cloudflare.com");esp_sntp_init();
        bool synced=false;start=millis();
        while(millis()-start<syncBudget){
            signal.sample();
            if(sntp_get_sync_status()==SNTP_SYNC_STATUS_COMPLETED){synced=true;break;}
            delay(50);
        }
        Serial.println(synced?"NTP_SYNCED":clockValid()?"NTP_RTC_FALLBACK":"TIME_SYNC_FAILED");
        signal.sample(true);
        if(clockValid()){
            for(int attempt=1;attempt<=2 && !success;++attempt){
                Serial.printf("WEATHER_ATTEMPT=%d\n",attempt);success=updateWeather();
                signal.sample(true);
                if(!success && attempt<2)delay(750);
            }
        }
        if(signal.count)Serial.printf("WIFI_SIGNAL samples=%d min_dbm=%d avg_dbm=%d max_dbm=%d channel=%d connect_ms=%u online_ms=%u weather_ok=%d\n",
            signal.count,signal.minimum,signal.total/signal.count,signal.maximum,channel,
            connectMs,millis()-connectedAt,success);
    }else {
        Serial.printf("WIFI_CONNECTION_FAILED status=%d elapsed_ms=%u\n",(int)WiFi.status(),millis()-start);
        WiFi.setAutoReconnect(false);WiFi.disconnect(false,false);
        int found=WiFi.scanNetworks(false,true),rssi=0,channel=0;bool visible=false;
        for(int i=0;i<found;++i)if(WiFi.SSID(i)==savedSsid){visible=true;rssi=WiFi.RSSI(i);channel=WiFi.channel(i);break;}
        Serial.printf("WIFI_TARGET_VISIBLE=%d RSSI=%d CHANNEL=%d scan_count=%d\n",visible,rssi,channel,found);
        WiFi.scanDelete();
    }
    const time_t next=nextNetworkMinute(time(nullptr),success);
    portENTER_CRITICAL(&weatherMux);cached.nextNetwork=next;portEXIT_CRITICAL(&weatherMux);
    Serial.printf("NEXT_NETWORK=%lld\n",(long long)next);
    esp_sntp_stop();
    uint32_t stopping=millis();while(esp_sntp_enabled() && millis()-stopping<1000)delay(10);
    stopWifi();
}
void networkWorker(void *){
    for(;;){
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        networkUpdate();
        Serial.println("NETWORK_FINISHED");
        portENTER_CRITICAL(&weatherMux);
        ++networkGeneration;networkBusy=false;
        portEXIT_CRITICAL(&weatherMux);
    }
}
void requestNetwork(){
    if(!networkTask)return;
    portENTER_CRITICAL(&weatherMux);
    const bool start=!networkBusy;
    if(start)networkBusy=true;
    portEXIT_CRITICAL(&weatherMux);
    if(start){
        // Change frequency on the UI task, between OLED transfers. APB stays 80 MHz.
        setCpuFrequencyMhz(240);
        Serial.printf("CPU_NETWORK mhz=%u\n",getCpuFrequencyMhz());
        xTaskNotifyGive(networkTask);
    }
}
void showPage(const WeatherCache &data,bool busy,bool switched){
    ClockView view;time_t now=time(nullptr);tm local={};localtime_r(&now,&local);
    if(clockValid()){
        strftime(view.time,sizeof(view.time),"%H:%M",&local);
        strftime(view.date,sizeof(view.date),"%a %m/%d",&local);
    }
    view.valid=data.valid;view.temperature=data.temperature;view.humidity=data.humidity;
    view.code=data.code;view.day=data.day;
    view.batteryLevel=batteryLevel;
    view.stale=!data.valid || now-data.fetched>1800 || now-data.observed>3600;
    if(!oledReady)oledReady=startOled(false);
    renderClockPage(frame,view,weatherPage);oledReady=flushOled();
    const int64_t pageShownUs=esp_timer_get_time();
    Serial.printf("PAGE name=%s switched=%d monotonic_ms=%lld time=%s weatherValid=%d stale=%d network=%d\n",
        weatherPage?"weather":"clock",switched,(long long)(pageShownUs/1000),view.time,view.valid,view.stale,busy);
    const uint8_t bit=weatherPage?2:1;
    if(dumpPages&bit){dumpFrame();dumpPages&=~bit;}
}
void sleepUntil(int64_t deadline){
    // Only the UI task requests network work, and it never does so while asleep.
    bool busy;cacheSnapshot(&busy);
    if(busy || digitalRead(BUTTON)==LOW || WiFi.getMode()!=WIFI_OFF){delay(10);return;}
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    esp_sleep_enable_ext0_wakeup((gpio_num_t)BUTTON,0);
    rtc_gpio_pullup_en((gpio_num_t)BUTTON);rtc_gpio_pulldown_dis((gpio_num_t)BUTTON);
    Serial.flush();
    int64_t remaining=deadline-esp_timer_get_time();
    if(remaining<3000){delay(1);return;}
    esp_sleep_enable_timer_wakeup(remaining);
    const int64_t before=esp_timer_get_time();
    esp_err_t result=esp_light_sleep_start();
    const auto reason=esp_sleep_get_wakeup_cause();
    rtc_gpio_deinit((gpio_num_t)BUTTON);pinMode(BUTTON,INPUT_PULLUP);
    Serial.printf("LIGHT_SLEEP elapsed_ms=%lld wake=%d result=%d wifiMode=%d\n",
        (long long)((esp_timer_get_time()-before)/1000),(int)reason,(int)result,(int)WiFi.getMode());
    if(result!=ESP_OK)delay(10);
}
void setup(){
    Serial.begin(115200);delay(80);
    setenv("TZ",WEATHER_TIMEZONE,1);tzset();
    const auto wake=esp_sleep_get_wakeup_cause();
    const bool warm=(wake==ESP_SLEEP_WAKEUP_TIMER||wake==ESP_SLEEP_WAKEUP_EXT0)&&cached.magic==MAGIC;
    if(!warm)cached={};
    restoreOutputs();startBattery();oledReady=startOled(warm);
    if(!warm)sleepRadio();
    cached.magic=MAGIC;++cached.wakes;
    Serial.printf("V3_CLOCK version=1.2.4 wake=%d retained=%d page_mode=manual cpuMHz=%u\n",
        (int)wake,warm,getCpuFrequencyMhz());
    WiFi.onEvent([](WiFiEvent_t event,WiFiEventInfo_t info){
        if(event==ARDUINO_EVENT_WIFI_STA_DISCONNECTED)
            Serial.printf("WIFI_DISCONNECT_REASON=%u\n",info.wifi_sta_disconnected.reason);
        if(event==ARDUINO_EVENT_WIFI_STA_CONNECTED)Serial.printf("WIFI_ASSOCIATED ms=%u\n",millis());
        if(event==ARDUINO_EVENT_WIFI_STA_GOT_IP)Serial.printf("WIFI_GOT_IP ms=%u\n",millis());
    });
    readSettings();
    if(savedSsid.isEmpty())provision();
    // TLS certificate work must not starve CPU0's idle watchdog or the UI.
    // Priority zero shares time with idle; the priority-one UI can preempt it.
    if(xTaskCreatePinnedToCore(networkWorker,"weather",16384,nullptr,0,&networkTask,1)!=pdPASS){
        Serial.println("NETWORK_TASK_FAILED");networkTask=nullptr;
    }
    showPage(cacheSnapshot(),false,false);
    shownMinute=time(nullptr)/60;
    requestNetwork();
}
void loop(){
    const int64_t nowUs=esp_timer_get_time();
    const uint8_t action=prg.update(nowUs,digitalRead(BUTTON)==LOW);
    bool buttonSwitched=false,timerFinished=false;
    if(action&PrgGesture::OpenSetup){
        if(countdown.running){countdown.stop();timerFinished=true;Serial.println("COUNTDOWN_CANCEL setup=1");}
        setupRequested=true;Serial.println("SETUP_REQUESTED");
    }
    if(!setupRequested && !countdown.running && (action&PrgGesture::StartTimer)){
        countdown.start(nowUs);nextTimerFrameUs=nowUs;
        lastTimerPhase=TimerPhase::Normal;lastCountdownSecond=-1;
        timerFrames=timerFailures=0;prg.clearClicks();
        Serial.printf("COUNTDOWN_START monotonic_us=%lld duration_us=30000000 return_page=%s\n",
            (long long)nowUs,weatherPage?"weather":"clock");
    }else if(!setupRequested && !countdown.running && (action&PrgGesture::TogglePage)){
        weatherPage=!weatherPage;buttonSwitched=true;
        Serial.printf("BUTTON_PAGE name=%s\n",weatherPage?"weather":"clock");
    }
    bool busy;uint32_t generation;
    WeatherCache data=cacheSnapshot(&busy,&generation);
    if(!busy && getCpuFrequencyMhz()!=80){setCpuFrequencyMhz(80);Serial.println("CPU_IDLE mhz=80");}
    if(setupRequested && !busy){provision();return;}

    if(countdown.running){
        const TimerView view=countdown.view(nowUs);
        if(view.phase==TimerPhase::Normal){
            countdown.stop();prg.clearClicks();timerFinished=true;
            Serial.printf("COUNTDOWN_DONE elapsed_us=%lld frames=%u errors=%u return_page=%s\n",
                (long long)(nowUs-countdown.startedAt),timerFrames,timerFailures,
                weatherPage?"weather":"clock");
        }else{
            if(nowUs>=nextTimerFrameUs){
                if(view.phase!=lastTimerPhase){
                    const char *phase=view.phase==TimerPhase::Counting?"counting":
                                      view.phase==TimerPhase::Zero?"zero":"fireworks";
                    Serial.printf("COUNTDOWN_PHASE name=%s elapsed_us=%lld\n",
                        phase,(long long)(nowUs-countdown.startedAt));
                    lastTimerPhase=view.phase;
                }
                const int second=(view.hundredths+99)/100;
                if(view.phase==TimerPhase::Counting && second!=lastCountdownSecond){
                    Serial.printf("COUNTDOWN_TICK remaining_cs=%d elapsed_us=%lld\n",
                        view.hundredths,(long long)(nowUs-countdown.startedAt));
                    lastCountdownSecond=second;
                }
                if(!oledReady)oledReady=startOled(false);
                renderCountdown(frame,view);oledReady=flushOled(false);
                ++timerFrames;if(!oledReady)++timerFailures;
                nextTimerFrameUs+=Countdown::FrameUs;
                if(nextTimerFrameUs<=nowUs)nextTimerFrameUs=nowUs+Countdown::FrameUs;
            }
            // Leave the selected normal page intact. Existing network work can
            // finish, but new requests and battery ADC work wait until the effect ends.
            delay(1);return;
        }
    }
    if(!busy && !setupRequested && time(nullptr)>=data.nextNetwork){
        requestNetwork();data=cacheSnapshot(&busy,&generation);
    }
    bool redraw=buttonSwitched||timerFinished;
    if(generation!=seenGeneration){seenGeneration=generation;dumpPages=3;redraw=true;}
    time_t minute=time(nullptr)/60;
    if(minute!=shownMinute){shownMinute=minute;redraw=true;}
    if(redraw)showPage(data,busy,buttonSwitched);
    if(batteryEnable>=0 && !busy && !prg.down && !prg.clicks && esp_timer_get_time()>=nextBatteryUs){
        const int previous=batteryLevel;readBattery();
        if(previous!=batteryLevel)showPage(data,busy,false);
    }
    if(busy || prg.down || setupRequested){delay(5);return;}
    timeval tv;gettimeofday(&tv,nullptr);
    const int64_t toMinute=(60-tv.tv_sec%60)*1000000LL-tv.tv_usec;
    int64_t deadline=nextIdleWakeUs(esp_timer_get_time(),toMinute,
                                   batteryEnable>=0?nextBatteryUs:0);
    // Waiting for a second/third click must not sleep until the next minute.
    if(prg.clicks && prg.settleAt<deadline)deadline=prg.settleAt;
    sleepUntil(deadline);
}
