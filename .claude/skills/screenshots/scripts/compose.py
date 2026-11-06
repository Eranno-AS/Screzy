"""Compose the Store screenshots from the parts capture.ps1 grabbed.

Usage: python -I compose.py <repo> <work> <out_dir>
Canvas = the screen's physical size (3840x2160 at 4K/150%). Every window, menu and badge is pasted
at its real pixel size; only the wallpaper, captions, shadows and the drawn pointer are made here.
"""
import json, os, sys
from PIL import Image, ImageDraw, ImageFilter, ImageFont

REPO, WORK, OUT = sys.argv[1], sys.argv[2], sys.argv[3]
here = os.path.dirname(os.path.abspath(__file__))
meta = json.load(open(os.path.join(WORK, 'meta.json'), encoding='utf-8-sig'))
shots = json.load(open(os.path.join(here, 'shots.json'), encoding='utf-8'))['shots']
W, H = meta['screen']
S = meta['dpi'] / 96                                   # display scaling, for pointer and badge size
K = W / 3840                                           # layout scale relative to a 4K canvas
TBY = meta['taskbar'][1]                               # top of the taskbar
FONT = 'C:/Windows/Fonts/'
INK, MUTED, BRAND = (16, 24, 51), (88, 96, 120), (56, 76, 188)
M = int(150 * K)                                       # outer margin


def img(name):
    return Image.open(os.path.join(WORK, name)).convert('RGBA')


def font(bold, size):
    return ImageFont.truetype(FONT + ('segoeuib.ttf' if bold else 'segoeui.ttf'), int(size * K))


def wallpaper():
    top, bot = (236, 241, 252), (205, 216, 244)
    bg = Image.new('RGBA', (W, H))
    d = ImageDraw.Draw(bg)
    for y in range(H):
        t = y / (H - 1)
        d.line([(0, y), (W, y)], fill=tuple(int(a + (b - a) * t) for a, b in zip(top, bot)) + (255,))
    return bg


def rounded(im, r):
    m = Image.new('L', im.size, 0)
    ImageDraw.Draw(m).rounded_rectangle([0, 0, im.width - 1, im.height - 1], r, fill=255)
    out = im.copy()
    out.putalpha(Image.composite(im.getchannel('A'), m, m))
    return out


