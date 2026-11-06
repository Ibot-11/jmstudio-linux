# jmstudio-linux

Run **JMStudio**, the Windows software for the **3DMakerpro Moose** 3D scanner (and its siblings), on Linux through **Proton**.

> Deutsche Kurzfassung: [weiter unten](#deutsch).

This is an unofficial helper. It is not affiliated with 3DMakerpro or JMMeta. JMStudio is proprietary and **not included**: you download the installer yourself and this repo sets everything else up around it.

## Status: work in progress

JMStudio installs and starts, and with the camera fix it recognises the scanner. **Scanning does not work yet:** JMStudio still reports "Connect scanner failed!" and I have not found out why. Details below, help is welcome.

| Part | State |
| --- | --- |
| Proton Experimental prefix, `setup`, `run`, logs, menu entry | works (CachyOS, Proton Experimental 11.0 / Wine 11.0, JMStudio 2.6.17) |
| USB access (`udev` rules) | works, tested with a Moose (`bcdc:000a`, `bcdc:000b`) |
| **Camera fix** (patched Wine `devenum.dll` and `mf.dll`) | JMStudio then finds the scanner's serial number, switches from the default "CR-Scan Lizard" to **Moose** and loads `moosePlugin.dll` |
| Camera capture | works in a test program using JMStudio's own OpenCV: both cameras open through DirectShow and deliver frames |
| **Connecting to the scanner in JMStudio** | **fails.** `Create Moose_ICT_130W Module`, `Searching camera list...`, then `Connect scanner failed!` every 3 s, without trying to open a camera |

Without the camera fix JMStudio never identifies the scanner at all, so it is a required step.

## Requirements

- Steam with **Proton Experimental** installed (Library > Tools). Its Steam Linux Runtime comes along automatically.
- A Vulkan capable GPU and recent Mesa/NVIDIA drivers.
- `bash`, `usbutils` (`lsusb`). Optional: `winetricks`, `vulkan-tools`.
- For the camera fix: `clang`, `lld`, `flex`, `bison`, `gcc`, `make`, `patch`, `curl`, `tar`, `xz`. It downloads the 33 MB Wine 11.0 source release once and builds two DLLs in about 10 seconds.
- A free JMMeta account to download the installer.

Other options: any other Proton (for example GE-Proton) via `--proton`, or plain system Wine via `--runner wine`.

## Quick start

```bash
git clone https://github.com/Ibot-11/jmstudio-linux.git
cd jmstudio-linux
```

1. Download **JMStudio > Windows** from <https://forum.jimumeta.com/home/help/download.html> (login required).
2. Install it into a fresh Proton prefix:
   ```bash
   ./jmstudio-linux setup ~/Downloads/JMStudioSetup-*.exe
   ```
   The installer window opens; click through it as on Windows. JMStudio starts at the end; close it so `setup` can finish.
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
| `setup [INSTALLER.exe]` | Create the prefix, run the installer, find `JMStudio.exe`, add the menu entry. Looks in `~/Downloads` if no path is given. |
| `run [args]` | Start JMStudio (default command). |
| `doctor` | Check Proton, container runtime, installation, camera fix, scanner USB access, GPUs. |
| `udev` | Install `udev/70-3dmakerpro.rules` (asks for your password). |
| `camerafix [build\|install\|remove\|status]` | Build and install, or remove, the patched Wine DLLs (default: install). |
| `desktop` | Recreate the application menu entry. |
| `set-exe PATH` | Point the launcher at `JMStudio.exe` if auto-detection fails. |
| `winecfg`, `regedit`, `cmd` | Wine tools inside the prefix. |
| `winetricks VERB...` | Run winetricks against the prefix. |
| `logs` | Print the newest Proton log. |
| `uninstall` | Remove the menu entry and, after asking, the whole prefix. |

Options: `--debug` (write a Proton log; set `WINEDEBUG` yourself for other channels), `--proton NAME|DIR`, `--runner auto|slr|proton|wine`.
Environment: `JMSTUDIO_HOME` (default `~/.local/share/jmstudio-linux`), `JMSTUDIO_PROTON`, `JMSTUDIO_RUNNER`.
Extra variables for JMStudio go into `~/.config/jmstudio-linux/env` as `export NAME=value` lines.

## What the camera fix does and why

The Moose is two UVC cameras behind a USB hub. JMStudio finds it like this: it asks Qt for the list of cameras, reads each camera's name and ID, and derives the scanner's serial number (for example `JMM3004751`) from them. The serial number decides which device plugin is loaded. Stock Wine breaks this in two places:

1. **DirectShow reports a fake `DevicePath`.** Wine's `devenum.dll` reports every camera as `dummy_video0`. Windows reports `\\?\usb#vid_bcdc&pid_000a&mi_00#...`, and JMStudio reads the vendor and product ID from it. [`patches/0001-devenum-usb-device-path.patch`](patches/0001-devenum-usb-device-path.patch) builds that path from sysfs.
2. **Media Foundation lists no cameras.** Qt 6.2's camera list uses `MFEnumDeviceSources`, which Wine answers with "Not implemented for video capture devices". [`patches/0002-mf-enumerate-video-capture-devices.patch`](patches/0002-mf-enumerate-video-capture-devices.patch) enumerates the cameras (through the same DirectShow list). Capture itself is not implemented there; activating a camera returns a source that refuses every request, which Qt handles gracefully.

`jmstudio-linux camerafix` builds both DLLs from the signed Wine 11.0 release sources (SHA-256 pinned) with `clang`, and installs them **only into the JMStudio prefix**: the prefix's symlinks into Proton are replaced by the patched files, the originals are kept as `*.orig`, and `camerafix remove` puts everything back. Proton itself is not touched. No binaries are shipped, only the patches. After a Proton update or prefix upgrade, run `camerafix` again; `doctor` tells you when it is missing.

## Open problem: "Connect scanner failed!"

What I know, in case you want to dig in:

- JMStudio's own log is in `~/.local/share/jmstudio-linux/prefix/pfx/drive_c/users/steamuser/AppData/Roaming/Jimuyida/JMStudio/log/`. With the camera fix it shows `Current device is Moose`, the serial number, `Loaded successfully: ...moosePlugin.dll`, then the failing connect.
- The Moose module (`Moose_ICT_130W` in `JMDeviceCore.dll`) opens its cameras through OpenCV 3.4.16 (`cv::VideoCapture::open`). In a test program ([`tools/opencv-capture.c`](tools/opencv-capture.c)) that same OpenCV DLL opens both cameras through DirectShow and `grab()` succeeds, so the camera pipeline works under Proton. The Media Foundation backend fails, as expected.
- In JMStudio the failure happens about 20 ms after "Searching camera list...", before any camera is opened, so the condition that fails is somewhere before the OpenCV call. I have not found it.
- JMStudio also contains USB speed checks ("USB port is not Super-speed"), and the Moose here sits behind a USB 2.0 hub at 480 Mbit/s. I have not checked whether that matters.

If you can read x64 disassembly or have the scanner connected to a USB 3 port, reports are very welcome.

## Troubleshooting

**JMStudio does not see the scanner**
1. `./jmstudio-linux doctor` - fix everything it marks `FAIL`.
2. Try another USB port, ideally USB 3 and without a hub.
3. `./jmstudio-linux --debug run`, connect the scanner, close the program, then inspect the log: `grep -iE 'devenum|mfplat|err:' "$(./jmstudio-linux logs)" | less`. For more channels: `WINEDEBUG='+timestamp,+devenum,+mfplat,err+all' ./jmstudio-linux --debug run`.
4. Try another runner: `--proton GE-Proton11-7-x86_64` or `--runner wine` (the camera fix is built against Wine 11.0 and is only tested with Proton Experimental).

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

**Stand: in Arbeit.** JMStudio installiert sich und startet. Mit dem Kamera-Fix erkennt es den Scanner (Seriennummer, Gerätemodell **Moose**, Moose-Plugin). **Scannen geht noch nicht:** JMStudio meldet weiterhin „Connect scanner failed!“, die Ursache ist offen. Die Kameras selbst liefern in einem Testprogramm mit JMStudios eigenem OpenCV Bilder.

**Warum der Kamera-Fix nötig ist:** Wine meldet für jede Kamera einen Platzhalter-Pfad (`dummy_video0`) und kann Videokameras nicht über Media Foundation auflisten. JMStudio ermittelt die Seriennummer aber genau darüber. Zwei kleine Wine-Patches (`patches/`) beheben das. Sie werden aus den signierten Wine-11.0-Quellen gebaut und nur in das JMStudio-Prefix installiert, Proton bleibt unverändert (`./jmstudio-linux camerafix remove` macht alles rückgängig).

**Voraussetzungen:** Steam mit *Proton Experimental*, Vulkan-fähige GPU, `usbutils`, für den Kamera-Fix `clang`, `lld`, `flex`, `bison`, `gcc`, `make`, `patch`, `curl`; ein kostenloses JMMeta-Konto für den Download.

**Schnellstart**

```bash
git clone https://github.com/Ibot-11/jmstudio-linux.git
cd jmstudio-linux
```

1. *JMStudio > Windows* von <https://forum.jimumeta.com/home/help/download.html> laden (Login nötig).
2. Installieren: `./jmstudio-linux setup ~/Downloads/JMStudioSetup-*.exe` (am Ende JMStudio schließen, damit `setup` fertig wird)
3. Einmalig USB-Rechte setzen (Passwort-Dialog): `./jmstudio-linux udev`
4. Kamera-Fix installieren: `./jmstudio-linux camerafix`
5. Scanner anstecken und prüfen: `./jmstudio-linux doctor`
6. Starten: `./jmstudio-linux run` oder über den Menüeintrag „JMStudio (3DMakerpro)“.

**Dateien speichern:** Wines Laufwerk `Z:` ist dein ganzes Dateisystem, Scans also unter `Z:\home\<name>\` ablegen.
