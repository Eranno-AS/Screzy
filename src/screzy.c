/*
 * Screzy - easy, smooth mouse-wheel scrolling for Windows.
 * Scroll easy. Flick to coast. Steer with mouse or rollerbar.
 * Copyright (c) 2026 Eranno AS. Released under the MIT License (see LICENSE).
 *
 * Intercepts vertical wheel ticks with a low-level mouse hook, swallows them,
 * and re-emits the same distance as many small, eased wheel events so every
 * app scrolls fluidly. Rapid wheel spinning accelerates. Per-app speed and
 * per-app disable are read from %APPDATA%\Screzy\screzy.ini.
 * A fast flick keeps coasting until it runs down or is stopped by a click; while it
 * coasts, wheel clicks (and optionally up/down pointer movement) steer its speed.
 *
 * Build: run build.sh (Linux/macOS) or build.cmd (Windows). See README.md.
 */
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0601
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <shlwapi.h>
#include <mmsystem.h>
#include <dwmapi.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define APP_VERSION   "1.0.1"
#define APP_URL       "https://screzy.com"
#define WM_TRAY       (WM_USER + 1)
#define WM_SHOWPREFS  (WM_USER + 2)   /* sent by a second launch: open Preferences */
#define WM_COASTSTART (WM_USER + 3)   /* animation thread -> UI: a coast began, show the indicator */
#define ID_ABOUT      1007
#define ID_WEBSITE    1008
#define ID_STARTUPSET 1009
#define ID_TOGGLE     1001
#define ID_SETTINGS   1002
#define ID_RELOAD     1003
#define ID_AUTOSTART  1004
#define ID_EXIT       1005
#define MAX_APPS      64
#define FRAME_MS      8

typedef struct { char name[64]; double speed; int disabled; } AppRule;

static struct {
    double speed;        /* global distance multiplier (1.0 = normal) */
    double duration_ms;  /* how long one tick glides */
    double accel;        /* extra multiplier per rapid consecutive tick */
    double accel_max;    /* cap on acceleration multiplier */
    double accel_start;  /* wheel speed (clicks/s) where acceleration begins */
    double coast_time;   /* seconds a flick keeps gliding; 0 = off, >= 999 = until stopped */
    double flick_rate;   /* wheel speed (clicks/s) that counts as a flick */
    int    flick_clicks; /* fast clicks in a row needed before a flick coasts */
    double coast_start;  /* coast starting speed in lines/s; 0 = the flick's own speed */
    int    coast_wait;   /* a flick only arms a coast at standstill ("ready"); wheel/rollerbar starts it */
    int    ready_key;    /* Ready shortcut: 0 off, 1 Alt + middle click, 2 Alt + G */
    double coast_max;    /* max coast speed in lines/s; 0 = no limit */
    int    steer;        /* steer coasts with the pointer: 0 off, 1 push (pointer held), 2 distance from start */
    double steer_sens;   /* steering: coast-speed change in lines/s per pixel moved */
    int    opp_stops;    /* opposite wheel click stops a coast (else: slows it down) */
    int    until_click;  /* a coast the user has steered never ends by itself, only by a click */
    int    indicator;    /* show the coast indicator next to the pointer */
    AppRule apps[MAX_APPS];
    int    napps;
} cfg;

static CRITICAL_SECTION lock;
static double pending = 0;          /* wheel units still to emit (signed) */
static double carry = 0;            /* sub-unit remainder between frames */
static double vel = 0;              /* current glide velocity, wheel units/s */
static HANDLE wake;                 /* signalled when a new wheel tick arrives */
/* Flick-to-coast: after a fast flick, keep gliding at the flick's speed and let it
   slowly run down. A left/middle/right click or an opposite wheel click stops it. */
static int    coastArmed = 0;      /* a flick was detected */
static int    coasting = 0;        /* the wheel has stopped and we're coasting */
static double coastVel = 0;        /* coasting speed, wheel units/s (signed) */
static double coastElapsed = 0;
static double lastRate = 0;        /* wheel speed at the last click, clicks/s */
static LONGLONG lastClickQ = 0;    /* QPC time of the last wheel click */

static int    coastHeld = 0;       /* user steered/nudged: hold the speed instead of decaying */
static int    coastWaiting = 0;    /* armed at standstill ("ready"), waiting for wheel/rollerbar */
static int    flickTail = 0;       /* ready came from a flick: ignore its trailing clicks for a moment */
static double steerTarget = 0;     /* distance steering: speed the coast eases toward */
static int    steerTargetOn = 0;
static int    steerEngaged = 0;    /* pointer movement passed the dead zone this coast */
static double steerAccum = 0;      /* vertical movement collected toward the dead zone */
static LONGLONG lastSteerQ = 0;
static double slowTime = 0;        /* how long a held coast has been near standstill */
static int    coastTail = 0;       /* a run-down coast is easing out to zero */
static double tailV0 = 0, tailT = 0, tailLen = 0;
static int    tailDir = 0;
/* Momentum: a fast run that isn't a flick yet carries on a little after the wheel stops,
   more the faster and longer it was. Not a coast: no indicator, no steering, clicks pass. */
static double softStrength = 0;    /* 0..1, how close the current run is to a flick */
static int    momOn = 0, momTail = 0, momDir = 0;
static double momVel = 0, momTau = 0, momT = 0, momLen = 0, momV0 = 0;
static volatile UINT wlinesG = 3;  /* Windows' lines per wheel click (updated by the animation) */
static volatile LONG steerAnchorY = 0; /* pointer height when the coast started; held there while steering */
static volatile LONG steerAnchorX = 0; /* ... and its x, where the indicator sits in distance mode */
static int S(int v);

static void stop_all(void) {       /* caller holds `lock` */
    pending = carry = vel = 0;
    coastArmed = coasting = 0; coastVel = coastElapsed = 0;
    coastHeld = steerEngaged = coastWaiting = flickTail = 0; steerAccum = slowTime = 0;
    steerTarget = 0; steerTargetOn = 0;
    coastTail = 0;
    momOn = momTail = 0; momVel = softStrength = 0;
}
static int    lastDir = 0;
static volatile int enabled = 1;
static volatile int running = 1;
static HHOOK hook;
static HWND  hwndMain;
static void update_hotkey(void);
static NOTIFYICONDATAA nid;
static char iniPath[MAX_PATH];
static HICON icoOn, icoOff;

static HICON load_icon(int id, int big) {
    int cx = GetSystemMetrics(big ? SM_CXICON : SM_CXSMICON);
    return (HICON)LoadImageA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(id), IMAGE_ICON, cx, cx, 0);
}

static const char *RUN_KEY = "Software\\Microsoft\\Windows\\CurrentVersion\\Run";

/* ---------------------------------------------------------------- config */
static int write_default_ini(void) {   /* returns 1 on first run */
    if (GetFileAttributesA(iniPath) != INVALID_FILE_ATTRIBUTES) return 0;
    WritePrivateProfileStringA("general", "speed", "1.0", iniPath);
    WritePrivateProfileStringA("general", "duration_ms", "260", iniPath);
    WritePrivateProfileStringA("general", "acceleration", "0.5", iniPath);
    WritePrivateProfileStringA("general", "acceleration_max", "4.0", iniPath);
    WritePrivateProfileStringA("general", "acceleration_starts_at", "6", iniPath);
    WritePrivateProfileStringA("general", "coast_seconds", "3", iniPath);
    WritePrivateProfileStringA("general", "flick_clicks_per_s", "15", iniPath);
    WritePrivateProfileStringA("general", "flick_clicks_needed", "4", iniPath);
    WritePrivateProfileStringA("general", "coast_start_lines_per_s", "flick", iniPath);
    WritePrivateProfileStringA("general", "coast_max_lines_per_s", "120", iniPath);
    WritePrivateProfileStringA("general", "steer_with_pointer", "0", iniPath);
    WritePrivateProfileStringA("general", "steer_sensitivity", "1.0", iniPath);
    WritePrivateProfileStringA("general", "opposite_click_stops_coast", "0", iniPath);
    WritePrivateProfileStringA("general", "steered_coast_until_click", "1", iniPath);
    WritePrivateProfileStringA("general", "show_coast_indicator", "1", iniPath);
    WritePrivateProfileStringA("general", "ready_shortcut", "off", iniPath);
    /* Per-app rules: exe name = speed multiplier, or "off" to disable. */
    /* No per-app rules by default; users add them in Preferences. */
    return 1;
}

static double get_double(const char *sec, const char *key, double def) {
    char buf[64], d[64];
    snprintf(d, sizeof d, "%g", def);
    GetPrivateProfileStringA(sec, key, d, buf, sizeof buf, iniPath);
    return atof(buf);
}

/* The ini's last-write time when we last read or wrote it, so an edit made in a text editor is noticed. */
static FILETIME iniStamp;
static void note_ini_stamp(void) {
    WIN32_FILE_ATTRIBUTE_DATA d;
    if (GetFileAttributesExA(iniPath, GetFileExInfoStandard, &d)) iniStamp = d.ftLastWriteTime;
}

static void load_config(void) {
    char *sec, *p;
    static char buf[8192];
    EnterCriticalSection(&lock);
    cfg.speed        = get_double("general", "speed", 1.0);
    cfg.duration_ms  = get_double("general", "duration_ms", 260);
    cfg.accel        = get_double("general", "acceleration", 0.5);
    cfg.accel_max    = get_double("general", "acceleration_max", 4.0);
    cfg.accel_start  = get_double("general", "acceleration_starts_at", 6);
    if (cfg.accel_start < 1) cfg.accel_start = 1;
    {
        char cb[32];
        GetPrivateProfileStringA("general", "coast_seconds", "3", cb, sizeof cb, iniPath);
        cfg.coast_time = (_stricmp(cb, "forever") == 0) ? 999 : atof(cb);
        if (cfg.coast_time < 0) cfg.coast_time = 0;
    }
    cfg.flick_rate   = get_double("general", "flick_clicks_per_s", 15);
    cfg.flick_clicks = (int)get_double("general", "flick_clicks_needed", 4);
    {
        char cb[32];
        GetPrivateProfileStringA("general", "coast_start_lines_per_s", "flick", cb, sizeof cb, iniPath);
        cfg.coast_wait  = _stricmp(cb, "wait") == 0;
        cfg.coast_start = (_stricmp(cb, "flick") == 0 || cfg.coast_wait) ? 0 : atof(cb);
        if (cfg.coast_start < 0) cfg.coast_start = 0;
    }
    if (cfg.flick_clicks < 2) cfg.flick_clicks = 2;
    if (cfg.flick_rate < 4) cfg.flick_rate = 4;
    {
        char cb[32];
        GetPrivateProfileStringA("general", "coast_max_lines_per_s", "120", cb, sizeof cb, iniPath);
        cfg.coast_max = (_stricmp(cb, "none") == 0) ? 0 : atof(cb);
        if (cfg.coast_max < 0) cfg.coast_max = 0;
    }
    cfg.steer        = (int)get_double("general", "steer_with_pointer", 0);   /* 0 off, 1 push, 2 distance */
    if (cfg.steer < 0 || cfg.steer > 2) cfg.steer = 0;
    cfg.steer_sens   = get_double("general", "steer_sensitivity", 1.0);
    if (cfg.steer_sens <= 0) cfg.steer_sens = 1.0;
    cfg.opp_stops    = (int)get_double("general", "opposite_click_stops_coast", 0) != 0;
    cfg.until_click  = (int)get_double("general", "steered_coast_until_click", 1) != 0;
    cfg.indicator    = (int)get_double("general", "show_coast_indicator", 1) != 0;
    {
        char rk[32];
        GetPrivateProfileStringA("general", "ready_shortcut", "off", rk, sizeof rk, iniPath);
        cfg.ready_key = _stricmp(rk, "alt+middle") == 0 ? 1 : _stricmp(rk, "alt+g") == 0 ? 2 : 0;
    }
    if (cfg.speed <= 0) cfg.speed = 1;
    if (cfg.duration_ms < 20) cfg.duration_ms = 20;
    cfg.napps = 0;
    GetPrivateProfileSectionA("apps", buf, sizeof buf, iniPath);
    for (sec = buf; *sec && cfg.napps < MAX_APPS; sec += strlen(sec) + 1) {
        if (sec[0] == ';') continue;
        p = strchr(sec, '=');
        if (!p) continue;
        AppRule *r = &cfg.apps[cfg.napps];
        size_t n = (size_t)(p - sec);
        if (n >= sizeof r->name) continue;
        memcpy(r->name, sec, n); r->name[n] = 0;
        StrTrimA(r->name, " \t");
        p++;
        while (*p == ' ') p++;
        r->disabled = (_stricmp(p, "off") == 0 || _stricmp(p, "disabled") == 0);
        r->speed = r->disabled ? 0 : atof(p);
        if (!r->disabled && r->speed <= 0) continue;
        cfg.napps++;
    }
    LeaveCriticalSection(&lock);
    note_ini_stamp();
}

