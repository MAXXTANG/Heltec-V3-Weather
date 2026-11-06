"""Enlarge actual firmware framebuffers, without smoothing OLED pixels."""
from pathlib import Path
from PIL import Image, ImageOps, ImageDraw
ROOT=Path(__file__).resolve().parents[1]
names=['disaster','disaster-units','disaster-failed']
canvas=Image.new('RGB',(544,900),(22,28,34))
for i,name in enumerate(names):
    im=ImageOps.invert(Image.open(ROOT/f'build/{name}.pbm').convert('L'))
    im.save(ROOT/f'docs/{name}-128x64.png')
    canvas.paste(im.resize((512,256),Image.Resampling.NEAREST),(16,i*300+30))
    ImageDraw.Draw(canvas).text((16,i*300+10),['Incident / detail','Dispatch units','Failed check / cached data'][i],fill='white')
canvas.save(ROOT/'docs/disaster-preview.png')
