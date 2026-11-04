# Publishing Screzy 1.0

This is a step-by-step guide to taking this folder to version 1.0. It covers a GitHub repository, a release download, screzy.com, and the Microsoft Store under **Eranno AS**.

Do it in **Antigravity** or **VS Code**. Both have Git built in and a terminal (**Ctrl + `**). Claude Code can do most of the typing: each step has a prompt you can paste.

Keep the order: the website must be live before you submit to the Store, because the Store checks the privacy URL.

---

## Checklist

- [x] 1. Open the folder and make the first Git commit
- [x] 2. Create the GitHub repository and push
- [x] 3. Build and test `Screzy.exe` on Windows
- [x] 4. Reserve the names and copy Product identity in Partner Center
- [x] 5. Build the MSIX and test it locally
- [x] 6. Take Store screenshots
- [x] 7. Put `website/` online at screzy.com
- [x] 8. Submit to the Store
- [ ] 9. Tag v1.0.0 and publish the GitHub release
- [x] 10. After approval: add the Store link everywhere

---

## 1. Open the folder and make the first commit

1. Unzip `Screzy-repo.zip` to a folder for code, e.g. `C:\Dev\Screzy`. Avoid OneDrive folders, which interfere with Git and builds.
2. In Antigravity or VS Code: **File → Open Folder… → `C:\Dev\Screzy`**.
3. Open the terminal and run:

```powershell
git init -b main
git add .
git commit -m "Screzy 1.0.0"
```

If Git asks who you are, run `git config --global user.name "Your Name"` and `git config --global user.email you@example.com` first.

`CLAUDE.md` in the root gives Claude Code the project context, so it knows the architecture, the invariants and which docs to keep in sync.

## 2. Create the GitHub repository

**Name:** `Screzy`. Create it under your own account or a new `eranno` organisation. For a company product, an organisation is neater: **github.com → + → New organization**, free plan.

**Easiest:** in the Source Control panel, click **Publish Branch** (VS Code) or **Publish to GitHub**, and choose **public**.

**Or with the GitHub CLI:**

```powershell
gh repo create Screzy --public --source . --push --description "Scroll easy. Flick to coast. Steer with mouse or rollerbar. Smooth scrolling for Windows."
```

Then replace the placeholders with the real address, e.g. `https://github.com/Eranno-AS/Screzy`:

> **Claude Code prompt:** *Replace every `GITHUB_URL` placeholder in `website/` with `https://github.com/Eranno-AS/Screzy` and update the example address in the comment at the top of `website/index.html`. Then commit with the message "Add GitHub links" and push.*

On GitHub, open the repository's **About** (⚙) and set:

- **Website:** `https://screzy.com`
- **Topics:** `windows`, `smooth-scrolling`, `mouse`, `win32`, `c`, `rollermouse`, `utility`

## 3. Build and test on Windows

Install a compiler once. The simplest option is **w64devkit**:

1. Download the latest `w64devkit-x64-*.exe` from https://github.com/skeeto/w64devkit/releases. It's a self-extracting archive. Extract it to `C:\w64devkit`.
2. Add `C:\w64devkit\bin` to your PATH: Start → *Edit environment variables for your account* → Path → New. Then restart Antigravity or VS Code.

Then build:

```powershell
.\build.cmd
.\build\Screzy.exe
```

Test it for real before publishing:

- [x] Windows' **Lines to scroll at a time** is set to **1** (Settings → Bluetooth & devices → Mouse).
- [x] Smooth scrolling works in a browser, Antigravity or VS Code, a PDF reader, Word, and Explorer.
- [x] Flick to coast. A left, middle or right click stops it, and the click does **not** open the link under the pointer.
- [x] Steering works in both styles, *Distance from start* and *Push*, with the mouse and the RollerMouse rollerbar.
- [x] *Wait (ready)* works, as do the Alt + middle click and Alt + G shortcuts.
- [x] Per-app rule: **Running apps…** → pick an app → *Disable smoothing* → **Add / Update** → **Apply**.
- [x] Tray icon: toggle on/off (it turns grey), **About**, **Website**, **Exit**.
- [x] Display scaling at 125% and 150%: no clipped labels in Preferences.
- [x] Old GlideScroll settings carry over. If you used a GlideScroll build, start Screzy once: your settings should come with it, and GlideScroll should close.

The GitHub Actions workflow also builds `Screzy.exe` on every push (**Actions** tab → latest run → *Artifacts*). That confirms the repository builds cleanly on a fresh machine.

## 4. Partner Center: reserve the names and copy Product identity

At https://partner.microsoft.com/dashboard, signed in with the **Eranno AS** account:

1. **Apps and games → + New product → MSIX or PWA app.**
2. Reserve **`Screzy – Easy, smooth scrolling`**. The character between the words is an *en dash* (–), not a hyphen. Copy it from here.
3. In the new product: **Product management → Manage app names → Reserve another name → `Screzy`**. Then nobody else can take the short name.
4. **Product management → Product identity.** Copy these three values into a note:
   - `Package/Identity/Name`, like `12345ErannoAS.Screzy…`
   - `Package/Identity/Publisher`, like `CN=XXXXXXXX-XXXX-…`
   - `Package/Properties/PublisherDisplayName`, which should be `Eranno AS`

If Partner Center won't accept the en dash, reserve `Screzy - Easy, smooth scrolling` with a plain hyphen. Then pass the same text to `pack.ps1` with `-DisplayName` in step 5, because the package name must match a reserved name exactly.

## 5. Build the MSIX

You need the **Windows SDK** for `makeappx.exe`. Get it from https://developer.microsoft.com/windows/downloads/windows-sdk/; only *Windows SDK Signing Tools for Desktop Apps* and *MSIX Packaging Tools* are needed.

**Local test first.** Turn on Settings → System → For developers → **Developer Mode**, then run:

```powershell
powershell -ExecutionPolicy Bypass -File .\store\pack.ps1 -IdentityName "ErannoAS.Screzy.Test" -Publisher "CN=ErannoTest" -RegisterLocally
```

Start **Screzy** from the Start menu. Check that it runs, that **Start with Windows…** opens Windows' Startup apps page with Screzy listed, and that settings survive a restart of the app. Remove the test install afterwards, and exit the plain `.exe` while testing so the two don't clash:

```powershell
Get-AppxPackage *Screzy* | Remove-AppxPackage
```

**The real package**, using your values from step 4:

```powershell
powershell -ExecutionPolicy Bypass -File .\store\pack.ps1 -IdentityName "<Package/Identity/Name>" -Publisher "<Package/Identity/Publisher>" -Version 1.0.0.0
```

This writes `store\out\Screzy_1.0.0.0_x64.msix`. The Store signs it; you don't need a certificate. The `store/out/` folder is ignored by Git.

*Optional:* to have GitHub build the MSIX too, add the repository variables `STORE_IDENTITY_NAME` and `STORE_PUBLISHER` (Settings → Secrets and variables → Actions → **Variables**).

## 6. Store screenshots

You need at least one, ideally 3–4, PNG at **1366×768 or larger**, taken on Windows (Win + Shift + S, or Snipping Tool):

1. The Preferences window over a long web page.
2. A coast running, with the indicator visible by the pointer, e.g. on a long article.
3. The tray menu open.
4. **Running apps…** with a per-app rule.

Use light mode and a tidy desktop, and remove anything personal from the browser.

## 7. Put the website online

`website/` is the complete static site for **screzy.com**:

- `index.html`, the product page with a live scroll demo
- `privacy/index.html`, which is the Store's privacy URL `https://screzy.com/privacy`
- `assets/`, holding the icon, the screenshot, the social-media image and the font, which is self-hosted with its licence included

Upload the **contents** of `website/` to the web root of screzy.com, through your host's file manager or FTP. Free alternatives that deploy from the GitHub repository:

- **Cloudflare Pages** or **Netlify**: connect the repository, set the build output directory to `website`, leave the build command empty, then add the custom domain screzy.com.
- **GitHub Pages**: Settings → Pages → deploy from a branch is limited to `/` or `/docs`, so use a small Actions workflow instead. Ask Claude Code: *"Add a GitHub Pages workflow that publishes the website/ folder, with screzy.com as the custom domain."*

Check `https://screzy.com/` and `https://screzy.com/privacy/` on both desktop and phone before continuing.

The **Get it from Microsoft Store** button points to the placeholder `STORE_URL` until step 10.

## 8. Submit to the Store

In Partner Center → your product → **Start submission**. All the text is in [`store/README.md`](store/README.md), sections 3–5:

| Section | What to do |
|---|---|
| Pricing and availability | **Free**, all markets, public |
| Properties | Category **Utilities & tools**. Privacy policy `https://screzy.com/privacy`. Website `https://screzy.com`. Support contact: an email or the GitHub issues page |
| Age ratings | IARC questionnaire: answer *no* to everything. You'll get 3+ / Everyone |
| Packages | Upload `store\out\Screzy_1.0.0.0_x64.msix` |
| Store listings → English | Name, short description, description, features, search terms and copyright from `store/README.md` §4. Screenshots from step 6. Images from `store/listing/` (logo-300, box art 1:1 1080, poster 2:3) |
| Submission options | **Restricted capabilities:** paste the `runFullTrust` justification. **Notes for certification:** paste the reviewer notes (both in §5) |

Then **Submit to the Store**. Certification usually takes 1–3 working days. If it's rejected, the report says why. The usual causes are the privacy URL, a screenshot or the runFullTrust text. Fix that and resubmit.

## 9. Tag v1.0.0 and publish the GitHub release

```powershell
git tag -a v1.0.0 -m "Screzy 1.0.0"
git push origin v1.0.0
```

The workflow builds `Screzy.exe` and attaches it to a GitHub release called **v1.0.0**. Then open **Releases**, edit the release, paste the 1.0.0 section of `CHANGELOG.md` as the notes, and publish. The website's **Download .exe** button points to `…/releases/latest/download/Screzy.exe`, so it works from now on.

The downloaded `.exe` isn't code-signed, so SmartScreen shows a warning on first run. The README explains how to get past it. The Store version has no warning, so point people there.

## 10. After Store approval

Partner Center shows the Store link, `https://apps.microsoft.com/detail/9XXXXXXXXXXX`, under **Product identity → URL**.

> **Claude Code prompt:** *The Microsoft Store link for Screzy is `https://apps.microsoft.com/detail/9XXXXXXXXXXX`. Replace the `STORE_URL` placeholder in `website/` with it, update the README's Microsoft Store line to link it, commit with the message "Add Store link" and push.*

Then upload `website/` again (or let Pages redeploy), and test the Store button.

---

## Later releases

1. Make the change, and test as in step 3.
2. > **Claude Code prompt:** *Prepare release 1.0.1: bump the version in `src/screzy.c` and `res/screzy.rc`, add a CHANGELOG entry for these changes: … and check that README, `store/README.md`, the privacy texts and `website/` still match the app.*
3. Rebuild, then run `pack.ps1 … -Version 1.0.1.0`. In Partner Center → **Update** (new submission) → upload the new package and fill in *What's new in this version*.
4. `git tag -a v1.0.1 -m "Screzy 1.0.1"` and `git push origin v1.0.1`.

The Store requires every new package to have a higher version than the last, with `.0` as the fourth part.

## Useful Claude Code prompts

- *"Read CLAUDE.md, then build with build.cmd and fix any warnings."*
- *"Users report X. Find where this is handled in src/screzy.c and propose a minimal fix. Don't touch the mouse-hook fast path."*
- *"Check that README, store/README.md, PRIVACY.md and website/ all describe the current settings and defaults."*
- *"Take the Settings table in README.md and make sure every slider in the Preferences window is listed with its real default."*