/* Find the rule for the process under the cursor. Returns multiplier, or 0 = off. */
static int self_elevated = -1;

static int token_elevated(HANDLE proc, int *ok) {
    HANDLE t; TOKEN_ELEVATION e; DWORD n; int r = 0;
    *ok = 0;
    if (!OpenProcessToken(proc, TOKEN_QUERY, &t)) return 0;
    if (GetTokenInformation(t, TokenElevation, &e, sizeof e, &n)) { r = e.TokenIsElevated != 0; *ok = 1; }
    CloseHandle(t);
    return r;
}

static double app_speed_at(POINT pt) {
    HWND w = WindowFromPoint(pt);
    DWORD pid = 0;
    char path[MAX_PATH] = "", *base;
    double mult = 1.0;
    int i;
    if (w) GetWindowThreadProcessId(w, &pid);
    if (pid) {
        HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
        int ok, elev;
        DWORD sz = sizeof path;
        if (!h) return 0; /* protected process: can't inject into it, leave wheel alone */
        elev = token_elevated(h, &ok);
        QueryFullProcessImageNameA(h, 0, path, &sz);
        CloseHandle(h);
        /* Windows blocks input injected from a normal app into an admin app (UIPI),
           so smoothing there would just kill scrolling. Pass the wheel through. */
        if (!self_elevated && (!ok || elev)) return 0;
    }
    base = PathFindFileNameA(path);
    EnterCriticalSection(&lock);   /* Preferences may be replacing the rule list right now */
    for (i = 0; i < cfg.napps; i++) {
        if (_stricmp(cfg.apps[i].name, base) == 0) {
            mult = cfg.apps[i].disabled ? 0 : cfg.apps[i].speed;
            break;
        }
    }
    LeaveCriticalSection(&lock);
    return mult;
}

/* Enter "ready": a coast at standstill at the pointer, nothing scrolled. The wheel or
   rollerbar then starts it; a click ends it. Caller must NOT hold `lock`. */
static void enter_ready(POINT pt) {
    EnterCriticalSection(&lock);
    stop_all();
    coastArmed = coasting = 1;
    coastVel = 0; coastHeld = 1; coastWaiting = 1; coastElapsed = 0;
    steerAnchorY = pt.y; steerAnchorX = pt.x;
    lastDir = 0;
    LeaveCriticalSection(&lock);
    PostMessageA(hwndMain, WM_COASTSTART, 0, 0);
    SetEvent(wake);
}

/* After we swallow Alt + middle click, a lone Alt release would open the app's menu bar.
   Tapping an unassigned key in between (vk 0xE8) tells Windows Alt was "used". */
static void mask_alt_menu(void) {
    INPUT in[2];
    ZeroMemory(in, sizeof in);
    in[0].type = in[1].type = INPUT_KEYBOARD;
    in[0].ki.wVk = in[1].ki.wVk = 0xE8;
    in[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, in, sizeof(INPUT));
}

