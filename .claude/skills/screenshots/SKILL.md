---
name: screenshots
description: Make the Microsoft Store screenshots for Screzy (coast, Preferences, tray menu, per-app rules) from real captures of the current app on a neutral scene. Use when the user asks for new or updated Store screenshots, listing images of the app, or says the app's look changed and the screenshots need redoing.
---

# Store screenshots

Makes four 4K screenshots (the screen's own size) in one consistent style: a soft blue wallpaper, a
headline on the left, an invented article in a plain window, and the **real** Screzy parts on top:
its Preferences window, tray menu, *Running apps…* menu and coast badge. Only the wallpaper,
captions, shadows, taskbar trimming and the pointer arrow are made by the script.

Output goes to `inbox/graphics/store-screenshots/` (git-ignored). Nothing reaches `store/listing/`
until the user has looked at the images and agreed.

## Before you run it

1. **Captions.** Read `scripts/shots.json` against `README.md` and the current Preferences. Every
   headline and sentence must describe what the app does *now*. Fix them first. Plain, friendly
   English, sentence case (`CLAUDE.md`, Style).
2. **Tell the user** what will happen: for about 30 seconds the pointer moves on its own, their
   Screzy is stopped, and a settings file with neutral per-app rules is swapped in. Both come back
   at the end (the script checks the settings file byte for byte). Ask them not to touch the mouse.
3. Requirements: Windows with Microsoft Edge, `gcc` and `windres` on PATH (w64devkit), Python with
   Pillow, and the taskbar on the primary screen at the bottom.

## Run

```sh
R=C:/Dev/Screzy; S=$R/.claude/skills/screenshots/scripts; W=$R/inbox/graphics/store-screenshots
python -I $S/make_demo_build.py $R $W/work/demo
powershell -ExecutionPolicy Bypass -File $S/capture.ps1 -Repo $R -Work $W/work
python -I $S/compose.py $R $W/work $W
```

`compose.py` can be re-run on its own after editing captions or layout. Only rerun the capture when
the app itself changed.

Then **look at every image** (Read the PNGs) before showing the user. Check:
- No personal data: no real app names, file names or browser profiles. The taskbar's clock, keyboard
  label (e.g. NOB) and the user's own tray icons do show: mention that.
- Text isn't clipped. The caption doesn't run into the window, and menus sit above the taskbar.
- The Preferences window is fully painted (no blank group boxes).
- The tray menu is the Store version's (*Start with Windows*, no *.ini* items).

## Hand-off

When the user approves: copy the PNGs to `store/listing/screenshot-<n>-<name>.png`, update the
numbered list under **Screenshots** in `store/README.md`, and commit. Remind them that screenshots
are uploaded by hand in Partner Center (Store listings → English → Screenshots). A new package
doesn't carry them (`RELEASING.md`, "What needs what").

## How it works, and what to fix when it breaks

- `make_demo_build.py` compiles a throwaway copy of `src/screzy.c` with three patches: a fixed
  *Running apps…* list of seven common apps, the Store tray menu, and *Start with Windows* shown as
  on. After a redesign a patch may stop matching. The script then stops and names the patch:
  update the search text there, keeping the intent. It also reads `ID_PICK`, `WM_TRAY` and the
  button text from the source.
- `capture.ps1` runs per-monitor DPI aware, so all coordinates are physical pixels. It finds windows
  by class (`ScrezyWnd`, `ScrezyPrefs`, menus `#32768`, visible ones only), not by title, because the
  Preferences title contains the version. It renders the article with headless Edge. **Never open
  a visible Edge window with a fresh profile**: Edge signs it in to the user's Microsoft account and
  syncs their extensions. The window frame is a plain WinForms window instead. If the taskbar
  auto-hides, the script points at the bottom edge to show it before the grab.
- Screzy ignores injected mouse input, so a real coast can't be started. The coast shot uses the
  app's own badge from `res/indicator/` at the offset `ind_tick()` uses (`size*9/16, size*5/8`
  from the pointer). If that code changes, update `compose.py`.
- The *Running apps…* menu goes at the centre of the button, because the app opens it at the click
  point.
- If the user runs Screzy from somewhere other than the Store or `build/`, the script restarts that
  same exe. Check that their Screzy is running again afterwards.
