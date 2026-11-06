# Changelog

## Unreleased

- **Softer endings:** a coast that runs down now eases out to a full stop over its last second, instead of stopping suddenly at a slow crawl. Ordinary scrolling eases out the same way, which shows most with a long glide duration.
- **Momentum:** fast scrolling that isn't quite a flick now carries on a little after the wheel stops, more the faster and longer you spin, so scrolling builds up gradually toward a coast. Coasts themselves still need the full flick. A click stops the momentum and still goes through as a normal click.
- **Simpler Preferences:** one window with three clear sections, Scrolling, Coasting and Autoscroll, plus per-app rules. Several settings are gone or moved to the settings file (see below). *Flick coasting* is now called *Coast length*. Glide duration now goes up to 1200 ms. *Acceleration starts at* is now only in `screzy.ini` (`acceleration_starts_at`).
- **New defaults** for new installs, for a floatier feel and more deliberate coasting: glide 800 ms, acceleration 1.2 (max 6×, starting at 5 clicks/s), coast length 7 s, a flick needs 10 clicks at 8 clicks/s or faster, max coast speed 60 lines/s. *Keep a coast I've adjusted until I click* and the coast indicator are now off by default. Existing settings are kept.
- **Clicks you meant aren't lost:** a click where you were scrolling still only stops a coast. But if you've moved the pointer away first, to a browser tab, a button or an email, the click stops the coast and still goes through. A coast also stops when the pointer moves onto another window or you switch apps, instead of quietly scrolling whatever is under the pointer.
- **Coast speed:** the old *Coast starting speed* slider is now *Coast speed*, and a separate checkbox, *A flick starts autoscroll instead of a coast*. Coasts can now start at 0.25–2× the flick's speed, calmer or livelier while still following how hard you flicked. The fixed starting speeds are gone, since a calmer coast or autoscroll does the same job better. A saved fixed speed now means a normal coast.
- **Autoscroll:** pointer steering is now its own mode, replacing *Wait (ready)*. With *A flick starts autoscroll instead of a coast* ticked, a flick stops the page and starts autoscroll at the pointer: moving the pointer (or a RollerMouse bar) below or above the start point scrolls, faster the further you go, until you click. The wheel steers it too, one speed step per click, down through a standstill and into reverse, so it also works without a rollerbar. *Autoscroll sensitivity* sets how far to move for full speed, and *Autoscroll starts at* sets the pace it sets off at (standstill to 10 lines/s, 10 by default). Ordinary coasts are steered with the wheel only, so moving the pointer to click something no longer changes their speed. The *Push* steering style is gone.
- **Coast speed works as set:** the last few clicks of a flick no longer count as nudges that speed up the coast.
- **Ready shortcut removed:** Alt + middle click and Alt + G are gone, since a flick now starts autoscroll. Screzy no longer looks at any key at all.
- **Fewer options:** *Opposite wheel click stops a coast*, *Keep steered coasts until I click*, *Show coast indicator* and *Steer coasts with mouse / rollerbar* are no longer in Preferences. The indicator now shows in autoscroll and when a flick waits in Ready. `opposite_click_stops_coast=1` and `show_coast_indicator=1` still work in `screzy.ini`.

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
