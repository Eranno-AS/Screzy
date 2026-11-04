# inbox

A scratch folder for anything that is not finished: drafts, exports, AI generations,
logo variants, screenshots you might use. **Git ignores everything here except this file
and `.gitignore`**, so nothing in this folder is ever committed, pushed or published.

- `graphics/` holds unfinished graphics.

Everything under `website/` goes live on screzy.com when it is pushed, so only finished
files that something references belong there. The same goes for `store/` and `res/`.

To publish a graphic, copy the finished file to its real place (see the table in
`RELEASING.md`), or run `tools/make_assets.py` on a master logo to generate the full set.
