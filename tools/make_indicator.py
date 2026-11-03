"""Render the coast indicator SVGs to raw premultiplied BGRA (top-down) for the .exe.
Run:  python3 tools/make_indicator.py res/icons res/indicator   (needs cairosvg + Pillow)"""
import sys, io, cairosvg
from PIL import Image
src, out = sys.argv[1], sys.argv[2]
for size in (32, 40, 48, 64):                 # 100%, 125%, 150%, 200% display scaling
    for state in ('ready', 'down', 'up'):
        png = cairosvg.svg2png(url=f'{src}/coast-light-{state}.svg', output_width=size, output_height=size)
        im = Image.open(io.BytesIO(png)).convert('RGBA')
        px = im.tobytes()
        buf = bytearray(len(px))
        for i in range(0, len(px), 4):                                   # premultiplied BGRA
            r, g, b, a = px[i], px[i + 1], px[i + 2], px[i + 3]
            buf[i:i + 4] = bytes((b * a // 255, g * a // 255, r * a // 255, a))
        open(f'{out}/coast-{size}-{state}.bin', 'wb').write(buf)
print('ok')