/* ------------------------------------------------------------ mouse hook */
static LRESULT CALLBACK mouse_proc(int code, WPARAM wp, LPARAM lp) {
    /* Fast path: ordinary pointer movement (by far the most frequent event, up to 1000/s)
       leaves immediately unless a coast is being steered. */
    if (wp == WM_MOUSEMOVE && !(coasting && cfg.steer))
        return CallNextHookEx(NULL, code, wp, lp);
    /* Steering: while coasting, vertical pointer movement (mouse or rollerbar) changes the
       coast's speed and direction instead of moving the pointer. Sideways moves still apply. */
    /* Distance steering (like middle-click autoscroll): the pointer moves freely, and the
       coast's speed follows how far it is above or below where the coast started. A small
       dead zone around the start means "standing still". */
    if (code == HC_ACTION && wp == WM_MOUSEMOVE && coasting && cfg.steer == 2 && enabled) {
        MSLLHOOKSTRUCT *m = (MSLLHOOKSTRUCT *)lp;
        if (!(m->flags & LLMHF_INJECTED)) {
            double d = (double)(m->pt.y - steerAnchorY);          /* > 0: below the start */
            double dead = S(8), x = fabs(d) - dead;
            EnterCriticalSection(&lock);
            if (coasting) {
                if (x > 0) {
                    /* The whole distance range maps onto 0..Max coast speed: very slow just
                       outside the dead zone, reaching the maximum ~280 px out (96-dpi pixels,
                       scaled by Steering sensitivity). */
                    double px = x * 96.0 / (double)S(96);
                    double full = 280.0 / cfg.steer_sens;
                    double frac = px / full; if (frac > 1) frac = 1;
                    double maxL = cfg.coast_max > 0 ? cfg.coast_max : 300.0;
                    double lines = maxL * pow(frac, 1.6);
                    steerTarget = (d > 0 ? -1 : 1) * lines * WHEEL_DELTA / (double)wlinesG;
                    steerTargetOn = 1;
                    steerEngaged = 1; coastHeld = 1; coastWaiting = 0; flickTail = 0;
                } else if (steerEngaged) {
                    steerTarget = 0; steerTargetOn = 1;            /* back at the start: stand still */
                }
            }
            LeaveCriticalSection(&lock);
            SetEvent(wake);
        }
        return CallNextHookEx(NULL, code, wp, lp);                 /* the pointer moves normally */
    }
    if (code == HC_ACTION && wp == WM_MOUSEMOVE && coasting && cfg.steer == 1 && enabled) {
        MSLLHOOKSTRUCT *m = (MSLLHOOKSTRUCT *)lp;
        if (!(m->flags & LLMHF_INJECTED)) {
            POINT cur; LARGE_INTEGER q, fq;
            (void)cur;
            /* Measure from where the pointer was when the coast began, not from
               GetCursorPos (which isn't reliably the pre-move position everywhere). */
            int dy = m->pt.y - steerAnchorY;
            if (dy != 0) {
                QueryPerformanceCounter(&q); QueryPerformanceFrequency(&fq);
                EnterCriticalSection(&lock);
                if (coasting) {
                    if ((double)(q.QuadPart - lastSteerQ) / (double)fq.QuadPart > 0.3) steerAccum = 0;
                    lastSteerQ = q.QuadPart;
                    steerAccum += dy;
                    /* dead zone: ignore brushes until ~6 px of deliberate movement */
                    if (steerEngaged || fabs(steerAccum) >= 6) {
                        double dv = (steerEngaged ? dy : steerAccum) * cfg.steer_sens * WHEEL_DELTA / (double)wlinesG;
                        steerEngaged = 1;
                        coastWaiting = 0;
                        coastVel -= dv;        /* pointer down (dy > 0) = scroll down = negative wheel units */
                        coastHeld = 1;
                    }
                }
                LeaveCriticalSection(&lock);
                SetCursorPos(m->pt.x, steerAnchorY);   /* keep the sideways part, hold the height */
                SetEvent(wake);
                return 1;
            }
        }
    }
    /* A click stops a coast, and only stops it: the click (and its release) is swallowed,
       so it doesn't also press a link or button under the pointer - like touching a
       scrolling phone screen. The release matters too: right-click menus open on release. */
    static WPARAM swallowUp = 0;
    if (code == HC_ACTION && swallowUp && wp == swallowUp) {
        swallowUp = 0;
        return 1;
    }
    if (code == HC_ACTION && coastArmed &&
        (wp == WM_LBUTTONDOWN || wp == WM_RBUTTONDOWN || wp == WM_MBUTTONDOWN) &&
        !(((MSLLHOOKSTRUCT *)lp)->flags & LLMHF_INJECTED)) {
        EnterCriticalSection(&lock); stop_all(); LeaveCriticalSection(&lock);
        swallowUp = wp == WM_LBUTTONDOWN ? WM_LBUTTONUP : wp == WM_RBUTTONDOWN ? WM_RBUTTONUP : WM_MBUTTONUP;
        return 1;
    }
    /* A click during momentum stops the page, but (unlike a coast) still goes through. */
    if (code == HC_ACTION && momOn &&
        (wp == WM_LBUTTONDOWN || wp == WM_RBUTTONDOWN || wp == WM_MBUTTONDOWN) &&
        !(((MSLLHOOKSTRUCT *)lp)->flags & LLMHF_INJECTED)) {
        EnterCriticalSection(&lock); if (momOn && !coastArmed) stop_all(); LeaveCriticalSection(&lock);
    }
    /* Ready shortcut: Alt + middle click. Only the Alt state is checked, at this moment. */
    if (code == HC_ACTION && wp == WM_MBUTTONDOWN && cfg.ready_key == 1 && enabled &&
        !(((MSLLHOOKSTRUCT *)lp)->flags & LLMHF_INJECTED) && (GetAsyncKeyState(VK_MENU) & 0x8000)) {
        POINT pt = ((MSLLHOOKSTRUCT *)lp)->pt;
        if (app_speed_at(pt) > 0) {          /* not in apps where smoothing is off */
            enter_ready(pt);
            mask_alt_menu();
            swallowUp = WM_MBUTTONUP;
            return 1;
        }
    }
    if (code == HC_ACTION && wp == WM_MOUSEWHEEL && enabled) {
        MSLLHOOKSTRUCT *m = (MSLLHOOKSTRUCT *)lp;
        if (!(m->flags & LLMHF_INJECTED)) {
            /* Ctrl = zoom, Alt/Win = app shortcuts: leave untouched. */
            if (!(GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
                !(GetAsyncKeyState(VK_MENU) & 0x8000) &&
                !(GetAsyncKeyState(VK_LWIN) & 0x8000)) {
                int delta = (short)HIWORD(m->mouseData);
                /* Only smooth real wheel notches; touchpads and hi-res wheels already scroll smoothly. */
                double mult = (delta != 0 && delta % WHEEL_DELTA == 0) ? app_speed_at(m->pt) : 0;
                if (mult > 0) {
                    int dir = delta > 0 ? 1 : -1;
                    static LONGLONG hookFreq, lastQ;
                    static double rate;            /* smoothed wheel speed, clicks per second */
                    LARGE_INTEGER q;
                    if (!hookFreq) { QueryPerformanceFrequency(&q); hookFreq = q.QuadPart; }
                    QueryPerformanceCounter(&q);
                    double gap = (double)(q.QuadPart - lastQ) / (double)hookFreq;
                    int clicks = abs(delta) / WHEEL_DELTA;   /* fast spins can arrive as 240, 360... */
                    EnterCriticalSection(&lock);
                    if (coasting && flickTail && dir == lastDir &&
                        (double)(q.QuadPart - lastClickQ) / (double)hookFreq < 0.35) {
                        /* Still the tail end of the flick that armed "ready": a spinning wheel
                           lets a few late clicks through. Ignore them until the wheel pauses. */
                        lastClickQ = lastQ = q.QuadPart;
                        LeaveCriticalSection(&lock);
                        return 1;
                    }
                    if (coasting) {
                        flickTail = 0;
                        /* Wheel nudges while coasting: same direction speeds it up a step,
                           the opposite direction slows it down a step (or stops it). */
                        double minV = 360.0 / (double)wlinesG;
                        double stillV = 60.0 / (double)wlinesG;
                        int cdir = coastVel > 0 ? 1 : -1;
                        double sp = fabs(coastVel);
                        if (sp < stillV) {
                            /* standing still (e.g. "ready"): start in the click's direction */
                            coastVel = dir * 2 * minV;
                            coastHeld = 1; coastWaiting = 0;
                        } else if (dir == cdir) {
                            double up = sp * 1.3;
                            if (up < sp + 2 * minV) up = sp + 2 * minV;
                            coastVel = cdir * up;
                            coastHeld = 1; coastWaiting = 0;
                        } else if (cfg.opp_stops || sp * 0.6 < 2 * minV) {
                            stop_all();
                            lastDir = 0; rate = 0;
                        } else {
                            coastVel = cdir * sp * 0.6;
                            coastHeld = 1;
                        }
                        lastQ = q.QuadPart;
                        LeaveCriticalSection(&lock);
                        SetEvent(wake);
                        return 1;
                    }
                    if (dir != lastDir) {
                        int wasCoasting = coastArmed;
                        stop_all();
                        if (wasCoasting) {   /* a click against a coast just stops it */
                            lastDir = 0; lastQ = q.QuadPart; rate = 0;
                            LeaveCriticalSection(&lock);
                            return 1;
                        }
                    }
                    /* Acceleration from how fast the wheel is turning. The rate is smoothed over
                       the last few clicks, so it builds up and falls off gradually rather than
                       resetting on one slightly slow click. A pause of 0.6 s starts over. */
                    if (dir != lastDir || gap > 0.6) rate = 0;
                    else {
                        double inst = clicks / (gap < 0.008 ? 0.008 : gap);
                        rate = rate == 0 ? inst : 0.5 * rate + 0.5 * inst;
                    }
                    /* Each 4 clicks/s above the threshold adds `accel` to the multiplier. */
                    double a = 1.0 + cfg.accel * (rate - cfg.accel_start) / 4.0;
                    if (a < 1.0) a = 1.0;
                    if (a > cfg.accel_max) a = cfg.accel_max;
                    pending += delta * cfg.speed * mult * a;
                    momOn = momTail = 0; momVel = 0;   /* the wheel takes over again; the glide carries on */
                    int waitArmed = 0;
                    /* Flick detection: enough fast clicks in a row -> arm coasting at the current
                       scroll speed. The first click of a run has no speed yet, so it starts at 1. */
                    static int fastRun;
                    if (rate == 0) fastRun = 1;
                    else if (rate >= cfg.flick_rate) fastRun += clicks;
                    else fastRun = 1;
                    if (cfg.coast_time > 0 && rate >= cfg.flick_rate && fastRun >= cfg.flick_clicks) {
                        double v = dir * rate * WHEEL_DELTA * cfg.speed * mult * a;
                        if (!coasting || fabs(v) > fabs(coastVel)) coastVel = v;
                        coastArmed = 1;
                    } else if (!coasting) coastArmed = 0;
                    /* Momentum strength: grows with speed (from where acceleration starts up
                       to the flick speed) and with clicks in a row (up to the clicks needed). */
                    {
                        static int softRun;
                        double r0 = cfg.accel_start;
                        if (r0 > cfg.flick_rate * 0.75) r0 = cfg.flick_rate * 0.5;
                        if (rate == 0 || rate < r0) softRun = 1; else softRun += clicks;
                        double sr = (rate - r0) / (cfg.flick_rate - r0);
                        double sc = (softRun - 1.0) / (cfg.flick_clicks - 1.0);
                        if (sr < 0) sr = 0;
                        if (sr > 1) sr = 1;
                        if (sc < 0) sc = 0;
                        if (sc > 1) sc = 1;
                        softStrength = (coastArmed || cfg.coast_time <= 0) ? 0 : sr * sc;
                    }
                    if (cfg.coast_wait && coastArmed && !coasting) {
                        /* "Wait (ready)": a flick only arms the coast. Drop the flick's distance
                           and let the page ease to a stop from its current speed. */
                        double w = 4.74 / (cfg.duration_ms / 1000.0);
                        pending = vel / w * 0.5;
                        waitArmed = 1;
                    }
                    (void)waitArmed;
                    lastRate = rate;
                    lastClickQ = q.QuadPart;
                    lastDir = dir;
                    lastQ = q.QuadPart;
                    LeaveCriticalSection(&lock);
                    SetEvent(wake);
                    return 1; /* swallow the original tick */
                }
            }
        }
    }
    return CallNextHookEx(hook, code, wp, lp);
}

/* --------------------------------------------------------- animation loop */
/* ---- frame pacing: a precise timer locked to the refresh rate of the monitor
   under the cursor, firing mid-frame so each display frame gets exactly one step. */
static HANDLE frameTimer;
static LONGLONG qpcFreq;

static void wait_until(LONGLONG target) {
    LARGE_INTEGER now, due;
    QueryPerformanceCounter(&now);
    LONGLONG ticks = target - now.QuadPart;
    if (ticks <= 0) return;
    due.QuadPart = -(ticks * 10000000LL / qpcFreq);        /* relative, 100 ns units */
    if (due.QuadPart == 0) return;
    if (frameTimer && SetWaitableTimer(frameTimer, &due, 0, NULL, NULL, FALSE))
        WaitForSingleObject(frameTimer, 100);
    else Sleep((DWORD)(ticks * 1000 / qpcFreq));
}

/* Refresh period of the monitor under the cursor (QPC ticks) + a vblank time to align to. */
static void frame_clock(LONGLONG *period, LONGLONG *anchor) {
    POINT pt; MONITORINFOEXA mi; DEVMODEA dm; DWM_TIMING_INFO ti;
    LARGE_INTEGER now;
    double hz = 60;
    QueryPerformanceCounter(&now);
    GetCursorPos(&pt);
    ZeroMemory(&mi, sizeof mi); mi.cbSize = sizeof mi;
    ZeroMemory(&dm, sizeof dm); dm.dmSize = sizeof dm;
    if (GetMonitorInfoA(MonitorFromPoint(pt, MONITOR_DEFAULTTONEAREST), (MONITORINFO *)&mi) &&
        EnumDisplaySettingsA(mi.szDevice, ENUM_CURRENT_SETTINGS, &dm) && dm.dmDisplayFrequency > 1)
        hz = dm.dmDisplayFrequency;
    *period = (LONGLONG)(qpcFreq / hz);
    *anchor = now.QuadPart;
    ZeroMemory(&ti, sizeof ti); ti.cbSize = sizeof ti;
    if (SUCCEEDED(DwmGetCompositionTimingInfo(NULL, &ti)) && ti.qpcRefreshPeriod > 0) {
        double r = (double)ti.qpcRefreshPeriod / (double)*period;
        if (r > 0.97 && r < 1.03) {          /* DWM is timing this monitor: use its exact clock */
            *period = (LONGLONG)ti.qpcRefreshPeriod;
            *anchor = (LONGLONG)ti.qpcVBlank;
        }
    }
}

static DWORD WINAPI anim_thread(LPVOID unused) {
    LARGE_INTEGER li;
    LONGLONG period = 0, next = 0;
    int gliding = 0;
    (void)unused;
    QueryPerformanceFrequency(&li); qpcFreq = li.QuadPart;
    frameTimer = CreateWaitableTimerExW(NULL, NULL, CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS);
    if (!frameTimer) frameTimer = CreateWaitableTimerW(NULL, TRUE, NULL);   /* pre-1803 Windows */

    while (running) {
        int idle;
        EnterCriticalSection(&lock);
        idle = (pending == 0 && vel == 0 && !coastArmed && !momOn && softStrength == 0);
        LeaveCriticalSection(&lock);
        if (idle) {                       /* nothing to do: sleep until the next wheel tick */
            gliding = 0;
            WaitForSingleObject(wake, 200);
            continue;
        }

        if (!gliding) {                   /* new glide: lock onto the display's clock */
            LONGLONG anchor;
            frame_clock(&period, &anchor);
            QueryPerformanceCounter(&li);
            /* first slot: 40% into a frame, at least a quarter frame from now */
            next = anchor + period * 4 / 10;
            if (next < li.QuadPart + period / 4)
                next += ((li.QuadPart + period / 4 - next) / period + 1) * period;
            gliding = 1;
        }

        wait_until(next);
        QueryPerformanceCounter(&li);
        int frames = 1;
        if (li.QuadPart > next + period / 2)      /* we were late by whole frames: catch up */
            frames += (int)((li.QuadPart - next + period / 2) / period);
        if (frames > 6) frames = 6;
        next += (LONGLONG)frames * period;
        if (next < li.QuadPart) next = li.QuadPart + period / 2;

        /* Advance the glide by exactly one display frame per step, so steps are even. */
        double dt = (double)period / (double)qpcFreq;
        UINT wlines = 3;                  /* Windows' lines per wheel click */
        SystemParametersInfoA(SPI_GETWHEELSCROLLLINES, 0, &wlines, 0);
        if (wlines == 0 || wlines == WHEEL_PAGESCROLL || wlines > 100) wlines = (wlines == 0 ? 3 : 20);
        wlinesG = wlines;
        int emit = 0, f;
        EnterCriticalSection(&lock);
        if (coastArmed && !coasting) {
            /* Start coasting once the wheel has gone quiet for ~1.25 click intervals. */
            LARGE_INTEGER nq; QueryPerformanceCounter(&nq);
            double quiet = (double)(nq.QuadPart - lastClickQ) / (double)qpcFreq;
            double wait = lastRate > 0 ? 1.0 / lastRate : 0.08;
            if (wait < 0.03) wait = 0.03;
            if (quiet > wait && (vel != 0 || cfg.coast_wait)) {
                /* Hand off seamlessly: carry on at the speed on screen right now, and give
                   the spring the lead it needs to hold that speed (no dip, no surge). */
                double w = 4.74 / (cfg.duration_ms / 1000.0);
                coasting = 1; coastElapsed = 0;
                coastVel = vel;
                /* Optional fixed starting speed: the flick picks the direction, then the
                   coast settles to a calm pace (handy for steering with the rollerbar). The
                   spring eases from the flick's speed down to it - no sudden stop. */
                if (cfg.coast_start > 0) coastVel = (vel > 0 ? 1 : -1) * cfg.coast_start * WHEEL_DELTA / wlines;
                if (cfg.coast_wait) { coastVel = 0; coastHeld = 1; coastWaiting = 1; flickTail = 1; }
                double lead = 2.0 * coastVel / w;
                { POINT p; if (GetCursorPos(&p)) { steerAnchorY = p.y; steerAnchorX = p.x; } }
                PostMessageA(hwndMain, WM_COASTSTART, 0, 0);
                if (fabs(pending) < fabs(lead)) pending = lead;
            } else if (quiet > 0.5) coastArmed = 0;   /* glide already ended: nothing to carry on */
        }
        if (coasting) {
            /* Keep extending the glide's target at coastVel; the spring follows smoothly. */
            double minV = 360.0 / wlines;   /* below ~3 lines/s, stop */
            double stillV = 60.0 / wlines;  /* ~0.5 lines/s: "standing still" for coasts */
            /* A slow fixed starting speed must not count as "ran down" straight away. */
            double endV = cfg.coast_start > 0 && cfg.coast_start * WHEEL_DELTA / wlines * 0.3 < minV
                          ? cfg.coast_start * WHEEL_DELTA / wlines * 0.3 : minV;
            double maxV = cfg.coast_max > 0 ? cfg.coast_max * WHEEL_DELTA / wlines : 1e12;
            for (f = 0; f < frames; f++) {
                if (steerTargetOn) coastVel += (steerTarget - coastVel) * (1.0 - exp(-dt / 0.08));
                if (coastVel >  maxV) coastVel =  maxV;
                if (coastVel < -maxV) coastVel = -maxV;
                pending += coastVel * dt;
                coastElapsed += dt;
                /* A coast the user has steered or nudged holds its speed; otherwise it runs down:
                   exponentially at first, then (the tail) easing out to a true standstill. */
                if (!coastHeld && cfg.coast_time < 999) {
                    if (coastTail) {
                        tailT += dt;
                        double u = 1.0 - tailT / tailLen;
                        if (u < 0) u = 0;
                        coastVel = tailDir * tailV0 * u * u;
                    } else coastVel *= exp(-dt / (cfg.coast_time / 3.0));
                }
            }
            if (coastHeld) {
                coastTail = 0;
                /* Held coasts may pass through zero (to reverse). With "until I click" they
                   never end by themselves; otherwise one that sits still for 1 s ends. */
                if (fabs(coastVel) < stillV && !cfg.until_click && !coastWaiting) slowTime += frames * dt; else slowTime = 0;
                if (slowTime > 1.0) { coastArmed = coasting = 0; coastVel = 0; coastHeld = steerEngaged = 0; pending = 0; vel = 0; }
            } else if (cfg.coast_time <= 0) {
                coastArmed = coasting = 0; coastVel = 0;
            } else if (!coastTail && fabs(coastVel) < endV) {
                /* Ran down: instead of stopping dead at ~3 lines/s, ease the speed out to zero
                   (v0 * (1 - t/T)^2). T = 2 * tau matches the exponential's slowing at the
                   hand-off, kept to about a second. */
                coastTail = 1;
                tailDir = coastVel > 0 ? 1 : -1;
                tailV0 = fabs(coastVel); tailT = 0;
                tailLen = 2.0 * cfg.coast_time / 3.0;
                if (tailLen < 0.6) tailLen = 0.6;
                if (tailLen > 1.2) tailLen = 1.2;
            } else if (coastTail && (tailT >= tailLen || fabs(coastVel) < stillV)) {
                coastArmed = coasting = 0; coastVel = 0; coastTail = 0;
            }
            if (coastElapsed > 1800) stop_all();   /* safety: 30 min */
        }
        if (!coastArmed && !coasting && !momOn && softStrength > 0) {
            /* Momentum: once the wheel goes quiet after a fast run that wasn't a flick, keep
               the speed on screen and let it run down quickly - the closer the run came to a
               flick, the longer (up to a 0.6 s time constant, well short of a coast). */
            LARGE_INTEGER nq; QueryPerformanceCounter(&nq);
            double quiet = (double)(nq.QuadPart - lastClickQ) / (double)qpcFreq;
            double wait = lastRate > 0 ? 1.0 / lastRate : 0.08;
            if (wait < 0.03) wait = 0.03;
            if (quiet > wait) {
                double maxTau = cfg.coast_time / 3.0;
                if (maxTau > 0.6) maxTau = 0.6;
                double tau = softStrength * maxTau;
                softStrength = 0;
                if (tau > 0.05 && vel != 0) {
                    double w = 4.74 / (cfg.duration_ms / 1000.0);
                    double lead = 2.0 * vel / w;
                    momOn = 1; momTail = 0; momVel = vel; momTau = tau; momT = 0;
                    momLen = 2.0 * tau;
                    if (momLen < 0.3) momLen = 0.3;
                    if (momLen > 1.0) momLen = 1.0;
                    if (fabs(pending) < fabs(lead)) pending = lead;
                }
            }
        }
        if (momOn) {
            /* Same shape as a coast's run-down: exponential, then an ease-out tail. */
            double minV = 360.0 / wlines, stillV = 60.0 / wlines;
            for (f = 0; f < frames && momOn; f++) {
                pending += momVel * dt;
                momT += dt;
                if (!momTail) {
                    momVel *= exp(-dt / momTau);
                    if (fabs(momVel) < minV) {
                        momTail = 1; momT = 0;
                        momDir = momVel > 0 ? 1 : -1; momV0 = fabs(momVel);
                    }
                } else {
                    double u = 1.0 - momT / momLen;
                    if (u < 0) u = 0;
                    momVel = momDir * momV0 * u * u;
                    if (u == 0 || fabs(momVel) < stillV) { momOn = 0; momVel = 0; }
                }
            }
        }
        for (f = 0; f < frames && pending != 0; f++) {
            /* Critically damped spring: velocity ramps up and down smoothly.
               ~95% of the distance is covered within duration_ms. */
            double w = 4.74 / (cfg.duration_ms / 1000.0);
            vel += (w * w * pending - 2.0 * w * vel) * dt;
            double step = vel * dt;
            if (step * pending < 0 && !coasting) { vel = 0; step = 0; }   /* never scroll backwards (except steering a coast) */
            /* Minimum speed: at least 1 whole unit every frame. Without it the glide's
               tail crawls in sub-unit amounts, which come out as an uneven
               move/pause/move stutter right as the scroll comes to rest. Kept at 1 unit
               (not a fixed lines/s) so the spring can ease out almost to a standstill. */
            if (fabs(step) < 1) step = pending > 0 ? 1 : -1;
            if (fabs(step) > fabs(pending)) step = pending;        /* never overshoot */
            pending -= step;
            carry += step;
            if (fabs(pending) < 0.5) { carry += pending; pending = 0; vel = 0; }
        }
        if (pending == 0 && !coasting && !momOn) { emit = (int)lround(carry); carry = 0; vel = 0; }
        else { emit = (int)carry; carry -= emit; }
        LeaveCriticalSection(&lock);

        if (emit) {
            INPUT in;
            ZeroMemory(&in, sizeof in);
            in.type = INPUT_MOUSE;
            in.mi.dwFlags = MOUSEEVENTF_WHEEL;
            in.mi.mouseData = (DWORD)emit;
            SendInput(1, &in, sizeof in);
        }
    }
    if (frameTimer) CloseHandle(frameTimer);
    return 0;
}

/* ------------------------------------------------------- preferences UI */
#include <commctrl.h>
#define ID_TB0   400   /* sliders: 400..439 */
#define ID_VAL0  440   /* their value labels: 440..479 */
#define ID_LIST  220
#define ID_NAME  221
#define ID_ASPD  222
#define ID_AOFF  223
#define ID_ASET  224
#define ID_ADEL  225
#define ID_TEST  226
#define ID_APPLY 227
#define ID_PICK  228
#define PICK_BASE 5000
#define ID_PREFS 1006

static HWND hPrefs;
static AppRule wk[MAX_APPS];   /* working copy of per-app rules */
static int nwk;
#define NTB 11
typedef char ntb_fits_id_range[(NTB <= 40) ? 1 : -1];
#define ID_STEER 230
#define ID_OPPSTOP 231
#define ID_UNTIL 232
#define ID_INDIC 233
#define ID_RKEY 234
static const char *tbName[NTB] = {"Distance per wheel click", "Glide duration (ms)", "Acceleration",
                                "Max acceleration", "Acceleration starts at",
                                  "Flick coasting", "Flick speed to coast",
                                  "Max coast speed", "Steering sensitivity", "Clicks needed to coast",
                                  "Coast starting speed"};
static const int tbOrder[NTB] = {0, 1, 2, 3, 4, 5, 10, 6, 9, 7, 8};   /* display order of the sliders */
static const int tbMin[NTB] = {2, 5, 0, 10, 2, 0, 8, 1, 1, 2, 0};
static const int tbMax[NTB] = {200, 80, 200, 100, 20, 21, 40, 31, 30, 12, 19};   /* start: 0 = as flicked, 1 = wait (ready), 2..19 = startSpeeds[1..18] */   /* coast: 0=off, 1..20 = 0.5..10 s, 21 = until stopped; max: 31 = no limit */
static const double tbScale[NTB] = {0.1, 10, 0.01, 0.1, 1, 0.5, 1, 10, 0.1, 1, 1};
/* Coast starting speed choices (lines/s): fine steps where slow reading speeds live. */
static const int startSpeeds[19] = {0, 1, 2, 3, 4, 5, 6, 8, 10, 12, 15, 20, 25, 30, 40, 50, 60, 80, 100};
static int start_pos(double v) {
    int k, best = 0;
    for (k = 1; k < 19; k++) if (fabs(startSpeeds[k] - v) < fabs(startSpeeds[best] - v)) best = k;
    return v <= 0 ? 0 : best;
}

static void save_config(void) {
    char b[64]; int i;
    snprintf(b, sizeof b, "%g", cfg.speed);        WritePrivateProfileStringA("general", "speed", b, iniPath);
    snprintf(b, sizeof b, "%g", cfg.duration_ms);  WritePrivateProfileStringA("general", "duration_ms", b, iniPath);
    snprintf(b, sizeof b, "%g", cfg.accel);        WritePrivateProfileStringA("general", "acceleration", b, iniPath);
    snprintf(b, sizeof b, "%g", cfg.accel_max);    WritePrivateProfileStringA("general", "acceleration_max", b, iniPath);
    snprintf(b, sizeof b, "%g", cfg.accel_start);  WritePrivateProfileStringA("general", "acceleration_starts_at", b, iniPath);
    WritePrivateProfileStringA("general", "acceleration_window_ms", NULL, iniPath); /* old setting */
    if (cfg.coast_time >= 999) snprintf(b, sizeof b, "forever"); else snprintf(b, sizeof b, "%g", cfg.coast_time);
    WritePrivateProfileStringA("general", "coast_seconds", b, iniPath);
    snprintf(b, sizeof b, "%g", cfg.flick_rate);   WritePrivateProfileStringA("general", "flick_clicks_per_s", b, iniPath);
    snprintf(b, sizeof b, "%d", cfg.flick_clicks); WritePrivateProfileStringA("general", "flick_clicks_needed", b, iniPath);
    if (cfg.coast_wait) snprintf(b, sizeof b, "wait");
    else if (cfg.coast_start <= 0) snprintf(b, sizeof b, "flick"); else snprintf(b, sizeof b, "%g", cfg.coast_start);
    WritePrivateProfileStringA("general", "coast_start_lines_per_s", b, iniPath);
    if (cfg.coast_max <= 0) snprintf(b, sizeof b, "none"); else snprintf(b, sizeof b, "%g", cfg.coast_max);
    WritePrivateProfileStringA("general", "coast_max_lines_per_s", b, iniPath);
    WritePrivateProfileStringA("general", "steer_with_pointer", cfg.steer == 2 ? "2" : cfg.steer == 1 ? "1" : "0", iniPath);
    snprintf(b, sizeof b, "%g", cfg.steer_sens);   WritePrivateProfileStringA("general", "steer_sensitivity", b, iniPath);
    WritePrivateProfileStringA("general", "opposite_click_stops_coast", cfg.opp_stops ? "1" : "0", iniPath);
    WritePrivateProfileStringA("general", "steered_coast_until_click", cfg.until_click ? "1" : "0", iniPath);
    WritePrivateProfileStringA("general", "show_coast_indicator", cfg.indicator ? "1" : "0", iniPath);
    WritePrivateProfileStringA("general", "ready_shortcut", cfg.ready_key == 1 ? "alt+middle" : cfg.ready_key == 2 ? "alt+g" : "off", iniPath);
    WritePrivateProfileStringA("apps", NULL, NULL, iniPath);
    for (i = 0; i < cfg.napps; i++) {
        if (cfg.apps[i].disabled) snprintf(b, sizeof b, "off");
        else snprintf(b, sizeof b, "%g", cfg.apps[i].speed);
        WritePrivateProfileStringA("apps", cfg.apps[i].name, b, iniPath);
    }
    note_ini_stamp();
}

static void tb_label(int i) {
    char b[48];
    int pos = (int)SendDlgItemMessageA(hPrefs, ID_TB0 + i, TBM_GETPOS, 0, 0);
    double v = pos * tbScale[i];
    if (i == 0) {
        /* Show the real distance: multiplier x Windows' "lines per notch" setting. */
        UINT lines = 3;
        SystemParametersInfoA(SPI_GETWHEELSCROLLLINES, 0, &lines, 0);
        if (lines == WHEEL_PAGESCROLL) snprintf(b, sizeof b, "%.1f pages", v);
        else snprintf(b, sizeof b, "%.1f lines", v * lines);
    }
    else if (i == 1) snprintf(b, sizeof b, "%d ms", (int)v);
    else if (i == 4 || i == 6) snprintf(b, sizeof b, "%d clicks/s", (int)v);
    else if (i == 7) {
        if (pos >= 31) snprintf(b, sizeof b, "No limit");
        else snprintf(b, sizeof b, "%d lines/s", (int)v);
    }
    else if (i == 8) snprintf(b, sizeof b, "%.1fx", v);
    else if (i == 9) snprintf(b, sizeof b, "%d clicks", (int)v);
    else if (i == 10) {
        if (pos <= 0 || pos > 19) snprintf(b, sizeof b, "As flicked");
        else if (pos == 1) snprintf(b, sizeof b, "Wait (ready)");
        else snprintf(b, sizeof b, "%d line%s/s", startSpeeds[pos - 1], startSpeeds[pos - 1] == 1 ? "" : "s");
    }
    else if (i == 5) {
        if (pos == 0) snprintf(b, sizeof b, "Off");
        else if (pos >= 21) snprintf(b, sizeof b, "Until stopped");
        else snprintf(b, sizeof b, "%.1f s", v);
    }
    else if (i == 3) snprintf(b, sizeof b, "%.1fx", v);
    else snprintf(b, sizeof b, "%.2f", v);
    SetDlgItemTextA(hPrefs, ID_VAL0 + i, b);
}

static void refresh_list(void) {
    char b[128]; int i;
    SendDlgItemMessageA(hPrefs, ID_LIST, LB_RESETCONTENT, 0, 0);
    for (i = 0; i < nwk; i++) {
        if (wk[i].disabled) snprintf(b, sizeof b, "%.80s   -   off", wk[i].name);
        else snprintf(b, sizeof b, "%.80s   -   %.2fx", wk[i].name, wk[i].speed);
        SendDlgItemMessageA(hPrefs, ID_LIST, LB_ADDSTRING, 0, (LPARAM)b);
    }
}

static void apply_prefs(void) {
    int i;
    EnterCriticalSection(&lock);
    cfg.speed        = SendDlgItemMessageA(hPrefs, ID_TB0 + 0, TBM_GETPOS, 0, 0) * tbScale[0];
    cfg.duration_ms  = SendDlgItemMessageA(hPrefs, ID_TB0 + 1, TBM_GETPOS, 0, 0) * tbScale[1];
    cfg.accel        = SendDlgItemMessageA(hPrefs, ID_TB0 + 2, TBM_GETPOS, 0, 0) * tbScale[2];
    cfg.accel_max    = SendDlgItemMessageA(hPrefs, ID_TB0 + 3, TBM_GETPOS, 0, 0) * tbScale[3];
    cfg.accel_start  = SendDlgItemMessageA(hPrefs, ID_TB0 + 4, TBM_GETPOS, 0, 0) * tbScale[4];
    {
        int cp = (int)SendDlgItemMessageA(hPrefs, ID_TB0 + 5, TBM_GETPOS, 0, 0);
        cfg.coast_time = cp >= 21 ? 999 : cp * tbScale[5];
    }
    cfg.flick_rate   = SendDlgItemMessageA(hPrefs, ID_TB0 + 6, TBM_GETPOS, 0, 0) * tbScale[6];
    {
        int mp = (int)SendDlgItemMessageA(hPrefs, ID_TB0 + 7, TBM_GETPOS, 0, 0);
        cfg.coast_max = mp >= 31 ? 0 : mp * tbScale[7];
    }
    cfg.steer_sens   = SendDlgItemMessageA(hPrefs, ID_TB0 + 8, TBM_GETPOS, 0, 0) * tbScale[8];
    cfg.flick_clicks = (int)SendDlgItemMessageA(hPrefs, ID_TB0 + 9, TBM_GETPOS, 0, 0);
    { int sp = (int)SendDlgItemMessageA(hPrefs, ID_TB0 + 10, TBM_GETPOS, 0, 0);
      cfg.coast_wait = sp == 1;
      cfg.coast_start = (sp >= 2 && sp <= 19) ? startSpeeds[sp - 1] : 0; }
    { int rk = (int)SendDlgItemMessageA(hPrefs, ID_RKEY, CB_GETCURSEL, 0, 0); cfg.ready_key = rk < 0 ? 0 : rk; }
    { int sv = (int)SendDlgItemMessageA(hPrefs, ID_STEER, CB_GETCURSEL, 0, 0); cfg.steer = sv < 0 ? 0 : sv; }
    cfg.opp_stops    = IsDlgButtonChecked(hPrefs, ID_OPPSTOP) == BST_CHECKED;
    cfg.until_click  = IsDlgButtonChecked(hPrefs, ID_UNTIL) == BST_CHECKED;
    cfg.indicator    = IsDlgButtonChecked(hPrefs, ID_INDIC) == BST_CHECKED;
    cfg.napps = nwk;
    for (i = 0; i < nwk; i++) cfg.apps[i] = wk[i];
    stop_all();
    LeaveCriticalSection(&lock);
    save_config();
    update_hotkey();
}

#define ID_STATUS 229
static int dirty, loading, fieldsEdited;

static void set_dirty(int d) {
    dirty = d;
    EnableWindow(GetDlgItem(hPrefs, ID_APPLY), d);
    if (d) SetDlgItemTextA(hPrefs, ID_STATUS, "");
}

/* Save the name/speed/disable fields into the working rule list.
   quiet = called from Apply/OK: skip silently if the name box is empty. */
static void commit_app_fields(HWND h, int quiet) {
    char n[64], sp[32]; int k, found = -1;
    GetDlgItemTextA(h, ID_NAME, n, sizeof n);
    GetDlgItemTextA(h, ID_ASPD, sp, sizeof sp);
    StrTrimA(n, " \t");
    if (!n[0]) {
        if (!quiet) MessageBoxA(h, "Enter an app name such as chrome.exe", "Screzy", MB_ICONINFORMATION);
        return;
    }
    for (k = 0; k < nwk; k++) if (_stricmp(wk[k].name, n) == 0) found = k;
    if (found < 0) { if (nwk >= MAX_APPS) return; found = nwk++; }
    snprintf(wk[found].name, sizeof wk[found].name, "%s", n);
    wk[found].disabled = IsDlgButtonChecked(h, ID_AOFF) == BST_CHECKED;
    wk[found].speed = atof(sp);
    if (!wk[found].disabled && wk[found].speed <= 0) wk[found].speed = 1.0;
    refresh_list();
    SendDlgItemMessageA(h, ID_LIST, LB_SETCURSEL, found, 0);
    fieldsEdited = 0;
    set_dirty(1);
}

static void set_font(HWND h) {
    HFONT f = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
    HWND c = GetWindow(h, GW_CHILD);
    for (; c; c = GetWindow(c, GW_HWNDNEXT)) SendMessageA(c, WM_SETFONT, (WPARAM)f, TRUE);
}

/* ---- running-app picker: popup menu of exe names that own a visible window */
static char pickNames[64][64];
static int  npick;

static BOOL CALLBACK enum_win(HWND w, LPARAM lp) {
    DWORD pid = 0; char path[MAX_PATH] = "", *base; int i;
    (void)lp;
    if (!IsWindowVisible(w) || GetWindow(w, GW_OWNER) || npick >= 64) return TRUE;
    if (GetWindowLongA(w, GWL_EXSTYLE) & WS_EX_TOOLWINDOW) return TRUE;
    if (GetWindowTextLengthA(w) == 0) return TRUE;
    GetWindowThreadProcessId(w, &pid);
    if (!pid) return TRUE;
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!h) return TRUE;
    DWORD sz = sizeof path;
    QueryFullProcessImageNameA(h, 0, path, &sz);
    CloseHandle(h);
    base = PathFindFileNameA(path);
    if (!base[0] || strlen(base) >= 64) return TRUE;
    for (i = 0; i < npick; i++) if (_stricmp(pickNames[i], base) == 0) return TRUE;
    strcpy(pickNames[npick++], base);
    return TRUE;
}

