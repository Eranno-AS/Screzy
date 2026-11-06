# Publishing Screzy to the Microsoft Store

This folder holds everything needed for the Store version: the package manifest template, logo images, the packaging script, and the listing text below for copying into Partner Center.

## 1. One-time setup

1. Use the **Eranno AS** company account in Partner Center (approved).
2. Choose **Apps and games → New product → MSIX or PWA app** and reserve the name **`Screzy – Easy, smooth scrolling`**. This is the listing name, with an en dash (–). Then, under **Product management → Manage app names**, also reserve plain **`Screzy`**, so no one else can take it.
3. Open **Product management → Product identity** and note three values:
   - `Package/Identity/Name` (e.g. `12345ErannoAS.Screzy`)
   - `Package/Identity/Publisher` (e.g. `CN=ABCDEF12-…`)
   - `Package/Properties/PublisherDisplayName` (should read `Eranno AS`)

   The package's display name must match a reserved name exactly. `pack.ps1` uses `Screzy – Easy, smooth scrolling` by default. Pass `-DisplayName "..."` if you reserve something else.

## 2. Build the package (on Windows)

Requirements: the Windows SDK (for `makeappx.exe`) and a built `build\Screzy.exe` (run `build.cmd`).

```powershell
.\store\pack.ps1 -IdentityName "<Package/Identity/Name>" -Publisher "<Package/Identity/Publisher>" -Version 1.0.0.0
```

This writes `store\out\Screzy_1.0.0.0_x64.msix`. The Store signs the package itself, so you don't need a certificate.

**Test the Store version locally first (optional, recommended).** Turn on *Developer Mode* (Settings → System → For developers), then run:

```powershell
.\store\pack.ps1 -IdentityName "ErannoAS.Screzy.Test" -Publisher "CN=ErannoTest" -RegisterLocally
```

Screzy then appears in the Start menu as a packaged app. Check these:

- **Start with Windows** in the tray menu is checked, and clicking it switches it off and on.
- Screzy is listed in Windows' Startup apps settings, matching the check mark.
- After you switch Screzy off on that Settings page, the tray item reads **Start with Windows…** and clicking it opens the page.
- Settings save between runs.

Remove the test install with `Get-AppxPackage *Screzy* | Remove-AppxPackage`.

**Or let GitHub build it.** Add the repository variables `STORE_IDENTITY_NAME` and `STORE_PUBLISHER` (Settings → Secrets and variables → Actions → Variables). The workflow then builds the .msix as a downloadable artifact on every push.

## 3. Submission

Create a submission in Partner Center and fill in these sections:

| Section | What to enter |
|---|---|
| Pricing and availability | **Free**, all markets, public visibility |
| Properties | Category **Utilities & tools**. Privacy policy URL `https://screzy.com/privacy`. Website `https://screzy.com`. Support contact: `info@screzy.com`. Product declarations: leave defaults |
| Age ratings | Answer the IARC questionnaire: no violence, no user interaction, no data sharing, no purchases. Expect a 3+ / Everyone rating |
| Packages | Upload `Screzy_1.0.0.0_x64.msix` |
| Store listings (English) | Copy the text from section 4 below, and upload images from `store/listing/` plus screenshots |
| Submission options → Restricted capabilities | Paste the `runFullTrust` justification below |
| Notes for certification | Paste the testing notes below |

**Screenshots.** At least one is required, as a PNG of 1366×768 or larger, taken on real Windows. The set in `store/listing/`, in this order:

1. `screenshot-1-coast.png`: a page coasting after a flick (normal coasts show no badge).
2. `screenshot-2-preferences.png`: the Preferences window, with its Scrolling, Coasting and Autoscroll sections, over a document.
3. `screenshot-3-tray.png`: the tray menu open.
4. `screenshot-4-per-app.png`: a per-app rule being added with **Running apps…**.

## 4. Listing text

**Product name:** Screzy – Easy, smooth scrolling

**Short description:**
Scroll easy. Flick to coast. Screzy turns every mouse-wheel click into a smooth glide in any Windows app, and keeps the page coasting when you flick, like scrolling on a phone.

**Description:**

> Scroll easy. Flick to coast.
>
> A normal mouse wheel scrolls in jumps. Screzy makes every app scroll smoothly: browsers, Office, PDF readers, code editors and more.
>
> Each wheel click becomes a smooth glide, timed to your screen's refresh rate. Spin faster and it accelerates, so long pages fly by while slow scrolling stays precise.
>
> Flick the wheel and the page keeps coasting, then gently runs down, like on a phone. Wheel clicks speed it up or slow it down, and a click stops it. Clicked a tab or a button instead? That click works as normal.
>
> Prefer hands-free reading? Let a flick start autoscroll, and steer it with the mouse, the wheel or a RollerMouse rollerbar.
>
> Good defaults out of the box, one clear Preferences window to fine-tune, and per-app rules for apps such as games.
>
> Screzy lives quietly in the system tray, uses no CPU while you're not scrolling, and never connects to the internet. No accounts, no ads, no tracking. It's free and open source.
>
> Tip: for the smoothest result, set Windows' "Lines to scroll at a time" to 1 (Settings → Bluetooth & devices → Mouse), or 3 if you prefer.

**What's new in this version:** A simpler Preferences window with better defaults. Glides and coasts ease out softly. Coasts can be calmer or livelier. Clicking a tab or a button during a coast now works first time. New optional autoscroll, steered with the mouse, the wheel or a RollerMouse bar.

**Product features:**
- Smooth, eased scrolling in every app
- Scroll acceleration when you spin the wheel faster
- Flick to coast, steer it with the wheel, click to stop
- Optional autoscroll with the mouse, wheel or RollerMouse rollerbar
- Per-app distance, or smoothing off per app
- Leaves touchpads, Ctrl + wheel zoom and admin windows alone
- No internet connection, no data collection
- Free and open source (MIT)

**Search terms:** smooth scrolling · mouse wheel · smooth scroll · autoscroll · momentum scrolling · scroll acceleration · RollerMouse

**Copyright and trademark info:** © 2026 Eranno AS. Screzy is not affiliated with Contour Design; RollerMouse is a trademark of Contour Design, Inc.

**Additional license terms:** MIT License: https://opensource.org/licenses/MIT

## 5. Text for reviewers

**runFullTrust justification:**

> Screzy is a Win32 desktop utility that smooths mouse-wheel scrolling system-wide. It needs runFullTrust to install a low-level mouse hook (WH_MOUSE_LL) that intercepts vertical wheel events, and to send the smoothed scrolling with SendInput. It uses no keyboard hook, collects no data and makes no network connections.

**Notes for certification:**

> Screzy runs in the system tray. On first launch (and whenever the app is launched again) the Preferences window opens. To test: open any long web page or document and scroll with a mouse wheel. Scrolling glides smoothly instead of jumping. Spin the wheel quickly (10 or more fast clicks) to see the page coast, then click where you were scrolling to stop it. That click intentionally only stops the coast and isn't passed to the window underneath. A click after moving the pointer away goes through normally. Optional autoscroll can be switched on in the Autoscroll section of Preferences. Use the "Try it" box in Preferences to test settings. The tray icon's right-click menu toggles smoothing, turns "Start with Windows" on and off, and exits the app. Touchpads and precision/high-resolution wheels are intentionally passed through unchanged, so testing requires a mouse with a standard notched scroll wheel. Screzy reads no keyboard input.
