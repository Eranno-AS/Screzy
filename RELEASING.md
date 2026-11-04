# Releasing Screzy

How to ship an update after 1.0. `PUBLISHING.md` covers the first release (accounts, names, the website, the first Store submission). This file covers every release after it.

Claude Code can do the mechanical steps: run `/release` and say what changed. The Partner Center steps are always yours.

## What needs what

| You changed | Where | Then you need |
|---|---|---|
| Code or behaviour | `src/` | A new version: the full release below |
| App icon (`.ico`) or coast indicators | `res/icons/`, `res/indicator/` | A new version: they're embedded in the exe |
| Store tiles | `store/Assets/` | A new version: they're inside the package |
| Store listing text, screenshots, box art, poster, logo | `store/listing/`, Partner Center | A listing update only: no new package, but it is still a submission and gets certified |
| Website, README, `docs/preferences.png` | `website/`, `README.md`, `docs/` | A push: Cloudflare redeploys the site by itself |

Everything under `website/` is public the moment it is pushed. Keep drafts and working files in `inbox/` (ignored by Git) and copy only finished, referenced files into the tree.

## Full release (new version)

Use the next number: 1.0.1 for fixes, 1.1.0 for new features. The Store's fourth part is always `0`.

1. **Make the change and test it** as in `PUBLISHING.md` step 3. Build with `.\build.cmd`: it must stay warning-free.
2. **Bump the version** (all of these, to the same number):
   - `APP_VERSION` in `src/screzy.c`
   - `FILEVERSION` and `PRODUCTVERSION` (`1,0,1,0`) and the `FileVersion` and `ProductVersion` strings (`"1.0.1"`) in `res/screzy.rc`
3. **Add a `CHANGELOG.md` entry** at the top: plain, friendly English, one bullet per change.
4. **Keep the docs in sync** (see `CLAUDE.md`, invariant 5): `README.md`, `store/README.md`, the privacy texts and `website/index.html`, wherever the change touches them.
5. **Commit and push.** The "build" workflow must go green.
6. **Build the package:**
   ```powershell
   powershell -ExecutionPolicy Bypass -File .\store\pack.ps1 -IdentityName "ErannoAS.ScrezyEasysmoothscrolling" -Publisher "CN=326321BE-2678-4B81-BA66-C39EE9951D35" -Version 1.0.1.0
   ```
   This writes `store\out\Screzy_1.0.1.0_x64.msix`. The two repository variables `STORE_IDENTITY_NAME` and `STORE_PUBLISHER` make GitHub build the same package: download it from the workflow run's **Screzy-msix** artifact if you prefer.
7. **Partner Center** → Screzy → **Update**. Upload the new package, write *What's new in this version* (the changelog bullets, shortened), check that the listing text still fits, and **Submit**. Certification takes 1–3 working days.
8. **Tag it** so GitHub has the standalone `.exe`:
   ```powershell
   git tag -a v1.0.1 -m "Screzy 1.0.1"
   git push origin v1.0.1
   ```
   The workflow attaches `Screzy.exe` to a release. Paste the new `CHANGELOG.md` section into the release notes.

Store users get the update automatically once it is approved. People with the plain `.exe` have to download the new one.

## New graphics

1. Put drafts and exports in `inbox/graphics/`. Nothing in it is ever committed.
2. Pick the finished icon: a square PNG, 1024 px or more, with transparent corners.
3. Generate everything from it, previewing first:
   ```powershell
   python tools\make_assets.py inbox\graphics\master.png --out inbox\graphics\preview
   python tools\make_assets.py inbox\graphics\master.png
   ```
   The second command overwrites `res/icons/*.ico`, `store/Assets/*.png` and `store/listing/*.png`. Use `--only icons`, `--only tiles` or `--only listing` for one group.
4. Make by hand what the script can't: `website/assets/screzy.svg`, `website/assets/og-image.png`, the coast indicators (`tools/make_indicator.py`), and screenshots.
5. Release as the table above says: icons and tiles need a full release, the rest don't.

For a new screenshot set, follow step 6 of `PUBLISHING.md` (1366×768 or larger) and replace the images in the Store listing.

## Troubleshooting

- **The build workflow fails with exit code 126:** `build.sh` lost its executable bit. Run `git update-index --chmod=+x build.sh` and commit.
- **The Store rejects the package identity:** the values must match **Product management → Product identity** exactly.
- **The version is rejected:** every package needs a higher version than the last one, with `.0` as the fourth part.
- **Rejected in certification:** the report says why. The usual causes are the privacy URL, a screenshot or the `runFullTrust` text (`store/README.md` §5).
