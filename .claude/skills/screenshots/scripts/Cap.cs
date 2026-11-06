// Small Win32 helpers for capture.ps1: find windows, post messages, grab pixels.
using System;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

public static class Cap {
    [DllImport("user32.dll")] static extern bool SetProcessDpiAwarenessContext(IntPtr v);
    [DllImport("user32.dll")] static extern uint GetDpiForSystem();
    [DllImport("user32.dll")] static extern int GetSystemMetrics(int i);
    [DllImport("user32.dll")] public static extern IntPtr FindWindow(string cls, string title);
    [DllImport("user32.dll")] public static extern IntPtr FindWindowEx(IntPtr p, IntPtr after, string cls, string title);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr h, uint m, IntPtr w, IntPtr l);
    [DllImport("user32.dll")] public static extern bool SetCursorPos(int x, int y);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] static extern bool GetWindowRect(IntPtr h, out RECT r);
    [DllImport("dwmapi.dll")] static extern int DwmGetWindowAttribute(IntPtr h, int a, out RECT r, int s);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr h);
    [StructLayout(LayoutKind.Sequential)] public struct RECT { public int L, T, R, B; }

    /* First *visible* top-level window of a class. Menus (#32768) leave hidden windows behind. */
    public static IntPtr FindVisible(string cls) {
        IntPtr h = IntPtr.Zero;
        while ((h = FindWindowEx(IntPtr.Zero, h, cls, null)) != IntPtr.Zero)
            if (IsWindowVisible(h)) { RECT r; GetWindowRect(h, out r); if (r.R - r.L > 20) return h; }
        return IntPtr.Zero;
    }

    /* Per-monitor DPI aware, so every coordinate below is in physical pixels. */
    public static void Init() { SetProcessDpiAwarenessContext(new IntPtr(-4)); }
    public static int Dpi() { return (int)GetDpiForSystem(); }
    public static int ScreenW() { return GetSystemMetrics(0); }
    public static int ScreenH() { return GetSystemMetrics(1); }

    /* Visible frame of a top-level window (without the invisible resize border). */
    public static int[] Frame(IntPtr h) {
        RECT d;
        if (DwmGetWindowAttribute(h, 9, out d, Marshal.SizeOf(typeof(RECT))) != 0) GetWindowRect(h, out d);
        return new int[] { d.L, d.T, d.R, d.B };
    }
    public static int[] Rect(IntPtr h) { RECT r; GetWindowRect(h, out r); return new int[] { r.L, r.T, r.R, r.B }; }

    /* Copy a rectangle of the screen to a PNG. The window must be on top and unobscured. */
    public static void Grab(int[] r, string path) {
        int w = r[2] - r[0], h = r[3] - r[1];
        using (var bmp = new Bitmap(w, h, PixelFormat.Format32bppArgb)) {
            using (var g = Graphics.FromImage(bmp)) g.CopyFromScreen(r[0], r[1], 0, 0, new Size(w, h));
            bmp.Save(path, ImageFormat.Png);
        }
    }
}
