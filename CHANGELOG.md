# Changelog

## Unreleased

- **Softer endings:** a coast that runs down now eases out to a full stop over its last second, instead of stopping suddenly at a slow crawl. Ordinary scrolling eases out the same way, which shows most with a long glide duration.
- **Momentum:** fast scrolling that isn't quite a flick now carries on a little after the wheel stops, more the faster and longer you spin, so scrolling builds up gradually toward a coast. Coasts themselves still need the full flick. A click stops the momentum and still goes through as a normal click.

## 1.0.1 — 2026

- **Store version:** *Start with Windows* in the tray menu now shows a check mark when it's on, and clicking it switches it on or off. If you've turned Screzy off on Windows' Startup apps page, the item reads *Start with Windows…* and opens that page, because only you can turn it back on there.
- **Settings file edits show up:** if you edit `screzy.ini` in a text editor, Screzy now notices when you open the tray menu or Preferences and reads the new values. *Reload settings* also refreshes an open Preferences window, and a changed Ready shortcut now takes effect without a restart.

## 1.0.0 — 2026

First public release, published by Eranno AS. (Called *GlideScroll* during development; Screzy carries over its settings on first start.)

- **Smooth scrolling:** each wheel click becomes a smooth glide in any app.
- **Smooth timing:** the glide eases in and out, and its steps are timed to your monitor's refresh rate.
- **Acceleration:** spinning the wheel faster scrolls further per click, then eases off as you slow down.
- **Flick coasting:** flick the wheel fast and the page keeps going, then slowly runs down. A left, middle or right click stops it without clicking whatever is under the pointer.
- **Coast steering:** while coasting, same-direction wheel clicks speed it up and opposite clicks slow it down (or stop it, as an option). Optional pointer steering with a RollerMouse rollerbar or mouse comes in two styles: *Distance from start*, where speed follows how far the pointer is from where the coast began (like middle-click autoscroll), and *Push*, where rolling adds or takes speed while the pointer stays put. Steered coasts hold their speed for hands-free reading and, by default, keep going until a mouse click, even through a standstill when reversing.
- **Max coast speed:** a setting that caps how fast a flick can coast.
- **Coast indicator:** a small click-through badge beside the pointer shows whether a coast is moving up, moving down or standing ready. It can be switched off in Preferences.
- **Coast starting speed:** optionally start every coast at a fixed, calm pace (from 1 line/s) instead of the flick's speed. Useful as a reading auto-scroll. *Wait (ready)* stops the page after a flick and waits for the wheel or rollerbar.
- **Ready shortcut:** Alt + middle click or Alt + G enters Ready at the pointer without scrolling anything. Off by default.
- **Flick trigger settings:** both the wheel speed and the number of fast clicks in a row needed to start a coast can be set, so brisk normal scrolling doesn't coast by accident.
- **Per-app rules:** set a different scroll distance (Distance ×) for an app, or turn smoothing off for it, picking from a list of running apps.
- **Preferences window:** live sliders and a test area to try settings in.
- **Responsive pointer:** the mouse hook runs on a dedicated high-priority thread, and plain pointer movement passes straight through, so pointer movement stays crisp.
- **Tray icon:** shows whether smoothing is on or off, with a menu to toggle it. It comes back by itself if Windows Explorer restarts.
- **Leaves some input untouched:** touchpads and high-resolution wheels, Ctrl/Alt/Win + wheel, sideways scrolling, and windows running as administrator.
- **Two editions:** a standalone .exe, and a Microsoft Store version that uses Windows' own setting for starting at sign-in.
