<p align="center">
  <img src="res/icons/screzy.svg" width="96" alt="Screzy icon">
</p>

<h1 align="center">Screzy</h1>

<p align="center"><b>Scroll easy. Flick to coast. Steer with mouse or rollerbar.</b><br>
Easy, smooth mouse-wheel scrolling for Windows.<br>
Free and open source (MIT) · Windows 10 and 11 · Tiny, no installer needed</p>

---

A normal mouse wheel scrolls in jumps. Screzy turns each click into a smooth glide in every app: browsers, Office, PDF readers, code editors. Flick the wheel fast and the page keeps coasting, like a phone or a free-spinning wheel. A click stops it.

<p align="center"><img src="docs/preferences.png" width="480" alt="Screzy Preferences window"></p>

## Features

- **Smooth glides.** Each wheel click eases in and out instead of jumping. Steps are timed to your monitor's refresh rate (60, 120, 144 Hz…), so the motion is even.
- **Acceleration.** Spin faster and each click goes further. Slow, careful scrolling stays precise.
- **Flick coasting.** Flick the wheel and let go, and the page keeps gliding at that speed, then gently runs down. A left, middle or right click stops it, and only stops it: that click isn't passed on, so it can't open a link or press a button by accident.
- **Steer a coast.** While it coasts, a wheel click in the same direction speeds it up and one the other way slows it down. Optionally, roll a RollerMouse bar (or move the mouse) up and down to steer its speed and direction smoothly. Once you steer, the coast holds that speed, which makes a hands-free reading scroll.
- **Coast indicator.** While a page coasts, a small badge beside the pointer shows its direction, or that it's ready but standing still.
- **Per-app rules.** Give an app its own scroll distance, or turn smoothing off for it (handy for games). Pick from a list of running apps.
- **Stays out of the way.** Touchpads and high-resolution wheels, Ctrl + wheel zoom, sideways scrolling and windows running as administrator are left alone.
- **Lightweight.** A single small `.exe` that uses no CPU while you're not scrolling. No network access, no telemetry ([privacy statement](PRIVACY.md)).

## Install

