"""Trigger exactly one labeled USB simulation; the user alone acknowledges BOOT."""
import argparse,json,re,time
from pathlib import Path
import serial
ap=argparse.ArgumentParser(description=__doc__)
ap.add_argument('--port',default='/dev/cu.usbserial-0001')
ap.add_argument('--wait-seconds',type=int,default=300)
args=ap.parse_args()
p=serial.Serial(port=None,baudrate=115200,timeout=.05)
p.dtr=False;p.rts=False;p.port=args.port;p.open()
Path('build').mkdir(exist_ok=True)
path=Path('build/usb-new-case-demo.log')
received=bytearray();pending=b'';sent=False;steady=False;ack=False;clock_after=False;end_after=None
try:
    with path.open('wb') as log:
        # Only reset EN once to open a reliable UART receive window. BOOT/DTR
        # stays deasserted throughout; no synthetic short press is ever sent.
        p.rts=True;time.sleep(.12);p.rts=False
        deadline=time.monotonic()+args.wait_seconds
        while time.monotonic()<deadline:
            data=p.read(8192)
            if data:
                log.write(data);log.flush();received.extend(data);pending+=data
                while b'\n' in pending:
                    raw,pending=pending.split(b'\n',1);line=raw.decode('utf8','replace').strip()
                    if line.startswith('V3_CLOCK version=1.4.1-usb-demo') and not sent:
                        p.write(b'SIMULATE_NEW_CASE\n');p.flush();sent=True
                        print('USB simulation command sent once; waiting for 8-second arm delay.',flush=True)
                    if line.startswith(('SIMULATION_','DISASTER_ALERT_')):
                        print(line,flush=True)
                    if line.startswith('DISASTER_ALERT_STEADY'):
                        steady=True;print('WAITING_FOR_USER_BOOT: notification is steady; no automatic button input will be sent.',flush=True)
                    if line.startswith('DISASTER_ALERT_ACK'):
                        ack=True
                    if ack and line.startswith('PAGE name=clock '):
                        clock_after=True;end_after=time.monotonic()+2
                    if 'SETUP_AP=' in line:raise RuntimeError('Device requires Wi-Fi setup; no credentials printed')
                    if any(x in line for x in ['Guru Meditation','Backtrace:','task_wdt:','OLED_FRAME ok=0']):raise RuntimeError('Firmware/OLED error; inspect local log')
            if end_after is not None and time.monotonic()>=end_after:break
finally:
    # DTR/BOOT was never asserted. Leave the display and user selection intact.
    p.dtr=False;p.rts=False;p.close()
s=received.decode('utf8','replace')
match=re.search(r'DISASTER_ALERT_STEADY elapsed_us=(\d+)',s)
result={'firmware':'1.4.1-usb-demo','simulation_command_sent_once':sent,
        'simulated_new_case_detected':'SIMULATION_NEW_CASE count=1' in s,
        'hardware_notification_started':'duration_us=3000000 simulated=1' in s,
        'steady_after_blink':steady,'blink_elapsed_us':int(match[1]) if match else None,
        'boot_acknowledged_without_synthetic_press':ack,'returned_to_clock':clock_after,
        'simulation_cleared':'SIMULATION_CLEARED after_manual_boot=1' in s,
        'official_case_cache_untouched':True,'official_new_incident_test':False,
        'optical_flash_timing_measured':False}
Path('docs/usb-demo-result.json').write_text(json.dumps(result,ensure_ascii=False,indent=2)+'\n')
print(json.dumps(result,ensure_ascii=False,indent=2),flush=True)
assert result['hardware_notification_started'],'Simulation did not start'
if not ack:print('Demo remains displayed until the user presses BOOT.',flush=True)
