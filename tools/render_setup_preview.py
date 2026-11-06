"""Compare the old/new setup layouts using example credentials only."""
from pathlib import Path
from PIL import Image,ImageOps,ImageDraw
ROOT=Path(__file__).resolve().parents[1]
canvas=Image.new('RGB',(544,600),(22,28,34))
for i,(name,label) in enumerate([('setup-before','Before / sample credentials'),('setup-readable','Cousine Bold / sample credentials')]):
    im=ImageOps.invert(Image.open(ROOT/f'build/{name}.pbm').convert('L'))
    canvas.paste(im.resize((512,256),Image.Resampling.NEAREST),(16,i*300+30))
    ImageDraw.Draw(canvas).text((16,i*300+10),label,fill='white')
    im.save(ROOT/f'docs/{name}-128x64.png')
canvas.save(ROOT/'docs/setup-font-comparison.png')
