"""Verify the installed V3 through USB DTR/GPIO0 (not mechanical/optical testing)."""
import argparse,json,re,time
from pathlib import Path
import serial
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--port',default='/dev/cu.usbserial-0001')
parser.add_argument('--output',type=Path,default=Path('build/alerts-device-runtime.log'))
args=parser.parse_args()
p=serial.Serial(port=None,baudrate=115200,timeout=.03)
p.dtr=False;p.rts=False;p.port=args.port;p.open()
args.output.parent.mkdir(parents=True,exist_ok=True)
buffer=bytearray()
with args.output.open('wb') as log:
    def pump(seconds):
        end=time.monotonic()+seconds
        while time.monotonic()<end:
            data=p.read(8192)
            if data:buffer.extend(data);log.write(data);log.flush()
    def text():return buffer.decode('utf8','replace')
    def press():
        p.dtr=True;pump(.10);p.dtr=False;pump(.9)
    def page():
        rows=re.findall(r'PAGE name=(\w+) switched=\d monotonic_ms=\d+.*',text())
        assert rows,'No rendered page';return rows[-1]
    def case_index():
        rows=re.findall(r'DISASTER_VIEW index=(\d+) count=(\d+)',text());assert rows
        return tuple(map(int,rows[-1]))
    try:
        p.rts=True;time.sleep(.12);p.rts=False
        until=time.monotonic()+90
        while time.monotonic()<until:
            pump(1)
            if 'NETWORK_FINISHED' in text():break
            assert 'SETUP_AP=' not in text(),'Device needs Wi-Fi setup'
        assert re.search(r'version=1\.4\.\d+[-\w]*',text()),'Wrong firmware version'
        counts=re.findall(r'DISASTER_OK count=(\d+).* new=(\d+)',text())
        assert counts and counts[0][1]=='0','Baseline fetch missing or triggered alert'
        assert 'WEATHER_OK' in text() and 'NTP_SYNCED' in text(),'Weather/time not verified'
        assert 'DISASTER_ALERT_START' not in text(),'Unexpected notification during baseline'
        count=int(counts[-1][0]);assert count>0,'Need live cases to verify paging'
        # The user may have browsed during boot/network startup. Normalize once.
        for _ in range(count+3):
            if page()=='clock':break
            press()
        assert page()=='clock'
        press();assert page()=='weather'
        press();assert page()=='disaster' and case_index()==(0,count)
        pump(27)  # The former 24-second automatic advance must no longer occur.
        assert case_index()==(0,count),'Case changed without a BOOT press'
        for index in range(1,count):
            press();assert page()=='disaster' and case_index()==(index,count),'Manual case advance failed'
        press();assert page()=='clock','Last case did not return to clock'
        # In normal mode, triple click still starts the existing countdown.
        for _ in range(3):
            p.dtr=True;pump(.10);p.dtr=False;pump(.10)
        pump(36)
        assert 'COUNTDOWN_DONE' in text() and 'return_page=clock' in text(),'Countdown/return failed'
        until=time.monotonic()+25
        while time.monotonic()<until:
            if len(re.findall(r'DISASTER_OK ',text()))>=2 and text().rfind('NETWORK_FINISHED')>text().rfind('DISASTER_OK '):break
            pump(1)
        assert len(re.findall(r'DISASTER_OK ',text()))>=2,'Second live refresh missing'
        assert not any(x in text() for x in ['Guru Meditation','Backtrace:','task_wdt:','OLED_FRAME ok=0','clipped=1']),'Device crash/OLED error'
        assert 'DISASTER_ALERT_START' not in text(),'Live source changed during test; inspect before interpreting result'
        result={'firmware':re.search(r'V3_CLOCK version=([^ ]+)',text())[1],'live_disaster_verified':True,'live_case_count':count,
            'weather_ntp_verified':True,'manual_case_paging_verified':True,'no_automatic_case_advance_seconds':27,
            'countdown_verified':True,'live_refreshes':len(re.findall(r'DISASTER_OK ',text())),
            'new_case_alert_native_simulation_verified':True,'new_case_alert_live_event_verified':False,
            'button_test':'USB DTR/GPIO0 simulation; mechanical and optical tests not performed'}
        Path('docs/device-status.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
        print(json.dumps(result,ensure_ascii=False,indent=2))
    finally:
        p.dtr=False;p.rts=False;p.close()
        for line in text().splitlines():
            if line.startswith(('V3_CLOCK ','DISASTER_OK ','DISASTER_HTTP_ERROR ','DISASTER_PARSE_ERROR','BUTTON_PAGE ','COUNTDOWN_DONE ','DISASTER_ALERT_')):print(line)
