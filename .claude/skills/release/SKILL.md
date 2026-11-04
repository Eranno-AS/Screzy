---
name: release
description: Prepare a Screzy release (version bump, changelog, docs sync, build, MSIX, tag) following RELEASING.md. Use when the user asks to release, ship, bump the version, or publish an update. It stops before Partner Center, which is manual.
---

# Release Screzy

`RELEASING.md` in the repository root is the procedure. Read it first and follow it. Don't duplicate its steps here.

Do these, in order, and report each result:

1. Ask for the new version number and the list of changes, unless the user already gave them. Use the next number: 1.0.1 for fixes, 1.1.0 for features. The Store needs the fourth part to be `0`.
2. Bump the version everywhere: `APP_VERSION` in `src/screzy.c`, `FILEVERSION`, `PRODUCTVERSION`, `FileVersion` and `ProductVersion` in `res/screzy.rc`. Grep for the old number afterwards to catch strays.
3. Add the `CHANGELOG.md` entry in the existing tone: plain, friendly English, sentence case.
4. Check that `README.md`, `store/README.md`, `PRIVACY.md`, `website/privacy/` and `website/index.html` still match the app, as `CLAUDE.md` invariant 5 requires. Fix what doesn't.
5. Build with `build.cmd` (or `build.sh`). It must be warning-free.
6. Build the package with `store/pack.ps1` using the identity and publisher in `RELEASING.md` and `-Version x.y.z.0`.
7. Show the user the diff and the package path. Commit with an imperative summary under 70 characters only once they agree, and push only when they ask.
8. Tag `vX.Y.Z` and push the tag only when the user says to. It publishes a GitHub release.

Stop there. Tell the user the Partner Center steps are theirs (RELEASING.md, step 7): upload the package, write *What's new*, submit.

For a graphics-only change, use the table in RELEASING.md to see whether a new version is needed at all. Never put unfinished graphics under `website/`, `store/` or `res/`: drafts live in `inbox/` (graphics in `inbox/graphics/`).
