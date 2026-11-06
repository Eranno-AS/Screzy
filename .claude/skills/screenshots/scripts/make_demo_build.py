"""Build a throwaway "demo" Screzy.exe for screenshots. Never committed, never shipped.

It is the current src/screzy.c with three patches, so the captures show what a Store user sees
without anything from this PC:
  1. Running apps... lists a fixed set of common apps instead of the windows open here.
  2. The tray menu is the Store version's (Start with Windows, no .ini items).
  3. Start with Windows shows as on.

Usage: python -I make_demo_build.py <repo> <out_dir>
Writes <out_dir>/Screzy.exe and <out_dir>/demo.json (control IDs the capture needs).
If a patch no longer matches after a redesign, this stops and says which one: update the
pattern here to match the new code, keeping the same intent.
"""
import json, os, re, subprocess, sys

repo, out = sys.argv[1], sys.argv[2]
os.makedirs(out, exist_ok=True)
src = open(os.path.join(repo, 'src', 'screzy.c'), encoding='utf-8').read()

DEMO_APPS = ['EXCEL.EXE', 'explorer.exe', 'msedge.exe', 'mspaint.exe', 'notepad.exe', 'POWERPNT.EXE', 'WINWORD.EXE']


def patch(s, old, new, what, start=0):
    i = s.find(old, start)
    if i < 0:
        sys.exit(f'make_demo_build: patch "{what}" no longer matches src/screzy.c. Update it in this script.')
    return s[:i] + new + s[i + len(old):], i


# 1. fixed running-apps list
names = ', '.join(f'"{n}"' for n in DEMO_APPS)
src, _ = patch(src, 'EnumWindows(enum_win, 0);',
               f'{{ static const char *demo[] = {{ {names} }}; int d; (void)enum_win;\n'
               f'      for (d = 0; d < {len(DEMO_APPS)}; d++) strcpy(pickNames[npick++], demo[d]); }}',
               'running-apps list')

# 2 + 3. Store tray menu with Start with Windows on (only inside the tray menu block)
_, at = patch(src, 'AppendMenuA(m, MF_STRING, ID_PREFS, "Preferences...");', 'AppendMenuA(m, MF_STRING, ID_PREFS, "Preferences...");', 'tray menu')
src, at = patch(src, 'if (is_packaged())', 'if (1)', 'Store tray menu', at)
src, _ = patch(src, 'su_state()', 'SU_ON', 'Start with Windows on', at)

ids = {}
for name in ('ID_PICK',):
    m = re.search(rf'#define\s+{name}\s+(\d+)', src)
    if not m:
        sys.exit(f'make_demo_build: {name} not found in src/screzy.c')
    ids[name] = int(m.group(1))
m = re.search(r'#define\s+WM_TRAY\s+\(WM_USER\s*\+\s*(\d+)\)', src)
if not m:
    sys.exit('make_demo_build: WM_TRAY not found in src/screzy.c')
ids['WM_TRAY'] = 0x400 + int(m.group(1))
m = re.search(r'MK\("BUTTON",\s*"(Running apps[^"]*)"', src)
ids['PICK_TEXT'] = m.group(1) if m else 'Running apps...'
ids['DEMO_APPS'] = len(DEMO_APPS)

c = os.path.join(out, 'screzy_demo.c')
open(c, 'w', encoding='utf-8').write(src)
libs = '-lwinmm -ldwmapi -lcomctl32 -lshlwapi -luser32 -lshell32 -ladvapi32 -lgdi32'.split()
res = os.path.join(out, 'res.o')
subprocess.run(['windres', '-I', os.path.join(repo, 'res'), os.path.join(repo, 'res', 'screzy.rc'),
                '-O', 'coff', '-o', res], check=True, cwd=repo)
subprocess.run(['gcc', '-O2', '-mwindows', '-static', '-o', os.path.join(out, 'Screzy.exe'), c, res] + libs, check=True)
json.dump(ids, open(os.path.join(out, 'demo.json'), 'w'), indent=1)
print('demo build ok', ids)