**Microsoft Store:** [get Screzy from the Store](https://apps.microsoft.com/detail/9PFTRF7TP5PS?mode=direct). It's the easiest option. Updates are automatic and there are no security warnings.

**Download:** grab `Screzy.exe` from the [latest release](../../releases/latest) and run it. Nothing to install; it lives in the system tray. Because the download isn't code-signed, Windows may show a SmartScreen warning on first run. Choose **More info → Run anyway**, or right-click the file → **Properties → Unblock** first.

## Using it

- Screzy runs in the **system tray**. Double-click the icon, or launch the app again, to open **Preferences**. Right-click it for the menu.
- The icon turns **grey** when smoothing is switched off.
- **Start with Windows** is in the tray menu, with a check mark when it's on. In the Store version it switches Windows' own startup setting on and off. If you've turned Screzy off on Windows' Startup apps page, only you can turn it back on there, so the item opens that page instead.

### Settings

| Setting | What it does | Default |
|---|---|---|
| Distance per wheel click | How far one click scrolls, shown in lines | 1× your Windows setting |
| Glide duration | How long each glide lasts. Longer feels floatier, shorter feels snappier | 260 ms |
| Acceleration | How much faster scrolling gets as you spin quicker | 0.5 |
| Max acceleration | Upper limit for acceleration | 4× |
| Acceleration starts at | Wheel speed where acceleration begins | 6 clicks/s |
| Flick coasting | How long a flick keeps gliding: Off, 0.5–10 s, or *Until stopped* | 3 s |
| Coast starting speed | *As flicked* keeps going at the speed you flicked. *Wait (ready)* makes a flick stop the page and wait: the wheel or rollerbar then starts it. A fixed value (1–100 lines/s, in fine steps at the slow end) eases the coast down to that pace | As flicked |
| Flick speed to coast | How fast you must spin to trigger coasting | 15 clicks/s |
| Clicks needed to coast | How many fast clicks in a row a flick needs. Raise it if coasts start when you don't want them | 4 clicks |
| Max coast speed | Upper limit for coasting speed, or *No limit* | 120 lines/s |
| Steering sensitivity | How strongly pointer movement changes a coast's speed, in both steering styles | 1.0× |
| Steer coasts with mouse / rollerbar | *Push*: rolling up or down changes the coast's speed while the pointer stays put. *Distance from start*: the pointer moves freely and the speed follows how far it is above or below where the coast started (see below) | Off |
| Keep steered coasts until I click | A coast you've steered never ends by itself, even at a standstill, so you can roll slowly through zero to change direction. Only a mouse click ends it | On |
| Opposite wheel click stops a coast | Stop instantly instead of slowing down a step | Off |
| Show coast indicator by the pointer | A small badge beside the pointer while a coast runs: dot and both arrows when it's at a standstill, dot and an arrow for the direction it's moving | On |
| Ready shortcut | *Alt + middle click* or *Alt + G* puts Screzy straight into Ready at the pointer, with nothing scrolled. The wheel or rollerbar then starts the coast and a click ends it. *Alt + G* is unavailable to other apps while Screzy runs | Off |

### Steering a coast

Flick the wheel to start a coast, then adjust it while it's moving:

- **Wheel:** each click in the coast's direction speeds it up a step. Each click the other way slows it down a step, and a slow coast stops.
- **Pointer (optional),** with two styles:
  - *Distance from start*, like a browser's middle-click autoscroll: the indicator stays where the coast began. Move the pointer below it to scroll down, above it to scroll up, further away to go faster, and back onto it to stand still. Your whole speed range is spread over the distance: about 1 line/s just outside the start, rising steadily to your *Max coast speed* about 280 px away. *Steering sensitivity* makes that distance shorter or longer.
  - *Push*: the pointer stays at the same height, and each roll up or down adds to or takes from the speed. A small dead zone ignores accidental touches.
- **Start slow:** set *Coast starting speed* to a calm reading pace. Try 2–5 lines/s for slow, hands-free reading, or 15–30 lines/s for skimming. A flick then just starts the scroll in that direction at that pace, ready to steer. For a coast that keeps going until you click, also set *Flick coasting* to *Until stopped*.
- **Start without moving the page:** set *Coast starting speed* to *Wait (ready)*. A flick then brings the page to a stop and shows the Ready indicator, and the first deliberate wheel click or rollerbar roll sets it moving. Late clicks from the flick itself are ignored. The page moves a little during the flick, because Screzy can only recognise a flick after a few clicks. For no movement at all, use the *Ready shortcut* instead. With a RollerMouse, you can map one of its buttons to *Alt + G* in the Contour software.
- **Any steering holds the speed.** The coast stops running down, so you can read at a steady pace. A left, middle or right click stops it. With *Keep steered coasts until I click* on (the default), a steered coast never ends by itself, even at a standstill, so you can roll slowly through zero to reverse. Turn it off, and a steered coast left at a standstill ends after a second.

### Per-app rules

Pick an app with **Running apps…** (or type its `.exe` name), then:

- **Distance ×** multiplies *Distance per wheel click* for that app only. Use 2.0 to scroll twice as far per click (handy in some PDF readers or Excel), or 0.5 for half. Coasts started in that app scale with it.
- **Disable smoothing** leaves that app's scrolling completely untouched, which is useful for games.

Press **Add / Update**, then **Apply**.

### Tip: set Windows to 1 line per click

Apps multiply every scroll step by Windows' own **lines per click** setting. The smaller that setting, the finer each glide step, and the smoother scrolling looks. If it's set high (some mouse software sets 10–20 lines), each tiny step becomes a jump of several pixels and scrolling looks jagged. For the smoothest result:

1. **Settings → Bluetooth & devices → Mouse → Lines to scroll at a time:** set it to **1**. Use **3** if you prefer how some apps feel with it.
2. Raise **Distance per wheel click** in Screzy until the distance feels right.

## Good to know

- **Apps that scroll by whole lines** (some older programs, and plain Windows text boxes) can't show sub-line steps, so they look steppier than browsers. Add a per-app rule if one misbehaves.
- **Other mouse software:** if your mouse software has its own smooth scrolling or a scroll-speed setting, turn those off or down to avoid doubling up.
- **Don't run it alongside other smooth-scrolling tools.**

## How it works

Screzy installs a low-level mouse hook on its own highest-priority thread, so the pointer never waits on the rest of the app. Ordinary pointer movement leaves the hook immediately. Each real wheel click (a multiple of 120 units) is swallowed and added to a target distance. An animation thread then moves toward that target with a critically damped spring, sending small wheel steps with `SendInput`, one per display frame, locked to the monitor's refresh rate. A minimum step size keeps the end of each glide even. Acceleration comes from the smoothed rate of wheel clicks. A flick hands its current speed to a coast that decays exponentially, unless the user steers it with wheel clicks or vertical pointer movement, which then sets and holds the coast's speed. Everything is plain C and Win32 with no dependencies; see [`src/screzy.c`](src/screzy.c).

## Building

You need MinGW-w64.

- **Linux / macOS / WSL:** `sudo apt install mingw-w64` (or `brew install mingw-w64`), then `./build.sh`
- **Windows:** install [w64devkit](https://github.com/skeeto/w64devkit) or MSYS2's `mingw-w64-x86_64-gcc`, then run `build.cmd`

The result is `build/Screzy.exe`, a single static executable. The Store (MSIX) package is built with `store/pack.ps1`; see [`store/README.md`](store/README.md). The screzy.com website lives in [`website/`](website/), and the release steps are in [PUBLISHING.md](PUBLISHING.md).

## Privacy

Screzy collects nothing and makes no network connections. Settings stay in one local file. Full details are in [PRIVACY.md](PRIVACY.md).

## Name

Screzy was called *GlideScroll* during development. When you first start Screzy, it carries over GlideScroll's settings and start-with-Windows entry, and closes a running GlideScroll.

Screzy is not affiliated with Contour Design. RollerMouse is a trademark of Contour Design, Inc.

## License

[MIT](LICENSE) © 2026 Eranno AS
