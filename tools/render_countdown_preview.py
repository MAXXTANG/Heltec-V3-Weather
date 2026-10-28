"""Export native countdown frames as a real-time demo and a fireworks excerpt."""
from pathlib import Path
from PIL import Image, ImageOps, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'docs/images'
paths = sorted((ROOT / 'build/countdown-frames').glob('frame-*.pbm'))
assert len(paths) == 855, 'Run sh tests/run.sh to render all 34.2 seconds first'
native = [ImageOps.invert(Image.open(path).convert('L')) for path in paths]
frames = [frame.resize((512, 256), Image.Resampling.NEAREST) for frame in native]
normal = Image.open(OUT / 'ui-clock-128x64.png').convert('L').resize((512, 256), Image.Resampling.NEAREST)
frames[0].save(OUT / 'countdown-demo.gif', save_all=True,
               append_images=frames[1:] + [normal], duration=[40] * 855 + [1200],
               loop=0, disposal=2, optimize=True)
frames[755].save(OUT / 'fireworks-demo.gif', save_all=True,
                 append_images=frames[756:], duration=40, loop=0, disposal=2, optimize=True)

font = ImageFont.truetype(ROOT / 'third_party/fonts/BarlowCondensed-Bold.ttf', 21)
title = ImageFont.truetype(ROOT / 'third_party/fonts/BarlowCondensed-Bold.ttf', 27)
sheet = Image.new('RGB', (568, 558), '#eeeef0')
draw = ImageDraw.Draw(sheet)
draw.text((20, 10), '30-SECOND COUNTDOWN / NATIVE OLED PREVIEW', fill='#151515', font=title)
examples = [(0, '30:00 / START'), (375, '15:00 / 1 Hz BLINK'),
            (625, '05:00 / 4 Hz BLINK'), (750, '00:00 / FINISH'),
            (780, 'FIREWORKS / 1.0 s'), (825, 'FIREWORKS / 2.8 s')]
for index, (frame_index, label) in enumerate(examples):
    x = 20 + (index % 2) * 272
    y = 50 + (index // 2) * 166
    sheet.paste(native[frame_index].resize((256, 128), Image.Resampling.NEAREST).convert('RGB'), (x, y))
    draw.text((x, y + 132), label, fill='#151515', font=font)
sheet.save(OUT / 'countdown-preview.png')
for name, expected_ms in [('countdown-demo.gif', 35400), ('fireworks-demo.gif', 4000)]:
    with Image.open(OUT / name) as gif:
        duration = 0
        for index in range(gif.n_frames):
            gif.seek(index)
            duration += gif.info['duration']
        assert duration == expected_ms, (name, duration)
print('Saved countdown demo (30s + 0.2s zero + 4s fireworks + normal page) and 4s fireworks excerpt.')