static int cmp_names(const void *a, const void *b) { return _stricmp((const char *)a, (const char *)b); }

static void pick_running_app(HWND h) {
    int i; HMENU m; POINT pt; int cmd;
    npick = 0;
    EnumWindows(enum_win, 0);
    if (!npick) { MessageBoxA(h, "No running apps found.", "Screzy", MB_ICONINFORMATION); return; }
    qsort(pickNames, npick, 64, cmp_names);
    m = CreatePopupMenu();
    for (i = 0; i < npick; i++) AppendMenuA(m, MF_STRING, PICK_BASE + i, pickNames[i]);
    GetCursorPos(&pt);
    cmd = TrackPopupMenu(m, TPM_RETURNCMD | TPM_RIGHTBUTTON, pt.x, pt.y, 0, h, NULL);
    DestroyMenu(m);
    if (cmd >= PICK_BASE && cmd < PICK_BASE + npick) {
        int k, found = -1; char b[32];
        SetDlgItemTextA(h, ID_NAME, pickNames[cmd - PICK_BASE]);
        /* if a rule already exists, load it; else reset fields to defaults */
        for (k = 0; k < nwk; k++) if (_stricmp(wk[k].name, pickNames[cmd - PICK_BASE]) == 0) found = k;
        snprintf(b, sizeof b, "%g", found >= 0 && !wk[found].disabled ? wk[found].speed : 1.0);
        SetDlgItemTextA(h, ID_ASPD, b);
        CheckDlgButton(h, ID_AOFF, found >= 0 && wk[found].disabled ? BST_CHECKED : BST_UNCHECKED);
        SetFocus(GetDlgItem(h, ID_ASPD));
    }
}

