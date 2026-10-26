"""Generate redistributable OLED glyphs from the bundled OFL font.

Letter/symbol glyph data remains under SIL OFL 1.1; the exporter and procedural
seven-segment digits are MIT. See third_party/fonts/OFL.txt.
"""
from pathlib import Path
from PIL import Image, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1]

def pack(im,threshold=150):
    data=[]
    for y in range(im.height):
        for start in range(0,im.width,8):
            byte=0
            for bit in range(8):
                if start+bit<im.width and im.getpixel((start+bit,y))<threshold:byte|=128>>bit
            data.append(byte)
    return data

def digital_numerals(glyphs,pixels):
    """Bake the reviewed seven-segment simulation into flash-resident font data."""
    zero=glyphs[ord('0')-32]
    height=zero[2];width=max(4,(height*22+17)//35);stroke=max(1,(height*3+17)//35)
    masks=[0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f]
    output=[];result=[]
    for code,g in enumerate(glyphs,32):
        offset,w,h,left,top,advance=g
        start=len(output)
        if ord('0')<=code<=ord('9'):
            im=Image.new('L',(width,height),255);draw=ImageDraw.Draw(im)
            middle=(height-1)//2
            segments=[(stroke,0,width-2*stroke,stroke),
                      (width-stroke,stroke,stroke,middle-stroke),
                      (width-stroke,middle+stroke,stroke,height-middle-2*stroke),
                      (stroke,height-stroke,width-2*stroke,stroke),
                      (0,middle+stroke,stroke,height-middle-2*stroke),
                      (0,stroke,stroke,middle-stroke),
                      (stroke,middle,width-2*stroke,stroke)]
            for bit,(x,y,sw,sh) in enumerate(segments):
                if masks[code-ord('0')]&(1<<bit) and sw>0 and sh>0:
                    draw.rectangle((x,y,x+sw-1,y+sh-1),fill=0)
            bounds=im.point(lambda value:255-value).getbbox()
            assert bounds is not None
            left=max(0,((advance+32)//64-width)//2)+bounds[0]
            top=zero[4]+bounds[1]
            im=im.crop(bounds);w,h=im.size
            output+=pack(im)
        else:
            output+=pixels[offset:offset+((w+7)//8)*h]
        result.append((start,w,h,left,top,advance))
    return result,output

lines=['// Letter/symbol glyphs derived from Barlow Condensed, SIL OFL 1.1.',
       '// Copyright 2017 The Barlow Project Authors. See third_party/fonts/OFL.txt.',
       '#pragma once','#ifdef ARDUINO\n#include <Arduino.h>\n#else\n#include <cstdint>\n#define PROGMEM\n#endif',
       'struct UiGlyph { uint16_t offset; uint8_t w,h; int8_t left,top; uint16_t advance64; };',
       'struct UiFont { const UiGlyph *glyphs; const uint8_t *pixels; };']
for size in (10,11,12,13,18):
    font=ImageFont.truetype(str(ROOT/'third_party/fonts/BarlowCondensed-Bold.ttf'),size)
    pixels=[];glyphs=[]
    for code in range(32,127):
        ch=chr(code);left,top,right,bottom=font.getbbox(ch,anchor='la')
        im=Image.new('L',(max(1,right-left),max(1,bottom-top)),255)
        ImageDraw.Draw(im).text((-left,-top),ch,font=font,fill=0,anchor='la')
        bounds=im.point(lambda v:255 if v<150 else 0).getbbox()
        if bounds:
            left+=bounds[0];top+=bounds[1];im=im.crop(bounds);w,h=im.size
        else:w=h=0
        glyphs.append((len(pixels),w,h,left,top,round(font.getlength(ch)*64)))
        if w and h:pixels+=pack(im)
    glyphs,pixels=digital_numerals(glyphs,pixels)
    name=f'ui_font{size}'
    lines.append(f'static const uint8_t {name}_pixels[] PROGMEM = {{')
    lines += [','.join(str(x) for x in pixels[i:i+24])+',' for i in range(0,len(pixels),24)]
    lines.append('};');lines.append(f'static const UiGlyph {name}_glyphs[] PROGMEM = {{')
    lines += ['{'+','.join(str(x) for x in g)+'},' for g in glyphs]
    lines.append('};');lines.append(f'static const UiFont {name} = {{{name}_glyphs,{name}_pixels}};')
(ROOT/'src/ui_fonts.h').write_text('\n'.join(lines)+'\n')
print('Generated OLED fonts from bundled Barlow Condensed (OFL).')
