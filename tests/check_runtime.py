"""Validate a local carousel serial capture and export its two valid screen frames."""
import argparse, json, re, statistics
from pathlib import Path
from PIL import Image

p=argparse.ArgumentParser(description=__doc__)
p.add_argument('log',type=Path)
p.add_argument('--output-dir',type=Path,default=Path('build/v3/runtime'))
a=p.parse_args()
text=a.log.read_text(errors='replace')
assert not any(x in text for x in ('task_wdt:', 'Backtrace:', 'Guru Meditation', 'OLED_FRAME ok=0')), 'Crash or OLED failure in capture'
assert 'WEATHER_OK' in text and 'NTP_SYNCED' in text, 'Weather/NTP not verified'
pattern=r'PAGE name=(\w+) switched=(\d) monotonic_ms=(\d+) time=(\S+) weatherValid=(\d) stale=(\d) network=(\d)'
rows=[m.groups() for m in re.finditer(pattern,text)]
switches=[r for r in rows if r[1]=='1']
assert len(switches)>=8, 'Capture at least eight page changes'
intervals=[int(b[2])-int(a[2]) for a,b in zip(switches,switches[1:])]
assert all(2900<=x<=3100 for x in intervals), intervals
assert all(a[0]!=b[0] for a,b in zip(switches,switches[1:])), 'Pages do not alternate'
sleeps=re.findall(r'LIGHT_SLEEP elapsed_ms=(\d+) wake=(\d+) result=(\d+) wifiMode=(\d+)',text)
assert len(sleeps)>=5 and all(r[2]=='0' and r[3]=='0' for r in sleeps), 'Sleep/Wi-Fi verification failed'
frames={};current=None
for line in text.splitlines():
    m=re.match(pattern,line)
    if m:current=m.groups()
    elif line.startswith('FRAME_HEX=') and current and current[4]=='1':
        data=bytes.fromhex(line.split('=',1)[1]);assert len(data)==1024
        im=Image.new('L',(128,64),0)
        for y in range(64):
            for x in range(128):im.putpixel((x,y),255 if data[x+(y//8)*128] & (1<<(y%8)) else 0)
        frames[current[0]]=im
assert set(frames)=={'clock','weather'}, 'Need valid snapshots of both pages'
assert frames['clock'].crop((0,0,128,11)).tobytes()==frames['weather'].crop((0,0,128,11)).tobytes(), 'Headers differ'
a.output_dir.mkdir(parents=True,exist_ok=True)
large=[]
for name in ('clock','weather'):
    im=frames[name];im.save(a.output_dir/f'v3-device-carousel-{name}-128x64.png')
    im=im.resize((1024,512),Image.Resampling.NEAREST);im.save(a.output_dir/f'v3-device-carousel-{name}.png');large.append(im)
large[0].save(a.output_dir/'v3-device-carousel.gif',save_all=True,append_images=large[1:],duration=3000,loop=0)
print(json.dumps({'page_changes':len(switches),'interval_ms':{'min':min(intervals),'median':statistics.median(intervals),'max':max(intervals)},'network_page_changes':sum(r[6]=='1' for r in switches),'light_sleep_count':len(sleeps),'fixed_header':'pixel-identical','valid_frames':list(frames)},indent=2))
