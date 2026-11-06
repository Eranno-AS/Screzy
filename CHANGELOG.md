# Changelog

## 1.1.0 — 2026

- **Simpler Preferences:** one window with three sections, Scrolling, Coasting and Autoscroll, and new defaults that feel good straight away: a longer, floatier glide and more deliberate coasting. Existing settings are kept.
- **Softer endings:** glides and coasts now ease out to a full stop instead of stopping at a slow crawl. Fast scrolling also carries on a little after you stop, building up toward a coast.
- **Coast speed:** coasts can start at 0.25–2× the flick's speed, for calmer or livelier coasts that still follow how hard you flicked. The fixed starting speeds are gone.
- **Clicks you meant aren't lost:** a click where you were scrolling still only stops a coast, but a click on a tab, a button or another window now goes through. A coast also stops when you move to another window or switch apps.
- **Autoscroll:** a flick can start autoscroll instead of a coast. Steer it with the pointer, a RollerMouse bar or the wheel, which steps through a standstill into reverse. It replaces *Wait (ready)* and pointer steering on coasts.
- **Removed:** the Ready shortcut (Alt + middle click, Alt + G), so Screzy no longer reads any key, and the *Push* steering style. A few rarely used options moved to `screzy.ini`: `acceleration_starts_at`, `opposite_click_stops_coast` and `show_coast_indicator`.

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
