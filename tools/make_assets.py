"""Generate the raster icons, Store tiles and listing images from one master PNG.
Run:  python tools/make_assets.py inbox/graphics/master.png            (needs Pillow)
      python tools/make_assets.py master.png --out C:/tmp/preview      (write somewhere else first)
      python tools/make_assets.py master.png --only tiles              (icons | tiles | listing)

The master is the finished app icon: square, at least 1024 px, ideally with transparent corners.
Writes, relative to the repo root (or to --out):
  res/icons/screzy.ico, screzy-off.ico    embedded in the exe, so they need a new build
  store/Assets/*.png                      package tiles, so they need a new Store version
  store/listing/*.png                     Store listing images, so a listing update
Not generated: website/assets/screzy.svg, website/assets/og-image.png, docs/preferences.png
(made by hand) and the coast indicators (tools/make_indicator.py)."""
import argparse, os, sys
from PIL import Image, ImageOps

ICO_SIZES = (16, 20, 24, 32, 40, 48, 64, 256)
TARGET_SIZES = (16, 24, 32, 48, 256)          # Square44x44Logo.targetsize-N
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def fit(master, w, h, scale, bg=None):
    """The master centred on a w x h canvas, `scale` of the shorter side, on a transparent or solid background."""
    side = round(min(w, h) * scale)
    canvas = Image.new('RGBA', (w, h), bg or (0, 0, 0, 0))
    icon = master.resize((side, side), Image.LANCZOS)
    canvas.alpha_composite(icon, ((w - side) // 2, (h - side) // 2))
    return canvas if bg is None else canvas.convert('RGB')


def grey(im):
    """The 'smoothing off' look: same shape, no colour."""
    g = ImageOps.grayscale(im.convert('RGB')).convert('RGBA')
    g.putalpha(im.getchannel('A'))
    return g


def save(im, root, rel):
    path = os.path.join(root, rel)
    os.makedirs(os.path.dirname(path), exist_ok=True)
    im.save(path, format='ICO', sizes=[(s, s) for s in ICO_SIZES]) if rel.endswith('.ico') else im.save(path)
    print('wrote', rel)


def make_icons(master, root):
    base = master.resize((256, 256), Image.LANCZOS)
    save(base, root, 'res/icons/screzy.ico')
    save(grey(base), root, 'res/icons/screzy-off.ico')


def make_tiles(master, root):
    a = 'store/Assets/'
    save(fit(master, 150, 150, 1), root, a + 'Square150x150Logo.png')
    save(fit(master, 44, 44, 1), root, a + 'Square44x44Logo.png')
    save(fit(master, 50, 50, 1), root, a + 'StoreLogo.png')
    save(fit(master, 310, 150, 0.8), root, a + 'Wide310x150Logo.png')
    for n in TARGET_SIZES:
        save(fit(master, n, n, 1), root, f'{a}Square44x44Logo.targetsize-{n}.png')
        save(fit(master, n, n, 1), root, f'{a}Square44x44Logo.targetsize-{n}_altform-unplated.png')


def make_listing(master, root, bg):
    l = 'store/listing/'
    save(fit(master, 300, 300, 1), root, l + 'logo-300.png')
    save(fit(master, 1080, 1080, 0.94), root, l + 'boxart-1x1-1080.png')
    save(fit(master, 720, 1080, 0.55, bg), root, l + 'poster-2x3-720x1080.png')


def main():
    p = argparse.ArgumentParser(description='Generate icons, tiles and listing images from a master PNG.')
    p.add_argument('master', help='square PNG, at least 1024 px')
    p.add_argument('--out', default=ROOT, help='root folder to write into (default: the repo)')
    p.add_argument('--only', choices=('icons', 'tiles', 'listing'), help='generate just one group')
    p.add_argument('--bg', default='#F3F5FB', help='poster background colour (default: %(default)s)')
    args = p.parse_args()

    master = Image.open(args.master).convert('RGBA')
    if master.width != master.height:
        sys.exit(f'The master must be square, this one is {master.width}x{master.height}.')
    if master.width < 1024:
        print(f'Warning: the master is only {master.width} px. 1024 px or more gives sharper results.')
    if master.getchannel('A').getextrema()[0] == 255:
        print('Note: the master has no transparency, so every tile and icon will have square corners.')

    if args.only in (None, 'icons'):
        make_icons(master, args.out)
    if args.only in (None, 'tiles'):
        make_tiles(master, args.out)
    if args.only in (None, 'listing'):
        make_listing(master, args.out, args.bg)
    print('ok')


if __name__ == '__main__':
    main()