def paste_window(c, win, x, y, shadow=48, alpha=110):
    """Windows 11 look: 8 px (scaled) rounded corners and a soft drop shadow."""
    r, shadow = int(8 * S), int(shadow * K)
    win = rounded(win, r)
    sh = Image.new('RGBA', (win.width + shadow * 4, win.height + shadow * 4), (0, 0, 0, 0))
    ImageDraw.Draw(sh).rounded_rectangle([shadow * 2, shadow * 2, shadow * 2 + win.width, shadow * 2 + win.height], r, fill=(10, 20, 50, alpha))
    c.alpha_composite(sh.filter(ImageFilter.GaussianBlur(shadow / 2)), (x - shadow * 2, y - shadow * 2 + shadow // 3))
    c.alpha_composite(win, (x, y))


def doc_window(page):
    """The captured window frame (real title bar and border) with the rendered page in its client area."""
    win = img('doc-frame.png')
    x, y, w, h = meta['doc_client']
    win.paste(img(page).crop((0, 0, w, h)), (x, y))
    return win


def tray_icon(px):
    ico = Image.open(os.path.join(REPO, 'res', 'icons', 'screzy.ico'))
    sizes = sorted(ico.info.get('sizes', {(px, px)}))
    ico.size = min(sizes, key=lambda s: abs(s[0] - px))
    return ico.convert('RGBA').resize((px, px), Image.LANCZOS)


def taskbar(c):
    """Real taskbar, trimmed: Start (+ first pin) on the left, the tray on the right, Screzy's icon shown in it."""
    tb = img('taskbar.png')
    h = tb.height
    bar = tb.crop((tb.width // 2, 0, tb.width // 2 + 1, h)).resize(tb.size)   # an empty column, top border included
    bar.paste(tb.crop((0, 0, int(100 * S), h)), (0, 0))
    right = tb.crop((tb.width - int(280 * S), 0, tb.width, h))
    bar.paste(right, (tb.width - right.width, 0))
    ip = int(16 * S)
    ix = tb.width - right.width - int(23 * S)
    bar.alpha_composite(tray_icon(ip), (ix, (h - ip) // 2))
    c.alpha_composite(bar, (0, TBY))
    return ix + ip // 2


def pointer():
    """Standard white arrow with a black outline, at the display's scale."""
    pts = [(0, 0), (0, 17), (4, 13), (7, 20), (10, 19), (7, 12), (12, 12)]
    ss = 4
    k = S * 1.25 * ss
    im = Image.new('RGBA', (int(16 * k) + 8, int(24 * k) + 8), (0, 0, 0, 0))
    ImageDraw.Draw(im).polygon([(x * k + 4, y * k + 4) for x, y in pts], fill=(255, 255, 255, 255), outline=(0, 0, 0, 255), width=int(1.4 * ss))
    return im.resize((im.width // ss, im.height // ss), Image.LANCZOS)


def indicator(state='down'):
    """The app's own coast badge (res/indicator, premultiplied BGRA) at the display's size."""
    size = min((32, 40, 48, 64), key=lambda s: abs(s - 32 * S))
    raw = open(os.path.join(REPO, 'res', 'indicator', f'coast-{size}-{state}.bin'), 'rb').read()
    out = bytearray(len(raw))
    for i in range(0, len(raw), 4):
        b, g, r, a = raw[i:i + 4]
        if a:
            r, g, b = (min(255, v * 255 // a) for v in (r, g, b))
        out[i:i + 4] = bytes((r, g, b, a))
    return Image.frombytes('RGBA', (size, size), bytes(out)), size


def caption(c, shot, width):
    d = ImageDraw.Draw(c)
    x, y = M, int(560 * K)
    hf, sf, tf = font(True, 112), font(False, 54), font(False, 42)
    for line in shot['headline']:
        d.text((x, y), line, font=hf, fill=INK)
        y += int(138 * K)
    y += int(36 * K)
    line = ''
    for w in shot['body'].split():
        t = (line + ' ' + w).strip()
        if d.textlength(t, font=sf) > width and line:
            d.text((x, y), line, font=sf, fill=MUTED); y += int(76 * K); line = w
        else:
            line = t
    d.text((x, y), line, font=sf, fill=MUTED)
    y += int(76 * K)
    if shot.get('legend'):                              # enlarged badge with a short explanation
        badge, _ = indicator(shot.get('badge') or 'down')
        badge = badge.resize((int(120 * K), int(120 * K)), Image.LANCZOS)
        y += int(60 * K)
        c.alpha_composite(badge, (x, y))
        d.text((x + badge.width + int(36 * K), y + badge.height // 2 - int(34 * K)), shot['legend'][0], font=sf, fill=INK)
        d.text((x + badge.width + int(36 * K), y + badge.height // 2 + int(30 * K)), shot['legend'][1], font=tf, fill=MUTED)


def brand(c):
    ic = tray_icon(int(64 * K))
    c.alpha_composite(ic, (M, M))
    ImageDraw.Draw(c).text((M + int(84 * K), M + int(2 * K)), 'Screzy', font=font(True, 50), fill=BRAND)


def scene(shot, page, dim=False):
    c = wallpaper()
    brand(c)
    win = doc_window(page)
    wx, wy = W - win.width - M, M
    paste_window(c, win, wx, wy, alpha=70 if dim else 110)
    caption(c, shot, wx - M - int(60 * K))
    return c, wx, wy


def fit_bottom_right(c, im, extra_right=0):
    """Place a window bottom-right above the taskbar, leaving room for a menu that sticks out."""
    x = W - im.width - extra_right - int(260 * K)
    y = max(M, TBY - im.height - int(120 * K))
    return x, y


os.makedirs(OUT, exist_ok=True)
for shot in shots:
    kind = shot['kind']
    if kind == 'coast':
        c, wx, wy = scene(shot, 'page-mid.png')
        cx, cy = wx + int((W - M - wx) * 0.63), wy + int(760 * K)          # pointer over the text column
        if shot.get('badge'):                         # only where the app shows one (see SKILL.md)
            badge, size = indicator(shot['badge'])
            c.alpha_composite(badge, (cx + size * 9 // 16, cy + size * 5 // 8))   # same offset as ind_tick() in src/screzy.c
        c.alpha_composite(pointer(), (cx - 4, cy - 4))
    elif kind == 'prefs':
        c, _, _ = scene(shot, 'page-top.png', dim=True)
        pref = img('prefs.png')
        x, y = fit_bottom_right(c, pref)
        paste_window(c, pref, x, y, shadow=40, alpha=140)
    elif kind == 'tray':
        c, _, _ = scene(shot, 'page-top.png', dim=True)
    elif kind == 'picker':
        c, _, _ = scene(shot, 'page-top.png', dim=True)
        # The app opens this menu with its top-left corner at the click point (pick_running_app),
        # so it goes at the centre of the Running apps... button, over the clean Preferences capture.
        pref, menu = img('prefs.png'), img('pick-menu.png')
        pr, br, mr = meta['prefs'], meta['pick_button'], meta['pick_menu']
        dx, dy = (br[0] + br[2]) // 2 - pr[0], (br[1] + br[3]) // 2 - pr[1]
        x, y = fit_bottom_right(c, pref, max(0, dx + menu.width - pref.width))
        y = min(y, TBY - int(40 * K) - dy - menu.height)                    # keep the menu above the taskbar
        paste_window(c, pref, x, y, shadow=40, alpha=140)
        paste_window(c, menu, x + dx, y + dy, shadow=24, alpha=120)
        hx, hy = meta['pick_hover']
        c.alpha_composite(pointer(), (x + dx + hx - mr[0] - 4, y + dy + hy - mr[1] - 4))
    else:
        sys.exit(f'unknown kind {kind}')
    tray_x = taskbar(c)
    if kind == 'tray':
        menu = img('tray-menu.png')
        paste_window(c, menu, tray_x - menu.width // 2 - int(60 * K), TBY - menu.height - int(18 * K), shadow=24, alpha=120)
    c.convert('RGB').save(os.path.join(OUT, shot['file']))
    print('wrote', shot['file'])

# Plain Preferences window (defaults, no rules) for docs/preferences.png and website/assets/preferences.png.
# Kept at full size so it stays sharp on high-DPI screens; the pages set its display size.
img('prefs-plain.png').convert('RGB').save(os.path.join(OUT, 'preferences.png'), optimize=True)
print('wrote preferences.png')
