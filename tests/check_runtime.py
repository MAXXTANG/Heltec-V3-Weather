"""Validate manual page selection from a local serial capture."""
import argparse
import json
import re
from pathlib import Path
from PIL import Image

PAGE = re.compile(r'PAGE name=(\w+) switched=(\d) monotonic_ms=(\d+) time=(\S+) weatherValid=(\d) stale=(\d) network=(\d)')


def validate_manual_pages(text, min_changes=2):
    current = pending = None
    rows = []
    changes = 0
    for line in text.splitlines():
        button = re.fullmatch(r'BUTTON_PAGE name=(clock|weather)', line)
        if button:
            assert current is not None and pending is None, 'Unmatched button event'
            pending = button.group(1)
            assert pending != current, 'Button did not select the other page'
        match = PAGE.fullmatch(line)
        if not match:
            continue
        row = match.groups()
        name, switched = row[0], row[1] == '1'
        assert name in ('clock', 'weather'), 'Unknown page'
        if current is None:
            assert name == 'clock' and not switched, 'Boot must start on clock'
        elif switched:
            assert pending == name and name != current, 'Page switched without PRG'
            pending = None
            changes += 1
        else:
            assert name == current and pending is None, 'Background update changed the page'
        current = name
        rows.append(row)
    assert pending is None, 'Button event never reached the screen'
    assert changes >= min_changes, 'Capture at least two short PRG presses'
    return rows, changes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('log', type=Path)
    parser.add_argument('--output-dir', type=Path, default=Path('build/runtime'))
    args = parser.parse_args()
    text = args.log.read_text(errors='replace')
    assert 'page_mode=manual' in text, 'Capture a fresh manual-mode boot'
    assert not any(x in text for x in ('task_wdt:', 'Backtrace:', 'Guru Meditation', 'OLED_FRAME ok=0')), 'Crash or OLED failure'
    assert 'WEATHER_OK' in text and 'NTP_SYNCED' in text, 'Weather/NTP not verified'
    rows, changes = validate_manual_pages(text)
    assert int(rows[-1][2]) - int(rows[0][2]) >= 60000, 'Capture at least one minute'
    sleeps = re.findall(r'LIGHT_SLEEP elapsed_ms=(\d+) wake=(\d+) result=(\d+) wifiMode=(\d+)', text)
    assert sleeps and all(r[2] == '0' and r[3] == '0' for r in sleeps), 'Sleep/Wi-Fi verification failed'
    frames = {}
    current = None
    for line in text.splitlines():
        match = PAGE.fullmatch(line)
        if match:
            current = match.groups()
        elif line.startswith('FRAME_HEX=') and current and current[4] == '1':
            data = bytes.fromhex(line.split('=', 1)[1])
            assert len(data) == 1024
            frame = Image.new('L', (128, 64), 0)
            for y in range(64):
                for x in range(128):
                    frame.putpixel((x, y), 255 if data[x + (y // 8) * 128] & (1 << (y % 8)) else 0)
            frames[current[0]] = frame
    assert set(frames) == {'clock', 'weather'}, 'Need valid snapshots of both pages'
    # Battery voltage can change between presses; the location/date stay fixed.
    assert frames['clock'].crop((23, 0, 128, 11)).tobytes() == frames['weather'].crop((23, 0, 128, 11)).tobytes(), 'Location/date headers differ'
    args.output_dir.mkdir(parents=True, exist_ok=True)
    for name, frame in frames.items():
        frame.save(args.output_dir / f'manual-{name}-128x64.png')
        frame.resize((1024, 512), Image.Resampling.NEAREST).save(args.output_dir / f'manual-{name}.png')
    print(json.dumps({'manual_page_changes': changes, 'automatic_page_changes': 0, 'light_sleep_count': len(sleeps), 'valid_frames': list(frames)}, indent=2))


if __name__ == '__main__':
    main()
