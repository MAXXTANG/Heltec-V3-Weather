"""Generate fixed 12px Noto CJK TC bitmaps; no host font required at runtime."""
from pathlib import Path
import sys
from PIL import Image, ImageDraw, ImageFont

ROOT=Path(__file__).resolve().parents[1]
source=Path(sys.argv[1])
wide=ImageFont.truetype(str(source),12)
narrow=ImageFont.truetype(str(source),11)
ranges=[(32,126),(0x3000,0x303f),(0x3400,0x4dbf),(0x4e00,0x9fff),(0xff00,0xffef)]
data=bytearray()
for first,last in ranges:
    for cp in range(first,last+1):
        im=Image.new('L',(12,12))
        if cp<127:
            ImageDraw.Draw(im).text((0,10),chr(cp),font=narrow,fill=255,anchor='ls')
        else:
            # Noto's nominal 12px size can extend above/below a 12px cell.
            # Fit the actual thresholded ink, preserving the top/bottom strokes.
            glyph=Image.new('L',(24,24))
            ImageDraw.Draw(glyph).text((0,16),chr(cp),font=wide,fill=255,anchor='ls')
            glyph=glyph.point(lambda p:255 if p>=95 else 0)
            box=glyph.getbbox()
            if box:
                glyph=glyph.crop(box)
                if glyph.width>12 or glyph.height>12:
                    glyph=glyph.resize((min(12,glyph.width),min(12,glyph.height)),Image.Resampling.NEAREST)
                im.paste(glyph,((12-glyph.width)//2,(12-glyph.height)//2))
        for y in range(12):
            bits=0
            for x in range(12):
                if im.getpixel((x,y))>=95: bits|=1<<(15-x)
            data.extend(bits.to_bytes(2,'big'))
out=ROOT/'src/disaster_font.h'
with out.open('w') as f:
    f.write('// Generated from Noto Sans CJK TC Regular (SIL OFL 1.1).\n#pragma once\n#include <cstdint>\n')
    f.write('static const uint8_t disasterFontPixels[]={\n')
    for i in range(0,len(data),24): f.write(','.join(f'0x{b:02x}' for b in data[i:i+24])+',\n')
    f.write('};\ninline int disasterGlyphIndex(uint32_t cp){\n')
    offset=0
    for first,last in ranges:
        f.write(f'if(cp>={first} && cp<={last})return {offset}+cp-{first};\n')
        offset+=last-first+1
    f.write("return '?'-32;\n}\n")
print(f'{offset} glyphs, {len(data)} bitmap bytes -> {out}')
