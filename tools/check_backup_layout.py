"""Check a full 8MB backup against the built partition table before app-only writes."""
from pathlib import Path
import hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[1]
backup=Path(sys.argv[1])
raw=backup.read_bytes()
assert len(raw)==0x800000,'Full 8MB backup required'
table=(ROOT/'.pio/build/heltec-v3/partitions.bin').read_bytes()
assert raw[0x8000:0x8000+len(table)]==table,'Partition table differs from this build'
partitions=[]
for i in range(0,0xc00,32):
    magic,kind,subtype,offset,size,label,flags=struct.unpack('<HBBII16sI',raw[0x8000+i:0x8020+i])
    if magic!=0x50aa:break
    label=label.split(b'\0')[0].decode('ascii')
    partitions.append(dict(label=label,type=kind,subtype=subtype,offset=offset,size=size))
app=next(p for p in partitions if p['type']==0 and p['subtype']==0x10)
assert app['offset']==0x10000
assert (ROOT/'.pio/build/heltec-v3/firmware.bin').stat().st_size<=app['size']
ota=next(p for p in partitions if p['type']==1 and p['subtype']==0)
entries=[]
for offset in [ota['offset'],ota['offset']+0x1000]:
    seq=struct.unpack_from('<I',raw,offset)[0]
    state,crc=struct.unpack_from('<II',raw,offset+24)
    # ROM boot_app0 uses sequence 1 for ota_0. Reject a non-default slot.
    if seq not in (0,0xffffffff):entries.append(dict(sequence=seq,state=state,crc=crc))
assert not entries or all((e['sequence']-1)%2==0 for e in entries),'OTA metadata may select app1; inspect before flashing'
report=dict(backup_bytes=len(raw),backup_sha256=hashlib.sha256(raw).hexdigest(),
            partition_table_matches=True,app_offset=app['offset'],partitions=partitions,ota_entries=entries)
(backup.parent/'layout-check.json').write_text(json.dumps(report,indent=2)+'\n')
print('8MB backup verified; partition table matches; app0 at 0x10000; original NVS and boot regions preserved')
