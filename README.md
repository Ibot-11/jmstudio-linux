# jmstudio-linux

Run **JMStudio**, the Windows software for the **3DMakerpro Moose** 3D scanner (and its siblings), on Linux through **Proton**.

> Deutsche Anleitung: [weiter unten](#deutsch).

This is an unofficial helper. It is not affiliated with 3DMakerpro or JMMeta. JMStudio is proprietary and **not included**: you download the installer yourself and this repo sets everything else up around it.

## Status

| Part | State |
| --- | --- |
| Prefix creation with Proton Experimental inside the Steam Linux Runtime | tested (CachyOS, Proton Experimental 11.0, Wine 11.0) |
| `setup`, exe detection, menu entry, `run`, `--debug` logs, `winetricks` | tested with stand-in programs |
| Proton's USB (`wineusb`) and camera (`qcap`) stack | present and enumerating host USB devices |
| **Real JMStudio installer and the Moose scanner** | **not verified yet.** Please report what you see (see [Reporting](#reporting-results)) |

The Moose does not need a kernel driver: it shows up as two UVC cameras (`0c45:636a` and `0c45:636b`, "KYT Camera A/B") behind a small USB hub. A [forum thread](https://forum.endeavouros.com/t/3d-scanner-sorftware-for-3dmakerpro-moose-3d-scanner/77272) reports that the Windows software did not detect the scanner on Linux (it is not clear how it was run there). Missing USB permissions are the first thing to rule out, and that is what the udev rules here address.

## Requirements

- Steam with **Proton Experimental** installed (Library > Tools). Its Steam Linux Runtime comes along automatically.
- A Vulkan capable GPU and recent Mesa/NVIDIA drivers.
- `bash`, `usbutils` (`lsusb`). Optional: `winetricks`, `vulkan-tools`.
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
   The installer window opens; click through it as on Windows. `--silent` tries an unattended install.
3. Give your user access to the scanner (once, asks for sudo):
   ```bash
   ./jmstudio-linux udev
   ```
4. Plug the scanner in (directly into a USB 2.0 port if you can) and check:
   ```bash
   ./jmstudio-linux doctor
   ```
5. Start it, or use the new "JMStudio (3DMakerpro)" entry in your application menu:
   ```bash
   ./jmstudio-linux run
   ```

The first start of JMStudio needs an internet connection (vendor requirement).

## Commands

| Command | What it does |
| --- | --- |
| `setup [INSTALLER.exe]` | Create the prefix, run the installer, find `JMStudio.exe`, add the menu entry. Looks in `~/Downloads` if no path is given. |
| `run [args]` | Start JMStudio (default command). |
| `doctor` | Check Proton, container runtime, installation, scanner USB access, GPUs. |
| `udev` | Install `udev/70-3dmakerpro.rules` (uses `sudo`). |
| `desktop` | Recreate the application menu entry. |
| `set-exe PATH` | Point the launcher at `JMStudio.exe` if auto-detection fails. |
| `winecfg`, `regedit`, `cmd` | Wine tools inside the prefix. |
| `winetricks VERB...` | Run winetricks against the prefix, for example `vcrun2022`. |
| `logs` | Print the newest Proton log. |
| `uninstall` | Remove the menu entry and, after asking, the whole prefix. |

Options: `--debug` (write a log with USB/camera tracing), `--proton NAME|DIR`, `--runner auto|slr|proton|wine`.
Environment: `JMSTUDIO_HOME` (default `~/.local/share/jmstudio-linux`), `JMSTUDIO_PROTON`, `JMSTUDIO_RUNNER`.
Extra variables for JMStudio go into `~/.config/jmstudio-linux/env` as `export NAME=value` lines.

## How it works

- A dedicated Proton prefix lives in `~/.local/share/jmstudio-linux/prefix`, so nothing touches your Steam games.
- Proton runs inside the Steam Linux Runtime container, exactly like Steam does it, but without the Steam client in the loop.
- Wine reaches the scanner over libusb (`wineusb`) and its cameras over V4L2 (`qcap`). Both need access to `/dev/bus/usb`, which the udev rules grant to the logged-in user via the `uaccess` tag.
- Files: Wine's `Z:` drive is your whole filesystem, so save scans anywhere under `Z:\home\<you>\`.

## Troubleshooting

**JMStudio does not see the scanner**
1. `./jmstudio-linux doctor` - fix everything it marks `FAIL`, then replug the scanner.
2. Try another USB port, ideally USB 2.0 and without a hub.
3. Run `./jmstudio-linux --debug run`, connect the scanner, close the program and look at what Wine saw:
   ```bash
   grep -iE 'wineusb|qcap|dshow|mfplat|err:' "$(./jmstudio-linux logs)" | less
   ```
4. Try another runner: `--proton GE-Proton11-7-x86_64` or `--runner wine`.

**Installer fails or the program crashes at start** - install the Visual C++ runtime with `./jmstudio-linux winetricks vcrun2022` and try again.

**Wrong scale on a HiDPI screen** - `./jmstudio-linux winecfg`, tab Graphics, adjust the DPI.

**Other 3DMakerpro scanners** (Moose Lite, Fox, ...) - they may use other USB IDs. Plug the scanner in, find its ID in `lsusb`, add a line to `udev/70-3dmakerpro.rules` and the ID to `KNOWN_IDS` in the script, then `./jmstudio-linux udev`. A pull request with the ID is very welcome.

**Start over** - `./jmstudio-linux uninstall`.

## Reporting results

Open an issue and paste the output of `./jmstudio-linux doctor`, your distro, Proton version, and (if the scanner was not found) the filtered log from above. Reports that it *does* work are just as useful.

## Legal

The scripts are MIT licensed. JMStudio and all related names belong to their owners; this repo neither contains nor redistributes any of their software.

---

## Deutsch

Mit diesem Repo läuft **JMStudio**, die Windows-Software für den **3DMakerpro Moose** 3D-Scanner, unter Linux über **Proton**. Inoffiziell, ohne Verbindung zu 3DMakerpro oder JMMeta. Die Software selbst ist **nicht enthalten**: Den Installer lädst du selbst herunter, den Rest richtet das Skript ein.

**Stand:** Prefix-Erstellung mit Proton Experimental, Setup, Start, Debug-Logs und winetricks sind mit Stand-in-Programmen getestet. Mit dem **echten JMStudio-Installer und dem Moose-Scanner** ist es noch **nicht überprüft**, Rückmeldungen sind willkommen.

**Voraussetzungen:** Steam mit installiertem *Proton Experimental*, Vulkan-fähige GPU, `usbutils`, ein kostenloses JMMeta-Konto für den Download.

**Schnellstart**

```bash
git clone https://github.com/Ibot-11/jmstudio-linux.git
cd jmstudio-linux
```

1. *JMStudio > Windows* von <https://forum.jimumeta.com/home/help/download.html> laden (Login nötig).
2. Installieren: `./jmstudio-linux setup ~/Downloads/JMStudioSetup-*.exe`
3. Einmalig USB-Rechte für den Scanner setzen (fragt nach sudo): `./jmstudio-linux udev`
4. Scanner anstecken (am besten direkt an einen USB-2.0-Port) und prüfen: `./jmstudio-linux doctor`
5. Starten: `./jmstudio-linux run` oder über den Menüeintrag "JMStudio (3DMakerpro)".

Der erste Start von JMStudio braucht eine Internetverbindung.

**Scanner wird nicht erkannt?** `doctor` ausführen und alle `FAIL`-Zeilen beheben, Scanner neu einstecken, anderen USB-Port testen. Mit `./jmstudio-linux --debug run` entsteht ein Log mit USB- und Kamera-Tracing (`./jmstudio-linux logs` zeigt den Pfad). Alternativ eine andere Proton-Version (`--proton GE-Proton11-7-x86_64`) oder `--runner wine` probieren.

**Dateien speichern:** Wines Laufwerk `Z:` ist dein ganzes Dateisystem, Scans also unter `Z:\home\<name>\` ablegen.
