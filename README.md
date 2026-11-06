# jmstudio-linux

Run **JMStudio**, the Windows software for the **3DMakerpro Moose** 3D scanner (and its siblings), on Linux through **Proton**.

> Deutsche Kurzfassung: [weiter unten](#deutsch).

This is an unofficial helper. It is not affiliated with 3DMakerpro or JMMeta. JMStudio is proprietary and **not included**: you download the installer yourself and this repo sets everything else up around it.

## Status: works on a Moose

JMStudio installs, starts, **recognises and connects to the Moose**, shows the live preview and records and processes scans. It has been tested by one person with one Moose, so treat it as experimental and please report what you see on other setups.

| Part | State |
| --- | --- |
| Proton Experimental prefix, `setup`, `run`, logs, menu entry | works (CachyOS, Proton Experimental 11.0 / Wine 11.0, JMStudio 2.6.17) |
| USB access (`udev` rules) | works, tested with a Moose (`bcdc:000a`, `bcdc:000b`) |
| **Camera fix** (patched Wine `devenum.dll`, `mf.dll` and a camera filter `qcapjm`) | JMStudio finds the scanner's serial number, switches from the default "CR-Scan Lizard" to **Moose**, loads `moosePlugin.dll`, finds both cameras and logs `Connect scanner successfully` |
| Camera capture | works in a test program using JMStudio's own OpenCV: both cameras open through DirectShow and deliver frames |
| Live preview | works (about 10-11 FPS, which is what the scanner's cameras deliver) |
| LED brightness (JMStudio's "Bright level") | works: JMStudio sets the cameras' exposure, which is what drives the LEDs, and Wine passes it on (exposure -13 gives V4L2 exposure 1; the LEDs get visibly darker in the program) |
| Scanning | works: scans are recorded and processed. Repeated scan / cancel / delete data / scan again cycles and switching between the scan and edit modes are stable (they used to end in "0 frames per second", see item 6 below) |

Without the camera fix JMStudio never identifies the scanner at all, so it is a required step.

## Install

```bash
curl -fsSL https://raw.githubusercontent.com/Ibot-11/jmstudio-linux/main/install.sh | bash
```

Prefer to read it first? Clone the repo and run `./install.sh`. The installer does everything in order and asks before each step that changes your system:

1. **Requirements:** Steam and Proton Experimental (it can open Steam's install dialog for you), and the build tools for the camera fix (it offers the matching `pacman`/`apt`/`dnf`/`zypper` command).
2. **JMStudio installer:** it cannot be downloaded automatically (JMMeta login), so the installer opens the download page and waits until the file appears in `~/Downloads`.
3. **Installs JMStudio** unattended into its own Proton prefix.
4. **USB permissions** for the scanner (one password prompt).
5. **Camera fix:** builds and installs the patched Wine DLLs (about 10 seconds plus a 33 MB download).
6. **Check:** adds the menu entry and runs `doctor`.

A complete run takes about half a minute plus the download. Run it again at any time to repair or update; finished steps are skipped. `install --yes` answers every question with yes. To remove everything: `jmstudio-linux uninstall`.

## Requirements

- Steam with **Proton Experimental** installed (Library > Tools). Its Steam Linux Runtime comes along automatically.
- A Vulkan capable GPU and recent Mesa/NVIDIA drivers.
- `bash`, `usbutils` (`lsusb`). Optional: `winetricks`, `vulkan-tools`.
- For the camera fix: `clang`, `lld`, `flex`, `bison`, `gcc`, `make`, `patch`, `curl`, `tar`, `xz`. It downloads the 33 MB Wine 11.0 source release once and builds two DLLs in about 10 seconds.
- A free JMMeta account to download the installer.

Other options: any other Proton (for example GE-Proton) via `--proton`, or plain system Wine via `--runner wine`.

## Manual installation

The same steps by hand, if you want control:

```bash
git clone https://github.com/Ibot-11/jmstudio-linux.git
cd jmstudio-linux
```

1. Download **JMStudio > Windows** from <https://forum.jimumeta.com/home/help/download.html> (login required).
2. Install it into a fresh Proton prefix (`--silent` runs the Windows installer unattended and closes the first start that the installer triggers; without it the installer window opens and you click through it, then close JMStudio so `setup` can finish):
   ```bash
   ./jmstudio-linux setup --silent ~/Downloads/JMStudioSetup-*.exe
   ```
3. Give your user access to the scanner (once; a graphical password dialog or `sudo` asks for your password):
   ```bash
   ./jmstudio-linux udev
   ```
4. Install the camera fix (builds two patched Wine DLLs and puts them into the prefix):
   ```bash
   ./jmstudio-linux camerafix
   ```
5. Plug the scanner in and check:
   ```bash
   ./jmstudio-linux doctor
   ```
6. Start it, or use the "JMStudio (3DMakerpro)" entry in your application menu:
   ```bash
   ./jmstudio-linux run
   ```

The first start of JMStudio needs an internet connection and a login (vendor requirement).

## Commands

| Command | What it does |
| --- | --- |
| `install [--yes] [--installer FILE]` | The guided installer described above. |
| `setup [--silent] [INSTALLER.exe]` | Only create the prefix, run the JMStudio installer, find `JMStudio.exe`, add the menu entry. Looks in `~/Downloads` if no path is given. |
| `run [args]` | Start JMStudio (default command). |
| `doctor` | Check Proton, container runtime, installation, camera fix, scanner USB access, GPUs. |
| `udev` | Install `udev/70-3dmakerpro.rules` (asks for your password). |
| `camerafix [build\|install\|remove\|status]` | Build and install, or remove, the patched Wine DLLs (default: install). |
| `reg ...` | Run Windows' `reg.exe` in the prefix. |
| `desktop` | Recreate the application menu entry. |
| `set-exe PATH` | Point the launcher at `JMStudio.exe` if auto-detection fails. |
| `winecfg`, `regedit`, `cmd` | Wine tools inside the prefix. |
| `winetricks VERB...` | Run winetricks against the prefix. |
| `logs` | Print the newest Proton log. |
| `uninstall` | Remove the menu entry and launcher link, and after asking the USB rules, the whole prefix and the build cache. |

Options: `--debug` (write a Proton log; set `WINEDEBUG` yourself for other channels), `--proton NAME|DIR`, `--runner auto|slr|proton|wine`.
Environment: `JMSTUDIO_HOME` (default `~/.local/share/jmstudio-linux`), `JMSTUDIO_PROTON`, `JMSTUDIO_RUNNER`.
Extra variables for JMStudio go into `~/.config/jmstudio-linux/env` as `export NAME=value` lines.

## What the camera fix does and why

The Moose is two UVC cameras behind a USB hub. JMStudio finds it by looking at Windows' camera names and device IDs: it asks Qt for the camera list and derives the scanner's serial number (for example `JMM3004751`) from it, which decides the device plugin; the Moose module then lists the DirectShow cameras again and reads the USB vendor and product ID from each camera's **display name**. Stock Wine breaks this in several places:

1. **DirectShow reports a fake `DevicePath`.** Wine's `devenum.dll` reports every camera as `dummy_video0`. Windows reports `\\?\usb#vid_bcdc&pid_000a&mi_00#...`. [`patches/0001-devenum-usb-device-path.patch`](patches/0001-devenum-usb-device-path.patch) builds that path from sysfs.
2. **The camera's display name has no USB ID.** Wine names the camera `@device:cm:{860BB310-...}\video0`; Windows names it `@device:pnp:\\?\usb#vid_...`, and JMStudio parses `vid_` and `&mi_` out of that. [`patches/0003-devenum-pnp-display-name.patch`](patches/0003-devenum-pnp-display-name.patch) fixes the name (and makes `MkParseDisplayName` accept it).
3. **The camera filter cannot set exposure.** JMStudio dims the scanner's LEDs through the cameras' exposure time (the LEDs are pulsed with it; the default 60 is the maximum, so without this they stay at full brightness). It sets it through DirectShow's `IAMCameraControl`, which Wine's `qcap` does not implement, and Wine only maps four `IAMVideoProcAmp` properties. [`patches/0004-qcap-camera-controls-as-qcapjm.patch`](patches/0004-qcap-camera-controls-as-qcapjm.patch) adds `IAMCameraControl` (exposure with Windows' log2-seconds scale, pan, tilt, zoom, focus, iris) and the missing `IAMVideoProcAmp` properties (gain, sharpness, gamma, white balance, backlight). The Linux half of `qcap` cannot be replaced from outside Proton, so the patched filter is built as a renamed copy, `qcapjm.dll` + `qcapjm.so`: it is found through `WINEDLLPATH` (set by the launcher) and registered for the camera filter's class in the prefix.
4. **Listing the cameras is not safe to do twice at once.** Wine's `devenum` wiped the camera list in the registry and rebuilt it on every enumeration. JMStudio opens both cameras from two threads (and parses display names while it is walking the list), so every now and then one thread saw no camera, and a camera was missing after the scanner had been stopped and started again (second scan: connected, but 0 frames per second). [`patches/0005-devenum-update-video-devices-in-place.patch`](patches/0005-devenum-update-video-devices-in-place.patch) updates the entries in place, removes only cameras that are really gone and serializes the update. With two threads listing 200 times each, stock Wine returned too few cameras in about 12% of the listings; with the fix, never. The sysfs lookup in patch 0001 reads one `modalias` file per camera instead of scanning all USB devices.
5. **Media Foundation lists no cameras.** Qt 6.2's camera list uses `MFEnumDeviceSources`, which Wine answers with "Not implemented for video capture devices". [`patches/0002-mf-enumerate-video-capture-devices.patch`](patches/0002-mf-enumerate-video-capture-devices.patch) enumerates the cameras (through the same DirectShow list). Capture itself is not implemented there; activating a camera returns a source that refuses every request, which Qt handles gracefully.
6. **Stopping a camera can hang for good, and that blocks the other camera.** Wine's camera filter has a thread that waits for the next frame in a loop (`while (read() == EAGAIN)`, one full CPU core per camera), and stopping the graph waits for that thread with no time limit. If the camera does not deliver a frame at that moment (it is not known why this happens now and then), the stop never finishes. JMStudio closes a camera with `VideoCapture::release()` while holding one global lock that every OpenCV call of both cameras goes through, so the hung stop also blocks the next `open()` of the other camera, and the preview shows 0 frames per second until the scanner is unplugged. It showed up after cancelling a scan, deleting the data and starting again, or after going to the edit mode and back. [`patches/0006-qcap-interruptible-frame-wait.patch`](patches/0006-qcap-interruptible-frame-wait.patch) waits in `poll()` for at most 50 ms and lets the stream thread look at the graph state again, so a stop always finishes; as a side effect the two busy-waiting CPU cores are gone. [`tools/opencv-stop.c`](tools/opencv-stop.c) runs open / grab / release cycles for both cameras the way JMStudio does; it cannot make the camera stall, so a real run in JMStudio is the test that counts.

`jmstudio-linux camerafix` builds everything from the signed Wine 11.0 release sources (SHA-256 pinned) with `clang`, and installs it **only into the JMStudio prefix**: the prefix's symlinks into Proton are replaced by the patched files (originals kept as `*.orig`), `qcapjm` is added next to them, and `camerafix remove` puts everything back. Proton itself is not touched. No binaries are shipped, only the patches. After a Proton update or prefix upgrade, run `camerafix` again; `doctor` tells you when it is missing. The same goes for updating this repo: the build remembers which patches it was made from, so after a `git pull` (or running `install.sh` again) `./jmstudio-linux install` or `camerafix` rebuilds and reinstalls when a patch changed, and `doctor` marks an out-of-date build as `FAIL`. Close JMStudio before you do.

## Not verified yet

- **The Moose's projector.** Whether JMStudio can drive it through Wine is unknown.
- **Why a camera sometimes stops delivering frames** when the preview is stopped (item 6). The hang it caused is fixed; the stall itself was never reproduced outside JMStudio.
- **Other 3DMakerpro scanners.** Only a Moose (`bcdc:000a`, `bcdc:000b`) has been tried. JMStudio's device plugins for other models read their cameras differently.
- **USB 3 vs. USB 2.** The Moose here is connected through a USB 2.0 hub at 480 Mbit/s and connects fine; JMStudio contains a "USB port is not Super-speed" message I have not seen.

JMStudio's own log, useful for any report, is in `~/.local/share/jmstudio-linux/prefix/pfx/drive_c/users/steamuser/AppData/Roaming/Jimuyida/JMStudio/log/`. A healthy start looks like `Current device is Moose`, `ICT CameraA index:0 ...`, `Connect scanner successfully...`.

## Frame rate

The Moose's cameras deliver **10 fps**: every mode they advertise (MJPEG 640x480, 1280x720, 1280x1024) has a 0.1 s frame interval (`v4l2-ctl -d /dev/video0 --list-formats-ext`). Natively, `v4l2-ctl` measures 10.00 fps per camera, also with both streaming at once, and asking for 15 fps is answered with 10. About 10-11 FPS in JMStudio is therefore the scanner's limit, not Wine's.

## Troubleshooting

**JMStudio does not see the scanner** (banner "Es besteht keine normale Verbindung", or `Connect scanner failed!` in its log)
1. `./jmstudio-linux doctor` - fix everything it marks `FAIL`; the camera fix must be installed.
2. Try another USB port, ideally USB 3 and without a hub.
3. `./jmstudio-linux --debug run`, connect the scanner, close the program, then inspect the log: `grep -iE 'devenum|mfplat|err:' "$(./jmstudio-linux logs)" | less`. For more channels: `WINEDEBUG='+timestamp,+devenum,+mfplat,err+all' ./jmstudio-linux --debug run`.
4. Try another runner: `--proton GE-Proton11-7-x86_64` or `--runner wine` (the camera fix is built against Wine 11.0 and is only tested with Proton Experimental).

**The preview shows 0 frames per second after a scan was cancelled or the mode was switched, and the scanner has to be unplugged and plugged in again** - that is the hang from item 6 above, fixed by patch 0006. `./jmstudio-linux doctor` tells you whether your camera fix includes it (otherwise: close JMStudio, run `./jmstudio-linux camerafix`).

**JMStudio hangs at start at 100% CPU, or the Proton log ends in `double free or corruption`** - this is a crash in Proton's own graphics start-up (DXVK/Vulkan), before JMStudio touches a camera. It also happens without the camera fix (2 of 30 starts in a test; 5 of 30 with it, too few runs to tell the difference). Close the hung process (`pkill -x JMStudio.exe`) and start again.

**Installer fails or the program crashes at start** - install the Visual C++ runtime with `./jmstudio-linux winetricks vcrun2022` and try again.

**Wrong scale on a HiDPI screen** - `./jmstudio-linux winecfg`, tab Graphics, adjust the DPI.

**Other 3DMakerpro scanners** (Moose Lite, Fox, ...) - they may use other USB IDs. Plug the scanner in, find its ID in `lsusb`, add a line to `udev/70-3dmakerpro.rules` and the ID to `KNOWN_IDS` in the script, then `./jmstudio-linux udev`. A pull request with the ID is very welcome.

**Start over** - `./jmstudio-linux uninstall`.

## Reporting results

Open an issue and paste the output of `./jmstudio-linux doctor`, your distro, Proton version, your scanner's `lsusb` IDs, and JMStudio's log from above. Reports that something *works* are just as useful.

## Legal

The scripts are MIT licensed (see `LICENSE`). The files in `patches/` are changes to Wine and are licensed under the GNU LGPL 2.1 or later like Wine itself. JMStudio and all related names belong to their owners; this repo neither contains nor redistributes any of their software.

---

## Deutsch

Mit diesem Repo läuft **JMStudio**, die Windows-Software für den **3DMakerpro Moose** 3D-Scanner, unter Linux über **Proton**. Inoffiziell, ohne Verbindung zu 3DMakerpro oder JMMeta. Die Software selbst ist **nicht enthalten**: Den Installer lädst du selbst herunter, den Rest richtet das Skript ein.

**Stand: funktioniert, aber experimentell.** JMStudio installiert sich, startet, **verbindet sich mit dem Moose**, zeigt das Livebild und nimmt Scans auf und verarbeitet sie. Getestet von einer Person mit einem Moose.

**Warum der Kamera-Fix nötig ist:** Wine meldet für jede Kamera einen Platzhalter-Pfad (`dummy_video0`), einen Anzeigenamen ohne USB-Kennung und kann Videokameras nicht über Media Foundation auflisten. JMStudio ermittelt Seriennummer und Kamera-IDs aber genau daraus. Sechs kleine Wine-Patches (`patches/`) beheben das und sorgen außerdem dafür, dass JMStudio die LED-Helligkeit einstellen kann und das Beenden einer Kamera nicht mehr hängen bleibt (sonst steht das Bild nach einem abgebrochenen Scan auf 0 FPS, bis man den Scanner abzieht). Sie werden aus den signierten Wine-11.0-Quellen gebaut und nur in das JMStudio-Prefix installiert, Proton bleibt unverändert (`./jmstudio-linux camerafix remove` macht alles rückgängig).

**Voraussetzungen:** Steam mit *Proton Experimental*, Vulkan-fähige GPU, `usbutils`, für den Kamera-Fix `clang`, `lld`, `flex`, `bison`, `gcc`, `make`, `patch`, `curl`; ein kostenloses JMMeta-Konto für den Download.

**Installation in einem Schritt**

```bash
curl -fsSL https://raw.githubusercontent.com/Ibot-11/jmstudio-linux/main/install.sh | bash
```

Der Installer prüft Steam, Proton Experimental und die Build-Werkzeuge, öffnet die JMStudio-Download-Seite (der Download braucht ein kostenloses JMMeta-Konto und lässt sich nicht automatisieren) und wartet, bis die Datei in `~/Downloads` liegt. Dann installiert er JMStudio, setzt die USB-Rechte (ein Passwort-Dialog), baut den Kamera-Fix, legt den Menüeintrag an und führt `doctor` aus. Vor jedem Schritt, der dein System ändert, fragt er nach. Entfernen: `jmstudio-linux uninstall`. Aktualisieren: Repo neu laden (`git pull` oder den Installer erneut ausführen) und `./jmstudio-linux install` starten, nachdem JMStudio geschlossen ist. Der Installer baut den Kamera-Fix neu, wenn sich ein Patch geändert hat, und `doctor` warnt vor einem veralteten Stand.

**Schritt für Schritt (manuell)**

1. *JMStudio > Windows* von <https://forum.jimumeta.com/home/help/download.html> laden (Login nötig).
2. Installieren: `./jmstudio-linux setup --silent ~/Downloads/JMStudioSetup-*.exe`
3. Einmalig USB-Rechte setzen (Passwort-Dialog): `./jmstudio-linux udev`
4. Kamera-Fix installieren: `./jmstudio-linux camerafix`
5. Scanner anstecken und prüfen: `./jmstudio-linux doctor`
6. Starten: `./jmstudio-linux run` oder über den Menüeintrag „JMStudio (3DMakerpro)“.

**Dateien speichern:** Wines Laufwerk `Z:` ist dein ganzes Dateisystem, Scans also unter `Z:\home\<name>\` ablegen.