/* Scale a 96-dpi layout value to the display's DPI (Screzy is system-DPI aware). */
static int ui_dpi(void) {
    static int dpi;
    if (!dpi) { HDC dc = GetDC(NULL); dpi = GetDeviceCaps(dc, LOGPIXELSY); ReleaseDC(NULL, dc); if (dpi <= 0) dpi = 96; }
    return dpi;
}
static int S(int v) { return MulDiv(v, ui_dpi(), 96); }

/* Slider positions (in each slider's own units) for the settings in memory. */
static void slider_positions(double cur[NTB]) {
    cur[0] = cfg.speed / tbScale[0]; cur[1] = cfg.duration_ms / tbScale[1];
    cur[2] = cfg.accel / tbScale[2]; cur[3] = cfg.accel_max / tbScale[3];
    cur[4] = cfg.accel_start / tbScale[4];
    cur[5] = cfg.coast_time >= 999 ? 21 : cfg.coast_time / tbScale[5];
    cur[6] = cfg.flick_rate / tbScale[6];
    cur[7] = cfg.coast_max <= 0 ? 31 : cfg.coast_max / tbScale[7];
    cur[8] = cfg.steer_sens / tbScale[8];
    cur[9] = cfg.flick_clicks;
    cur[10] = cfg.coast_wait ? 1 : cfg.coast_start > 0 ? start_pos(cfg.coast_start) + 1 : 0;
}

/* Fill every Preferences control from the settings in memory. Used when the window opens, and again
   when the settings are reloaded from the ini while it is open. */
