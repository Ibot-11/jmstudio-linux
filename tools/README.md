# Diagnostic programs

Small Windows programs that run in the JMStudio prefix and show what Wine reports to an
application. They are what found the problems the camera fix addresses. They are sources
only; build them with `winegcc` from the Wine tree that `jmstudio-linux camerafix build`
leaves in `~/.cache/jmstudio-linux/wine-build/build`:

```bash
cd ~/.cache/jmstudio-linux/wine-build/build
tools/winegcc/winegcc -b x86_64-windows -I../wine-11.0/include -I../wine-11.0/include/msvcrt \
    --wine-objdir . -o /tmp/dshow-cameras.exe /path/to/dshow-cameras.c \
    -lole32 -loleaut32 -lstrmiids -luuid -lucrtbase -lkernel32
```

| Program | Shows |
| --- | --- |
| `dshow-cameras.c` | DirectShow video devices with `FriendlyName`, `DevicePath`, `Description` (what JMStudio's DirectShow code reads) |
| `mf-cameras.c` | `MFEnumDeviceSources` for video capture and what happens when a device is activated (what Qt's camera list does). Also link `-lmf -lmfplat -lmfreadwrite -lmfuuid` |
| `devenum-race.c` | Several threads list the video capture devices at the same time and count how often one of them sees too few cameras (`devenum-race.exe THREADS ITERATIONS EXPECTED`). Shows the race fixed by patch 0005 |
| `opencv-properties.c` | Opens camera 0 through JMStudio's OpenCV (DirectShow) and calls `VideoCapture::set/get` for brightness, contrast, gain, exposure, zoom, pan and so on, the way JMStudio sets camera properties. Without the camera fix everything except brightness, contrast, saturation and hue fails; with an extra two arguments (`<propId> <value>`) it sets one property, e.g. `15 -13` for the shortest exposure |
| `opencv-capture.c` | Loads JMStudio's own `opencv_videoio3416.dll` and tries `cv::VideoCapture::open(index, backend)` plus `grab()` for DirectShow, Media Foundation and "any" |

Run them through the launcher so Proton's environment is used, for example
`jmstudio-linux cmd /c Z:\path\to\opencv-capture.exe Z:\path\to\JMStudio\2.6.17.0237 -1 Z:\path\to\out.txt`
(`opencv-capture` writes to the file named by its third argument because Proton does not
forward standard output). The steam container does not share `/tmp`, so keep the files under
your home directory.
