# Screzy Privacy Statement

*Effective 4 November 2026. Publisher: Eranno AS, Norway.*

**In short: Screzy collects nothing, sends nothing and connects to nothing.**

## What Screzy does on your computer

To smooth your scrolling, Screzy has to see some mouse input as it happens:

- **Mouse-wheel movement.** Screzy intercepts wheel clicks and replaces them with many small, smooth scroll steps. It needs this to work at all.
- **Mouse-button presses (left, middle, right).** Screzy only checks whether a button was pressed, and how far the pointer is from where the coast started, so that a click can stop a coasting scroll. When a click near that spot stops a coast, Screzy doesn't pass that click on. While a coast runs, Screzy also checks which window is under the pointer and which app is in front, so the coast stops if you move elsewhere. None of this is stored.
- **Up/down pointer movement, only in autoscroll.** With *A flick starts autoscroll instead of a coast* turned on, Screzy uses how far the pointer is above or below where the coast began to set its speed, until you click. At all other times, pointer movement is ignored.
- **The program name under the pointer** (for example `chrome.exe`). Screzy checks this so your per-app rules apply, such as a different scroll distance or smoothing turned off for one app.
- **The list of open programs.** Screzy reads this only when you press **Running apps…** in Preferences, so you can pick one from a menu.

Screzy handles all of this in memory, at the moment it happens. It does not record, log, store or send anything.

Screzy does **not** read your typing or any other keys, your screen contents, files, clipboard or browsing.

## What is stored

Screzy stores only your settings, in one small text file on your own computer:

- **Downloaded version:** `%APPDATA%\Screzy\screzy.ini`
- **Microsoft Store version:** the app's private storage folder, which Windows manages

The file holds your slider values and any per-app rules you add, which are program names such as `chrome.exe` with a distance multiplier or "off". To remove it, uninstall Screzy. With the downloaded version, also delete the folder above.

## Network and third parties

- Screzy makes **no network connections**.
- It has **no** analytics, telemetry, crash reporting, advertising or user accounts.
- The **Website** menu item opens screzy.com in your browser. Nothing happens unless you choose it. What applies there is described under *This website* below.
- If you install Screzy from the Microsoft Store, Microsoft handles the download and updates under its own privacy terms. Eranno AS gets no personal information from that.

## This website

screzy.com is a plain static site. It sets no cookies and has no analytics, tracking, ads or forms. Its fonts and images come from the site itself, not from other services.

The site is hosted by Cloudflare, which handles standard connection data such as your IP address to deliver and protect the site, under [Cloudflare's own privacy terms](https://www.cloudflare.com/privacypolicy/). Eranno AS doesn't use that data to identify or follow visitors. Links to GitHub and the Microsoft Store lead to sites with their own privacy terms.

## Children

Screzy collects no data from anyone, including children.

## Changes and contact

If this statement changes, the new version will be published at screzy.com/privacy and in the Screzy source repository, with a new effective date.

Questions? Email Eranno AS at <info@screzy.com>, or open an issue in the Screzy repository on GitHub.
