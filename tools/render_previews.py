"""Render firmware framebuffers and the actual native weather icon family."""
from pathlib import Path
from PIL import Image, ImageOps, ImageDraw, ImageFont
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'docs/images';OUT.mkdir(parents=True,exist_ok=True)
frames=[]
for page in ('clock','weather'):
    im=ImageOps.invert(Image.open(ROOT/f'build/{page}.pbm').convert('L'))
    if page=='clock':
        assert im.crop((0,11,128,64)).tobytes()==Image.open(ROOT/'tests/clock-body-reference.png').tobytes()
    im.save(OUT/f'ui-{page}-128x64.png')
    large=im.resize((1024,512),Image.Resampling.NEAREST);large.save(OUT/f'ui-{page}.png');frames.append(large)
frames[0].save(OUT/'ui-carousel.gif',save_all=True,append_images=frames[1:],duration=3000,loop=0)
labels=[('clear','Clear'),('night','Clear night'),('partly-cloudy','Partly cloudy'),
        ('partly-night','Cloudy night'),('overcast','Overcast'),('fog','Fog'),
        ('drizzle','Drizzle'),('rain','Rain'),('showers','Showers'),('snow','Snow'),('storm','Thunderstorm')]
font_path=str(ROOT/'third_party/fonts/BarlowCondensed-Bold.ttf')
title=ImageFont.truetype(font_path,34);label_font=ImageFont.truetype(font_path,28);note=ImageFont.truetype(font_path,20)
sheet=Image.new('RGB',(984,854),'#eeeeea');draw=ImageDraw.Draw(sheet)
draw.text((24,12),'Weather icons / native OLED pixels',font=title,fill='#161616')
for i,(name,label) in enumerate(labels):
    x=24+(i%4)*240;y=66+(i//4)*250
    icon=ImageOps.invert(Image.open(ROOT/f'build/icons-{name}.pbm').convert('L')).crop((42,10,86,48))
    card=Image.new('L',(216,200),0);card.paste(icon.resize((176,152),Image.Resampling.NEAREST),(20,24));sheet.paste(card.convert('RGB'),(x,y))
    width=draw.textlength(label,font=label_font);draw.text((x+(216-width)/2,y+204),label,font=label_font,fill='#161616')
draw.text((24,818),'44 x 38 pixels. Centered silhouettes. Labels appear only in this guide.',font=note,fill='#505050')
sheet.save(OUT/'weather-icons.png')
print('Saved public font previews and all 11 native weather icons.')
