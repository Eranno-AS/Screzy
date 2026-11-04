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
| Properties | Category **Utilities & tools**. Privacy policy URL `https://screzy.com/privacy`. Website `https://screzy.com`. Support contact: your support page or email. Product declarations: leave defaults |
| Age ratings | Answer the IARC questionnaire: no violence, no user interaction, no data sharing, no purchases. Expect a 3+ / Everyone rating |
| Packages | Upload `Screzy_1.0.0.0_x64.msix` |
| Store listings (English) | Copy the text from section 4 below, and upload images from `store/listing/` plus screenshots |
| Submission options → Restricted capabilities | Paste the `runFullTrust` justification below |
| Notes for certification | Paste the testing notes below |

**Screenshots.** At least one is required, as a PNG of 1366×768 or larger, taken on real Windows. Good choices:

1. The Preferences window over a web page.
2. The tray menu open.
3. A per-app rule being added with **Running apps…**.

## 4. Listing text

**Product name:** Screzy – Easy, smooth scrolling

**Short description:**
Scroll easy. Flick to coast. Steer with mouse or rollerbar. Screzy turns every mouse-wheel click into a smooth glide in any Windows app, speeds up when you spin faster, and keeps coasting when you flick, like scrolling on a phone.

**Description:**

> Scroll easy. Flick to coast. Steer with mouse or rollerbar.
>
> A normal mouse wheel scrolls in jumps. Screzy makes every app scroll smoothly: browsers, Office, PDF readers, code editors and more.
>
> Each wheel click becomes a smooth glide that eases in and out, timed to your monitor's refresh rate, so the motion stays fluid on 60, 120 and 144 Hz screens. Spin the wheel faster and it accelerates, so long pages go by quickly while slow scrolling stays precise.
>
> Flick the wheel and let go: the page keeps coasting and gently runs down, just like on a phone. While it coasts, steer it. Wheel clicks speed it up or slow it down. Or move the mouse, or the rollerbar on a Contour RollerMouse, away from where the coast started: the further you go, the faster it scrolls. A small indicator by the pointer shows the direction, and a click stops it.
>
> For hands-free reading, start coasts at a calm fixed pace, or let a flick wait until you steer. Fine-tune everything in one Preferences window, and give individual apps their own distance, or switch smoothing off for them, such as games.
>
> Screzy lives quietly in the system tray, uses no CPU while you're not scrolling, and never connects to the internet. No accounts, no ads, no tracking. It's free and open source.
>
> Tip: for the smoothest result, set Windows' "Lines to scroll at a time" to 1 (Settings → Bluetooth & devices → Mouse), or 3 if you prefer, then adjust the distance in Screzy.

**What's new in this version:** First release.

**Product features:**
- Smooth, eased scrolling for every app
- Steps timed to your monitor's refresh rate
- Scroll acceleration when you spin the wheel faster
- Flick to coast, with a click to stop
- Steer a coast's speed with the wheel or rollerbar
- Coast indicator by the pointer shows direction
- Per-app speed, or smoothing off per app
- Adjustable distance, glide time, acceleration and coasting
- Leaves touchpads, Ctrl+wheel zoom and admin windows alone
- Tray icon shows when smoothing is on or off
- No internet connection, no data collection
- Free and open source (MIT)

**Search terms:** smooth scrolling · mouse wheel · smooth scroll · autoscroll · momentum scrolling · scroll acceleration · RollerMouse

**Copyright and trademark info:** © 2026 Eranno AS. Screzy is not affiliated with Contour Design; RollerMouse is a trademark of Contour Design, Inc.

**Additional license terms:** MIT License: https://opensource.org/licenses/MIT

## 5. Text for reviewers

**runFullTrust justification:**

> Screzy is a Win32 desktop utility that smooths mouse-wheel scrolling system-wide. It needs runFullTrust to install a low-level mouse hook (WH_MOUSE_LL) that intercepts vertical wheel events, and to send the smoothed scrolling with SendInput. It uses no keyboard hook, collects no data and makes no network connections.

**Notes for certification:**

> Screzy runs in the system tray. On first launch (and whenever the app is launched again) the Preferences window opens. To test: open any long web page or document and scroll with a mouse wheel. Scrolling glides smoothly instead of jumping. Flick the wheel quickly to see it coast, and click to stop it. The click that stops a coast is intentionally not passed to the window underneath. Use the "Try it" box in Preferences to test settings. The tray icon's right-click menu toggles smoothing, turns "Start with Windows" on and off (it shows the current state with a check mark, and opens Windows Startup settings if the user has switched it off there), and exits the app. Note: touchpads and precision/high-resolution wheels are intentionally passed through unchanged. Testing requires a mouse with a standard notched scroll wheel.