static void sync_prefs(HWND h) {
    double cur[NTB]; int i;
    slider_positions(cur);
    for (i = 0; i < NTB; i++) {
        int p = (int)(cur[i] + 0.5);
        if (p < tbMin[i]) p = tbMin[i];
        if (p > tbMax[i]) p = tbMax[i];
        SendDlgItemMessageA(h, ID_TB0 + i, TBM_SETPOS, TRUE, p);
    }
    SendDlgItemMessageA(h, ID_STEER, CB_SETCURSEL, cfg.steer, 0);
    SendDlgItemMessageA(h, ID_RKEY, CB_SETCURSEL, cfg.ready_key, 0);
    CheckDlgButton(h, ID_OPPSTOP, cfg.opp_stops ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(h, ID_UNTIL, cfg.until_click ? BST_CHECKED : BST_UNCHECKED);
    CheckDlgButton(h, ID_INDIC, cfg.indicator ? BST_CHECKED : BST_UNCHECKED);
    nwk = cfg.napps;
    for (i = 0; i < nwk; i++) wk[i] = cfg.apps[i];
    refresh_list();
    for (i = 0; i < NTB; i++) tb_label(i);
    loading = 1;
    SetDlgItemTextA(h, ID_NAME, "");
    SetDlgItemTextA(h, ID_ASPD, "1.0");
    CheckDlgButton(h, ID_AOFF, BST_UNCHECKED);
    loading = 0;
    fieldsEdited = 0;
    set_dirty(0);
}

static void update_hotkey(void);

/* Re-read the ini and refresh an open Preferences window. Unless `force`, this does nothing when the file
   is as we last read or wrote it, so it's cheap enough to call whenever the tray menu or Preferences opens. */
static void reload_settings(int force) {
    WIN32_FILE_ATTRIBUTE_DATA d;
    if (!force && (!GetFileAttributesExA(iniPath, GetFileExInfoStandard, &d) ||
                   CompareFileTime(&d.ftLastWriteTime, &iniStamp) == 0)) return;
    load_config();
    update_hotkey();
    if (hPrefs) sync_prefs(hPrefs);
}

static LRESULT CALLBACK prefs_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    int i;
    switch (msg) {
    case WM_CREATE: {
        HINSTANCE hi = GetModuleHandleA(NULL);
        double cur[NTB];
        static char *test;
        hPrefs = h;
        slider_positions(cur);
        /* Layout is written in 96-dpi units and scaled to the display (125%, 150%...),
           so text drawn at the system's size always has room. */
#define MK(cls, txt, style, x, y, w, ht, id) \
        CreateWindowA(cls, txt, WS_CHILD | WS_VISIBLE | (style), S(x), S(y), S(w), S(ht), h, (HMENU)(INT_PTR)(id), hi, NULL)
        MK("BUTTON", "Scrolling", BS_GROUPBOX, 10, 6, 540, 388, 0);
        for (int r = 0; r < NTB; r++) {
            i = tbOrder[r];
            int y = 26 + r * 26, p = (int)(cur[i] + 0.5);
            MK("STATIC", tbName[i], 0, 22, y + 3, 162, 18, 0);
            HWND tb = MK(TRACKBAR_CLASSA, "", WS_TABSTOP | TBS_NOTICKS, 186, y, 256, 24, ID_TB0 + i);
            SendMessageA(tb, TBM_SETRANGE, TRUE, MAKELONG(tbMin[i], tbMax[i]));
            if (p < tbMin[i]) p = tbMin[i];
            if (p > tbMax[i]) p = tbMax[i];
            SendMessageA(tb, TBM_SETPOS, TRUE, p);
            MK("STATIC", "", 0, 448, y + 3, 98, 18, ID_VAL0 + i);
        }
        MK("BUTTON", "Keep steered coasts until I click",
           WS_TABSTOP | BS_AUTOCHECKBOX, 22, 316, 250, 20, ID_UNTIL);
        MK("BUTTON", "Show coast indicator by the pointer",
           WS_TABSTOP | BS_AUTOCHECKBOX, 282, 316, 262, 20, ID_INDIC);
        MK("BUTTON", "Opposite wheel click stops a coast",
           WS_TABSTOP | BS_AUTOCHECKBOX, 22, 340, 250, 20, ID_OPPSTOP);

        MK("STATIC", "Steer coasts with mouse / rollerbar", 0, 22, 369, 210, 18, 0);
        {
            HWND cb = MK("COMBOBOX", "", WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, 236, 365, 196, 120, ID_STEER);
            SendMessageA(cb, CB_ADDSTRING, 0, (LPARAM)"Off");
            SendMessageA(cb, CB_ADDSTRING, 0, (LPARAM)"Push (pointer stays put)");
            SendMessageA(cb, CB_ADDSTRING, 0, (LPARAM)"Distance from start");
            SendMessageA(cb, CB_SETCURSEL, cfg.steer, 0);
        }
        MK("STATIC", "Ready shortcut", 0, 282, 343, 92, 18, 0);
        {
            HWND cb = MK("COMBOBOX", "", WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST, 376, 339, 166, 120, ID_RKEY);
            SendMessageA(cb, CB_ADDSTRING, 0, (LPARAM)"Off");
            SendMessageA(cb, CB_ADDSTRING, 0, (LPARAM)"Alt + middle click");
            SendMessageA(cb, CB_ADDSTRING, 0, (LPARAM)"Alt + G");
            SendMessageA(cb, CB_SETCURSEL, cfg.ready_key, 0);
        }
        MK("BUTTON", "Per-app rules", BS_GROUPBOX, 10, 400, 540, 118, 0);
        MK("LISTBOX", "", WS_TABSTOP | WS_BORDER | WS_VSCROLL | LBS_NOTIFY, 22, 420, 236, 88, ID_LIST);
        MK("STATIC", "App .exe name (e.g. chrome.exe)", 0, 270, 418, 270, 16, 0);
        MK("EDIT", "", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 270, 434, 142, 22, ID_NAME);
        MK("BUTTON", "Running apps...", WS_TABSTOP, 418, 433, 122, 24, ID_PICK);
        CreateWindowW(L"STATIC", L"Distance \u00D7", WS_CHILD | WS_VISIBLE, S(270), S(466), S(68), S(16), h, NULL, hi, NULL);
        MK("EDIT", "1.0", WS_TABSTOP | WS_BORDER | ES_AUTOHSCROLL, 340, 462, 44, 22, ID_ASPD);
        MK("BUTTON", "Disable smoothing", WS_TABSTOP | BS_AUTOCHECKBOX, 394, 464, 150, 20, ID_AOFF);
        MK("BUTTON", "Add / Update", WS_TABSTOP, 270, 490, 112, 24, ID_ASET);
        MK("BUTTON", "Remove", WS_TABSTOP, 392, 490, 92, 24, ID_ADEL);

        MK("STATIC", "Try it - scroll here with your new settings (press Apply first):", 0, 12, 526, 530, 16, 0);
        MK("EDIT", "", WS_BORDER | WS_VSCROLL | ES_MULTILINE | ES_READONLY, 10, 544, 540, 46, ID_TEST);
        MK("STATIC", "", 0, 12, 608, 260, 18, ID_STATUS);
        MK("BUTTON", "OK", WS_TABSTOP | BS_DEFPUSHBUTTON, 290, 602, 80, 28, IDOK);
        MK("BUTTON", "Apply", WS_TABSTOP, 380, 602, 80, 28, ID_APPLY);
        MK("BUTTON", "Cancel", WS_TABSTOP, 470, 602, 80, 28, IDCANCEL);
#undef MK
        set_font(h);
        sync_prefs(h);
        test = (char *)malloc(200 * 64);
        test[0] = 0;
        for (i = 1; i <= 200; i++) { char l[64]; snprintf(l, sizeof l, "Line %d - the quick brown fox jumps over the lazy dog\r\n", i); strcat(test, l); }
        SetDlgItemTextA(h, ID_TEST, test);
        free(test);
        set_dirty(0);
        return 0;
    }
    case WM_HSCROLL:
        for (i = 0; i < NTB; i++) tb_label(i);
        set_dirty(1);
        return 0;
    case WM_COMMAND: {
        int id = LOWORD(wp), code = HIWORD(wp);
        if (id == ID_LIST && code == LBN_SELCHANGE) {
            int s = (int)SendDlgItemMessageA(h, ID_LIST, LB_GETCURSEL, 0, 0);
            if (s >= 0 && s < nwk) {
                char b[32];
                loading = 1;
                SetDlgItemTextA(h, ID_NAME, wk[s].name);
                snprintf(b, sizeof b, "%g", wk[s].disabled ? 1.0 : wk[s].speed);
                SetDlgItemTextA(h, ID_ASPD, b);
                CheckDlgButton(h, ID_AOFF, wk[s].disabled ? BST_CHECKED : BST_UNCHECKED);
                loading = 0;
                fieldsEdited = 0;
            }
        } else if ((id == ID_NAME || id == ID_ASPD) && code == EN_CHANGE) {
            if (!loading) { fieldsEdited = 1; set_dirty(1); }
        } else if ((id == ID_RKEY || id == ID_STEER) && code == CBN_SELCHANGE) {
            set_dirty(1);
        } else if ((id == ID_OPPSTOP || id == ID_UNTIL || id == ID_INDIC) && code == BN_CLICKED) {
            set_dirty(1);
        } else if (id == ID_AOFF && code == BN_CLICKED) {
            fieldsEdited = 1; set_dirty(1);
        } else if (id == ID_PICK) {
            pick_running_app(h);
        } else if (id == ID_ASET) {
            commit_app_fields(h, 0);
        } else if (id == ID_ADEL) {
            int s = (int)SendDlgItemMessageA(h, ID_LIST, LB_GETCURSEL, 0, 0);
            if (s >= 0 && s < nwk) {
                memmove(&wk[s], &wk[s + 1], (nwk - s - 1) * sizeof(AppRule));
                nwk--; refresh_list(); set_dirty(1);
            }
        } else if (id == ID_APPLY) {
            if (fieldsEdited) commit_app_fields(h, 1);
            apply_prefs();
            set_dirty(0);
            SetDlgItemTextA(h, ID_STATUS, "Settings applied.");
            SetFocus(GetDlgItem(h, ID_TEST)); /* Apply is now greyed out; move focus somewhere useful */
        } else if (id == IDOK) {
            if (fieldsEdited) commit_app_fields(h, 1);
            apply_prefs(); DestroyWindow(h);
        } else if (id == IDCANCEL) DestroyWindow(h);
        return 0;
    }
    case WM_CLOSE: DestroyWindow(h); return 0;
    case WM_DESTROY: hPrefs = NULL; return 0;
    }
    return DefWindowProcA(h, msg, wp, lp);
}

static void show_prefs(void) {
    static int reg;
    HINSTANCE hi = GetModuleHandleA(NULL);
    reload_settings(0);     /* pick up edits made to the ini file */
    if (hPrefs) { SetForegroundWindow(hPrefs); return; }
    if (!reg) {
        WNDCLASSA wc; INITCOMMONCONTROLSEX ic = { sizeof ic, ICC_BAR_CLASSES };
        InitCommonControlsEx(&ic);
        ZeroMemory(&wc, sizeof wc);
        wc.lpfnWndProc = prefs_proc; wc.hInstance = hi; wc.lpszClassName = "ScrezyPrefs";
        wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
        wc.hCursor = LoadCursorA(NULL, IDC_ARROW);
        wc.hIcon = load_icon(101, 1);
        RegisterClassA(&wc);
        reg = 1;
    }
    RECT r = {0, 0, S(560), S(640)};
    AdjustWindowRect(&r, WS_CAPTION | WS_SYSMENU, FALSE);
    hPrefs = CreateWindowA("ScrezyPrefs", "Screzy Preferences", WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
                           CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top, NULL, NULL, hi, NULL);
    SetForegroundWindow(hPrefs);
}

/* ------------------------------------------------------- launch context */
/* Installed from the Microsoft Store (MSIX)? Then Windows manages "start at sign-in"
   through the package's StartupTask, and the Run registry key isn't used. */
static int is_packaged(void) {
    typedef LONG (WINAPI *GCPFN)(UINT32 *, PWSTR);
    static int cached = -1;
    if (cached < 0) {
        GCPFN f = (GCPFN)(void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "GetCurrentPackageFullName");
        UINT32 len = 0;
        cached = f && f(&len, NULL) == ERROR_INSUFFICIENT_BUFFER;
    }
    return cached;
}

/* ------------------------------------------- Store start-at-sign-in (WinRT) */
/* In the Store build the "start with Windows" switch is the package's StartupTask. MinGW has no
   plain-C header for it, so the few calls we need go through raw COM vtables, found at run time in
   combase.dll: no import library, no new dependency. Any failure just means "state unknown". */
