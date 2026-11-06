<p align="center">
  <img src="res/icons/screzy.svg" width="96" alt="Screzy icon">
</p>

<h1 align="center">Screzy</h1>

<p align="center"><b>Scroll easy. Flick to coast.</b><br>
Easy, smooth mouse-wheel scrolling for Windows.<br>
Free and open source (MIT) · Windows 10 and 11 · Tiny, no installer needed</p>

---

A normal mouse wheel scrolls in jumps. Screzy turns each click into a smooth glide in every app. Flick the wheel and the page keeps coasting, like on a phone. A click stops it.

<p align="center"><img src="docs/preferences.png" width="480" alt="Screzy Preferences window"></p>

## Features

- **Smooth glides.** Each click eases in and out, timed to your screen's refresh rate.
- **Acceleration.** Spin faster and each click goes further. Slow scrolling stays precise.
- **Flick to coast.** The page keeps gliding, then gently runs down. Wheel clicks speed it up or slow it down. A click where you were scrolling only stops it. Move away to click a tab or a button, and that click works as normal.
- **Autoscroll (optional).** Let a flick start autoscroll instead: move the pointer, a RollerMouse bar or the wheel to set the speed and direction.
- **Per-app rules.** Give an app its own scroll distance, or turn smoothing off for it, for example in games.
- **Stays out of the way.** Touchpads, high-resolution wheels, Ctrl + wheel zoom, sideways scrolling and admin windows are left alone.
- **Light and private.** Uses no CPU while you're not scrolling. No internet, no tracking ([privacy statement](PRIVACY.md)).

## Install

**Microsoft Store:** [get Screzy from the Store](https://apps.microsoft.com/detail/9PFTRF7TP5PS). It's the easiest option, with automatic updates and no security warnings.

**Download:** grab `Screzy.exe` from the [latest release](../../releases/latest) and run it. There's nothing to install. The file isn't code-signed, so Windows may warn you the first time: choose **More info → Run anyway**.

## Using it

Screzy lives in the **system tray**. Double-click its icon, or start it again, to open **Preferences**. Right-click it to switch smoothing off (the icon turns grey) or to set **Start with Windows**.

The defaults are tuned to feel good straight away. Preferences is there to fine-tune:

| Section | Settings | Defaults |
|---|---|---|
| Scrolling | Distance per wheel click, Glide duration, Acceleration, Max acceleration | 1×, 800 ms, 1.2, 6× |
| Coasting | Coast length, Coast speed, Clicks needed to coast, Flick speed to coast, Max coast speed | 7 s, 1× the flick, 10 clicks, 8 clicks/s, 60 lines/s |
| Autoscroll | *A flick starts autoscroll instead of a coast*, Autoscroll starts at, Autoscroll sensitivity | Off, 10 lines/s, 1.0× |

The **Try it** box lets you feel a change before you keep it. **Per-app rules:** pick an app with **Running apps…**, set **Distance ×** or tick **Disable smoothing**, then press **Add / Update** and **Apply**.

### Coasting

- Flick the wheel: at least 10 fast clicks in a row, so a coast only starts when you mean it.
- Wheel clicks in the same direction speed it up. Clicks the other way slow it down and then stop it. Once you adjust a coast, it holds its speed.
- For a calm reading pace, set **Coast speed** to 0.25–0.5×. Set **Coast length** to *Until stopped* to keep it going until you click.
- A coast stops when you click, move to another window or switch apps.

### Autoscroll

Tick **A flick starts autoscroll instead of a coast**, then flick:

- The page stops, a small badge marks the start point, and autoscroll sets off in the flick's direction.
- Move the pointer (or roll a RollerMouse bar) below the badge to scroll down, above it to scroll up, and further away to go faster.
- Or use the wheel: each click is one step faster or slower, through a standstill into reverse.
- A click ends it.

### Tip: set Windows to 1 line per click

In **Settings → Bluetooth & devices → Mouse**, set **Lines to scroll at a time** to **1** (or 3 if you prefer), then adjust **Distance per wheel click** in Screzy. Smaller Windows steps make smoother glides.

### More settings

A few rarely used options are only in the settings file, `%APPDATA%\Screzy\screzy.ini`:

- `acceleration_starts_at=5`: the wheel speed, in clicks per second, where acceleration begins.
- `opposite_click_stops_coast=1`: one opposite wheel click stops a coast straight away.
- `show_coast_indicator=1`: show the badge for every coast, not just in autoscroll.

## Good to know

- **Some apps scroll by whole lines**, such as older programs and plain text boxes, so they look steppier than a browser. Add a per-app rule if one misbehaves.
- **Turn off other smooth scrolling,** including in your mouse software, so the two don't double up.

## How it works

A low-level mouse hook runs on its own high-priority thread and lets ordinary pointer movement straight through. Each real wheel click (a multiple of 120 units) is swallowed and added to a target distance. An animation thread moves toward that target with a critically damped spring, sending small wheel steps with `SendInput` once per display frame. A flick hands its speed to a coast that decays exponentially and eases out at the end. Everything is plain C and Win32 with no dependencies; see [`src/screzy.c`](src/screzy.c).

## Building

You need MinGW-w64.

- **Linux / macOS / WSL:** `sudo apt install mingw-w64` (or `brew install mingw-w64`), then `./build.sh`
- **Windows:** install [w64devkit](https://github.com/skeeto/w64devkit) or MSYS2's `mingw-w64-x86_64-gcc`, then run `build.cmd`

The result is `build/Screzy.exe`, a single static executable. The Store package is built with `store/pack.ps1` (see [`store/README.md`](store/README.md)). The website is in [`website/`](website/), and release steps are in [RELEASING.md](RELEASING.md).

## Name

Screzy was called *GlideScroll* during development. On first start, it carries over GlideScroll's settings and start-with-Windows entry, and closes a running GlideScroll.

Screzy is not affiliated with Contour Design. RollerMouse is a trademark of Contour Design, Inc.

## License

[MIT](LICENSE) © 2026 Eranno AS