enum { SU_OFF = 0, SU_USER_OFF = 1, SU_ON = 2, SU_POLICY_OFF = 3, SU_POLICY_ON = 4 };
#define SU_STR(s)  L##s, (UINT32)(sizeof(L##s) / sizeof(WCHAR) - 1)
typedef HRESULT (WINAPI *PfnInit)(int);
typedef HRESULT (WINAPI *PfnFactory)(void *, const GUID *, void **);
typedef HRESULT (WINAPI *PfnNewStr)(const WCHAR *, UINT32, void **);
typedef HRESULT (WINAPI *PfnFreeStr)(void *);
typedef HRESULT (WINAPI *PfnQI)(void *, const GUID *, void **);
typedef HRESULT (WINAPI *PfnGetPtr)(void *, void **);
typedef HRESULT (WINAPI *PfnGetInt)(void *, int *);
typedef HRESULT (WINAPI *PfnGetAsync)(void *, void *, void **);
typedef HRESULT (WINAPI *PfnVoid)(void *);
typedef ULONG (WINAPI *PfnRelease)(void *);
/* Vtable slots: 0-2 IUnknown, 3-5 IInspectable, then the interface's own methods. */
#define VT(p, slot, T)  ((T)(*(void ***)(p))[slot])

static void su_release(void *p) { if (p) VT(p, 2, PfnRelease)(p); }

/* Waits for an IAsyncOperation to leave "started" (AsyncStatus 0); true if it completed (1). */
static int su_wait(void *op, DWORD ms) {
    static const GUID iidAsyncInfo = { 0x36, 0, 0, { 0xC0, 0, 0, 0, 0, 0, 0, 0x46 } };
    void *info = NULL; int st = 0; DWORD t0 = GetTickCount();
    if (VT(op, 0, PfnQI)(op, &iidAsyncInfo, &info) < 0) return 0;
    while (VT(info, 7, PfnGetInt)(info, &st) >= 0 && st == 0 && GetTickCount() - t0 < ms) Sleep(2);
    su_release(info);
    return st == 1;
}

/* The IStartupTask for ScrezyStartup, or NULL. Release it with su_release(). */
static void *su_task(void) {
    static const GUID iidStatics = { 0xee5b60bd, 0xa148, 0x41a7, { 0xb2, 0x6e, 0xe8, 0xb8, 0x8a, 0x1e, 0x62, 0xf8 } };
    HMODULE cb = LoadLibraryA("combase.dll");
    PfnInit init = cb ? (PfnInit)(void *)GetProcAddress(cb, "RoInitialize") : NULL;
    PfnFactory factory = cb ? (PfnFactory)(void *)GetProcAddress(cb, "RoGetActivationFactory") : NULL;
    PfnNewStr newStr = cb ? (PfnNewStr)(void *)GetProcAddress(cb, "WindowsCreateString") : NULL;
    PfnFreeStr freeStr = cb ? (PfnFreeStr)(void *)GetProcAddress(cb, "WindowsDeleteString") : NULL;
    void *cls = NULL, *id = NULL, *statics = NULL, *op = NULL, *task = NULL;
    if (!init || !factory || !newStr || !freeStr) return NULL;
    init(1);   /* RO_INIT_MULTITHREADED; if the thread is already set up another way, that works too */
    if (newStr(SU_STR("Windows.ApplicationModel.StartupTask"), &cls) >= 0) {
        if (factory(cls, &iidStatics, &statics) >= 0 && newStr(SU_STR("ScrezyStartup"), &id) >= 0 &&
            VT(statics, 7, PfnGetAsync)(statics, id, &op) >= 0) {          /* IStartupTaskStatics::GetAsync */
            if (su_wait(op, 300)) VT(op, 8, PfnGetPtr)(op, &task);          /* IAsyncOperation::GetResults */
            su_release(op);
        }
        su_release(statics);
        if (id) freeStr(id);
        freeStr(cls);
    }
    return task;
}

/* SU_* state of the startup task, or -1 if it can't be read. */
static int su_state(void) {
    void *t = su_task(); int s = -1;
    if (t && VT(t, 8, PfnGetInt)(t, &s) < 0) s = -1;   /* IStartupTask::get_State */
    su_release(t);
    return s;
}

static void open_startup_settings(void) {
    ShellExecuteA(NULL, "open", "ms-settings:startupapps", NULL, NULL, SW_SHOW);
}

/* Off to on can make Windows ask the user, so this runs on its own thread, never the tray's. */
static DWORD WINAPI su_toggle_thread(LPVOID unused) {
    void *t = su_task(); int s = -1, done = 0;
    (void)unused;
    if (t && VT(t, 8, PfnGetInt)(t, &s) >= 0) {
        if (s == SU_ON) done = VT(t, 7, PfnVoid)(t) >= 0;                   /* Disable */
        else if (s == SU_OFF) {
            void *op = NULL; int now = -1;
            if (VT(t, 6, PfnGetPtr)(t, &op) >= 0) {                         /* RequestEnableAsync */
                if (su_wait(op, 120000)) VT(op, 8, PfnGetInt)(op, &now);
                su_release(op);
                done = now >= 0;                                            /* a "no" from the user is an answer too */
            }
        }
    }
    su_release(t);
    if (!done) open_startup_settings();
    return 0;
}

/* The tray item. Where Windows lets the app flip the switch it does; if the user turned Screzy off in
   Settings (or policy decides), only they can turn it back on, so show them the page. */
static void store_startup_click(void) {
    int s = su_state();
    if (s == SU_ON || s == SU_OFF) {
        HANDLE h = CreateThread(NULL, 0, su_toggle_thread, NULL, 0, NULL);
        if (h) { CloseHandle(h); return; }
    }
    open_startup_settings();
}

static ULONGLONG process_start(DWORD pid) {
    FILETIME c, e, k, u; ULONGLONG t = 0;
    HANDLE h = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (h) {
        if (GetProcessTimes(h, &c, &e, &k, &u)) t = ((ULONGLONG)c.dwHighDateTime << 32) | c.dwLowDateTime;
        CloseHandle(h);
    }
    return t;
}

/* Started automatically at sign-in (rather than by the user)? Either the Run-key
   launch passed --autostart, or we started within 90 s of the desktop (explorer). */
static int launched_at_signin(const char *cmd) {
    DWORD shellPid = 0; ULONGLONG me, shell;
    if (cmd && strstr(cmd, "--autostart")) return 1;
    GetWindowThreadProcessId(GetShellWindow(), &shellPid);
    if (!shellPid) return 0;
    me = process_start(GetCurrentProcessId());
    shell = process_start(shellPid);
    return me && shell && me >= shell && (me - shell) < 90ULL * 10000000ULL;
}

/* ------------------------------------------------------------ hook thread */
/* The low-level mouse hook runs on its own thread that does nothing else, at the highest
   priority. Windows waits for this hook on every mouse event, so it must never be held up
   by the tray menu, Preferences, the indicator or anything else in the app. */
static DWORD hookThreadId;
static HANDLE hookReady;

static DWORD WINAPI hook_thread(LPVOID hi) {
    MSG m;
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
    PeekMessageA(&m, NULL, 0, 0, PM_NOREMOVE);           /* create this thread's message queue */
    hook = SetWindowsHookExA(WH_MOUSE_LL, mouse_proc, (HINSTANCE)hi, 0);
    SetEvent(hookReady);
    while (GetMessageA(&m, NULL, 0, 0) > 0) { }          /* hook callbacks are delivered here */
    UnhookWindowsHookEx(hook);
    return 0;
}

/* ------------------------------------------------------- coast indicator */
/* A small badge beside the pointer while a coast runs: dot + both arrows when it's at a
   standstill ("ready"), dot + arrow for the direction it's moving. It's a layered,
   click-through, never-activated window, polled by a UI-thread timer only while shown. */
#define IND_TIMER 7
enum { IND_READY, IND_DOWN, IND_UP };
static HWND hInd;
static HDC indDC;
static HBITMAP indBmp[3];
static int indSize, indState = -1, indAlpha, indShown;

static void ind_init(HINSTANCE hi) {
    static const int sizes[4] = {32, 40, 48, 64};
    HDC dc = GetDC(NULL);
    int dpi = GetDeviceCaps(dc, LOGPIXELSY), want = MulDiv(32, dpi, 96), k, st;
    ReleaseDC(NULL, dc);
    for (k = 0; k < 3 && sizes[k] < want; k++) ;          /* smallest size >= wanted */
    indSize = sizes[k];
    indDC = CreateCompatibleDC(NULL);
    for (st = 0; st < 3; st++) {
        HRSRC r = FindResourceA(hi, MAKEINTRESOURCEA(300 + k * 3 + st), (LPCSTR)RT_RCDATA);
        BITMAPINFO bi; void *bits = NULL;
        if (!r || SizeofResource(hi, r) != (DWORD)(indSize * indSize * 4)) continue;
        ZeroMemory(&bi, sizeof bi);
        bi.bmiHeader.biSize = sizeof bi.bmiHeader;
        bi.bmiHeader.biWidth = indSize; bi.bmiHeader.biHeight = -indSize;   /* top-down */
        bi.bmiHeader.biPlanes = 1; bi.bmiHeader.biBitCount = 32; bi.bmiHeader.biCompression = BI_RGB;
        indBmp[st] = CreateDIBSection(NULL, &bi, DIB_RGB_COLORS, &bits, NULL, 0);
        if (indBmp[st] && bits) memcpy(bits, LockResource(LoadResource(hi, r)), indSize * indSize * 4);
    }
    WNDCLASSA wc; ZeroMemory(&wc, sizeof wc);
    wc.lpfnWndProc = DefWindowProcA; wc.hInstance = hi; wc.lpszClassName = "ScrezyIndicator";
    RegisterClassA(&wc);
    hInd = CreateWindowExA(WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
                           "ScrezyIndicator", "", WS_POPUP, 0, 0, indSize, indSize, NULL, NULL, hi, NULL);
}

static void ind_tick(HWND owner) {
    POINT cur; HMONITOR mon; MONITORINFO mi;
    int active = coasting && enabled && cfg.indicator && hInd;
    int st = IND_READY;
    if (active) {
        double minV = 60.0 / (double)wlinesG;               /* ~0.5 lines/s */
        if (coastVel > minV) st = IND_UP;                    /* positive wheel units = up */
        else if (coastVel < -minV) st = IND_DOWN;
    }
    /* fade in quickly, out a little slower */
    if (active) { indAlpha += 40; if (indAlpha > 255) indAlpha = 255; }
    else        { indAlpha -= 26; if (indAlpha < 0) indAlpha = 0; }
    if (!active && indAlpha == 0) {
        if (indShown) { ShowWindow(hInd, SW_HIDE); indShown = 0; }
        indState = -1;
        KillTimer(owner, IND_TIMER);
        return;
    }
    if (active) indState = st;
    if (indState < 0 || !indBmp[indState]) return;

    /* lower-right of the pointer, kept on the pointer's monitor; in distance steering it
       stays centred on the start point, as the reference you steer from */
    GetCursorPos(&cur);
    POINT pos = { cur.x + MulDiv(indSize, 9, 16), cur.y + MulDiv(indSize, 5, 8) };
    if (cfg.steer == 2) {
        pos.x = steerAnchorX - indSize / 2; pos.y = steerAnchorY - indSize / 2;
        cur.x = steerAnchorX; cur.y = steerAnchorY;
    }
    mon = MonitorFromPoint(cur, MONITOR_DEFAULTTONEAREST);
    mi.cbSize = sizeof mi;
    if (GetMonitorInfoA(mon, &mi)) {
        if (pos.x + indSize > mi.rcWork.right)  pos.x = cur.x - indSize - MulDiv(indSize, 3, 8);
        if (pos.y + indSize > mi.rcWork.bottom) pos.y = cur.y - indSize - MulDiv(indSize, 3, 8);
    }
    static POINT lastPos = { -99999, -99999 };
    static int lastState = -1, lastAlpha = -1;
    if (indShown && pos.x == lastPos.x && pos.y == lastPos.y && indState == lastState && indAlpha == lastAlpha)
        return;                                   /* nothing changed: don't make Windows recompose */
    lastPos = pos; lastState = indState; lastAlpha = indAlpha;
    SIZE sz = { indSize, indSize };
    POINT src = { 0, 0 };
    BLENDFUNCTION bf = { AC_SRC_OVER, 0, (BYTE)indAlpha, AC_SRC_ALPHA };
    HGDIOBJ old = SelectObject(indDC, indBmp[indState]);
    UpdateLayeredWindow(hInd, NULL, &pos, &sz, indDC, &src, 0, &bf, ULW_ALPHA);
    SelectObject(indDC, old);
    if (!indShown) {
        SetWindowPos(hInd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE | SWP_SHOWWINDOW);
        indShown = 1;
    }
}

/* ------------------------------------------------------------- tray / UI */
static int autostart_on(void) {
    HKEY k; int on = 0;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_READ, &k) == ERROR_SUCCESS) {
        on = RegQueryValueExA(k, "Screzy", NULL, NULL, NULL, NULL) == ERROR_SUCCESS;
        RegCloseKey(k);
    }
    return on;
}

static void set_autostart(int on) {
    HKEY k;
    if (RegOpenKeyExA(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_SET_VALUE, &k) != ERROR_SUCCESS) return;
    if (on) {
        char exe[MAX_PATH + 2];
        char exe2[MAX_PATH + 16];
        GetModuleFileNameA(NULL, exe + 1, MAX_PATH);
        exe[0] = '"'; strcat(exe, "\"");
        snprintf(exe2, sizeof exe2, "%s --autostart", exe);
        RegSetValueExA(k, "Screzy", 0, REG_SZ, (BYTE *)exe2, (DWORD)strlen(exe2) + 1);
        RegCloseKey(k);
        return;
    } else RegDeleteValueA(k, "Screzy");
    RegCloseKey(k);
}

/* One-time tidy-up after the rename from GlideScroll: close a still-running GlideScroll
   (two smoothers would fight) and move its start-with-Windows entry to Screzy. */
static void migrate_from_glidescroll(void) {
    HKEY k;
    HWND old = FindWindowA("GlideScrollWnd", NULL);
    if (!old) old = FindWindowExA(HWND_MESSAGE, NULL, "GlideScrollWnd", NULL);
    if (old) PostMessageA(old, WM_COMMAND, 1005 /* its Exit command */, 0);
    if (!is_packaged() && RegOpenKeyExA(HKEY_CURRENT_USER, RUN_KEY, 0, KEY_READ | KEY_SET_VALUE, &k) == ERROR_SUCCESS) {
        if (RegQueryValueExA(k, "GlideScroll", NULL, NULL, NULL, NULL) == ERROR_SUCCESS) {
            RegDeleteValueA(k, "GlideScroll");
            RegCloseKey(k);
            set_autostart(1);
            return;
        }
        RegCloseKey(k);
    }
}

static void update_tip(void) {
    snprintf(nid.szTip, sizeof nid.szTip, "Screzy - %s", enabled ? "on" : "off");
    nid.hIcon = enabled ? icoOn : icoOff;
    nid.uFlags = NIF_TIP | NIF_ICON;
    Shell_NotifyIconA(NIM_MODIFY, &nid);
}

static UINT wmTaskbarCreated;   /* broadcast when Explorer (and the tray) restarts */
#define HOTKEY_READY 1

/* Alt + G is registered with Windows, so we're told only when that exact combination is pressed. */
static void update_hotkey(void) {
    UnregisterHotKey(hwndMain, HOTKEY_READY);
    if (cfg.ready_key == 2 && !RegisterHotKey(hwndMain, HOTKEY_READY, MOD_ALT | 0x4000 /* MOD_NOREPEAT */, 'G'))
        MessageBoxA(NULL, "Alt + G is already used by another program, so it can't be the Ready shortcut.\n"
                          "Choose Alt + middle click in Preferences instead.", "Screzy", MB_OK | MB_ICONWARNING);
}

static LRESULT CALLBACK wnd_proc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    if (wmTaskbarCreated && msg == wmTaskbarCreated) {
        nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
        nid.hIcon = enabled ? icoOn : icoOff;
        Shell_NotifyIconA(NIM_ADD, &nid);
        return 0;
    }
    if (msg == WM_TRAY && lp == WM_LBUTTONDBLCLK) { show_prefs(); return 0; }
    if (msg == WM_SHOWPREFS) { show_prefs(); return 0; }
    if (msg == WM_HOTKEY && wp == HOTKEY_READY) {
        POINT pt; GetCursorPos(&pt);
        if (enabled && app_speed_at(pt) > 0) enter_ready(pt);
        return 0;
    }
    if (msg == WM_COASTSTART) { if (cfg.indicator && hInd) SetTimer(h, IND_TIMER, 15, NULL); return 0; }
    if (msg == WM_TIMER && wp == IND_TIMER) { ind_tick(h); return 0; }
    if (msg == WM_TRAY && (lp == WM_RBUTTONUP || lp == WM_LBUTTONUP)) {
        POINT pt; GetCursorPos(&pt);
        HMENU m;
        reload_settings(0);
        m = CreatePopupMenu();
        AppendMenuA(m, MF_STRING | (enabled ? MF_CHECKED : 0), ID_TOGGLE, "Smooth scrolling");
        AppendMenuA(m, MF_STRING, ID_PREFS, "Preferences...");
        if (is_packaged()) {
            /* Store install: Windows owns the startup switch (Settings > Apps > Startup). A "..." means
               the click opens that page because the app isn't allowed to change the switch itself. */
            int su = su_state();
            AppendMenuA(m, MF_STRING | (su == SU_ON || su == SU_POLICY_ON ? MF_CHECKED : 0), ID_STARTUPSET,
                        su == SU_ON || su == SU_OFF ? "Start with Windows" : "Start with Windows...");
        } else {
            AppendMenuA(m, MF_STRING, ID_SETTINGS, "Edit .ini file...");
            AppendMenuA(m, MF_STRING, ID_RELOAD, "Reload settings");
            AppendMenuA(m, MF_STRING | (autostart_on() ? MF_CHECKED : 0), ID_AUTOSTART, "Start with Windows");
        }
        AppendMenuA(m, MF_SEPARATOR, 0, NULL);
        AppendMenuA(m, MF_STRING, ID_WEBSITE, "Website");
        AppendMenuA(m, MF_STRING, ID_ABOUT, "About Screzy");
        AppendMenuA(m, MF_SEPARATOR, 0, NULL);
        AppendMenuA(m, MF_STRING, ID_EXIT, "Exit");
        SetForegroundWindow(h);
        TrackPopupMenu(m, TPM_RIGHTBUTTON, pt.x, pt.y, 0, h, NULL);
        DestroyMenu(m);
        return 0;
    }
    if (msg == WM_COMMAND) {
        switch (LOWORD(wp)) {
        case ID_TOGGLE:
            enabled = !enabled;
            EnterCriticalSection(&lock); stop_all(); LeaveCriticalSection(&lock);
            update_tip(); break;
        case ID_PREFS: show_prefs(); break;
        case ID_SETTINGS: ShellExecuteA(NULL, "open", "notepad.exe", iniPath, NULL, SW_SHOW); break;
        case ID_RELOAD:   reload_settings(1); break;
        case ID_AUTOSTART: set_autostart(!autostart_on()); break;
        case ID_STARTUPSET: store_startup_click(); break;
        case ID_WEBSITE: ShellExecuteA(NULL, "open", APP_URL, NULL, NULL, SW_SHOW); break;
        case ID_ABOUT:
            MessageBoxA(NULL,
                "Screzy " APP_VERSION "\n"
                "Scroll easy. Flick to coast. Steer with mouse or rollerbar.\n\n"
                "Copyright (c) 2026 Eranno AS\n"
                "Free and open source under the MIT License.\n\n"
                "screzy.com",
                "About Screzy", MB_OK | MB_ICONINFORMATION | MB_SETFOREGROUND);
            break;
        case ID_EXIT: DestroyWindow(h); break;
        }
        return 0;
    }
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProcA(h, msg, wp, lp);
}

int WINAPI WinMain(HINSTANCE hi, HINSTANCE hp, LPSTR cmd, int show) {
    (void)hp; (void)show;
    HANDLE single = CreateMutexA(NULL, TRUE, "Screzy_SingleInstance");
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        /* Already running in the tray: launching it again opens Preferences there. */
        HWND other = FindWindowA("ScrezyWnd", NULL);
        if (other) {
            DWORD pid = 0;
            GetWindowThreadProcessId(other, &pid);
            AllowSetForegroundWindow(pid);
            PostMessageA(other, WM_SHOWPREFS, 0, 0);
        }
        return 0;
    }

    InitializeCriticalSection(&lock);
    {   /* settings live in %APPDATA%\Screzy so they're always writable */
        HANDLE me; int ok;
        char oldIni[MAX_PATH];
        SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, iniPath);
        strcpy(oldIni, iniPath);
        PathAppendA(iniPath, "Screzy");
        CreateDirectoryA(iniPath, NULL);
        PathAppendA(iniPath, "screzy.ini");
        /* Screzy was called GlideScroll during development: carry its settings over once. */
        PathAppendA(oldIni, "GlideScroll\\glidescroll.ini");
        if (GetFileAttributesA(iniPath) == INVALID_FILE_ATTRIBUTES &&
            GetFileAttributesA(oldIni) != INVALID_FILE_ATTRIBUTES)
            CopyFileA(oldIni, iniPath, TRUE);
        me = GetCurrentProcess();
        self_elevated = token_elevated(me, &ok);
    }
    migrate_from_glidescroll();
    int firstRun = write_default_ini();
    load_config();

    wmTaskbarCreated = RegisterWindowMessageA("TaskbarCreated");
    WNDCLASSA wc; ZeroMemory(&wc, sizeof wc);
    wc.lpfnWndProc = wnd_proc; wc.hInstance = hi; wc.lpszClassName = "ScrezyWnd";
    RegisterClassA(&wc);
    hwndMain = CreateWindowExA(WS_EX_TOOLWINDOW, "ScrezyWnd", "Screzy", WS_POPUP, 0, 0, 0, 0,
                              NULL, NULL, hi, NULL);   /* hidden top-level: receives the TaskbarCreated broadcast */

    ZeroMemory(&nid, sizeof nid);
    nid.cbSize = sizeof nid; nid.hWnd = hwndMain; nid.uID = 1;
    nid.uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP;
    nid.uCallbackMessage = WM_TRAY;
    icoOn = load_icon(101, 0);
    icoOff = load_icon(102, 0);
    nid.hIcon = icoOn;
    strcpy(nid.szTip, "Screzy - on");
    Shell_NotifyIconA(NIM_ADD, &nid);

    ind_init(hi);
    update_hotkey();
    timeBeginPeriod(1);
    hookReady = CreateEventA(NULL, TRUE, FALSE, NULL);
    HANDLE ht = CreateThread(NULL, 0, hook_thread, hi, 0, &hookThreadId);
    WaitForSingleObject(hookReady, 2000);
    wake = CreateEventA(NULL, FALSE, FALSE, NULL);
    HANDLE th = CreateThread(NULL, 0, anim_thread, NULL, 0, NULL);
    SetThreadPriority(th, THREAD_PRIORITY_ABOVE_NORMAL);

    /* Opened by the user (Start menu, Store "Open", double-click)? Show Preferences so
       they can see it's running. Silent when started automatically at sign-in. */
    if (firstRun || !launched_at_signin(cmd)) show_prefs();

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0) > 0) { if (hPrefs && IsDialogMessageA(hPrefs, &msg)) continue; TranslateMessage(&msg); DispatchMessageA(&msg); }

    running = 0;
    SetEvent(wake);
    WaitForSingleObject(th, 500);
    PostThreadMessageA(hookThreadId, WM_QUIT, 0, 0);
    WaitForSingleObject(ht, 1000);
    Shell_NotifyIconA(NIM_DELETE, &nid);
    timeEndPeriod(1);
    CloseHandle(single);
    return 0;
}
