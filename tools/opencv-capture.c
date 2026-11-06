/* Drives JMStudio's own OpenCV 3.4.16 videoio DLL: cv::VideoCapture::open(index, backend). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

typedef void *(*ctor_t)(void *self);
typedef unsigned char (*open_t)(void *self, int index, int api);
typedef unsigned char (*bool0_t)(void *self);
typedef void (*dtor_t)(void *self);

int main(int argc, char **argv)
{
    const char *dir = argc > 1 ? argv[1] : ".";
    char path[MAX_PATH];
    HMODULE mod;
    ctor_t ctor;
    open_t open_;
    bool0_t is_opened, grab;
    int apis[] = { 700, 1400, 0 }; /* CAP_DSHOW, CAP_MSMF, CAP_ANY */
    const char *names[] = { "CAP_DSHOW", "CAP_MSMF", "CAP_ANY" };
    int i, idx, which = argc > 2 ? atoi(argv[2]) : -1;

    if (argc > 3) freopen(argv[3], "w", stdout);
    SetCurrentDirectoryA(dir);
    SetDllDirectoryA(dir);
    snprintf(path, sizeof(path), "%s\\opencv_videoio3416.dll", dir);
    if (!(mod = LoadLibraryA(path))) { printf("LoadLibrary failed: %lu\n", GetLastError()); return 1; }

    ctor = (ctor_t)GetProcAddress(mod, "??0VideoCapture@cv@@QEAA@XZ");
    open_ = (open_t)GetProcAddress(mod, "?open@VideoCapture@cv@@QEAA_NHH@Z");
    is_opened = (bool0_t)GetProcAddress(mod, "?isOpened@VideoCapture@cv@@UEBA_NXZ");
    grab = (bool0_t)GetProcAddress(mod, "?grab@VideoCapture@cv@@UEAA_NXZ");
    printf("symbols: ctor=%p open=%p isOpened=%p grab=%p\n", ctor, open_, is_opened, grab);
    if (!ctor || !open_ || !is_opened || !grab) return 2;

    for (i = 0; i < 3; i++)
    {
        if (which >= 0 && which != i) continue;
        for (idx = 0; idx < 4; idx++)
        {
            void *cap = calloc(1, 512);
            unsigned char ok, opened, grabbed = 0;
            ctor(cap);
            ok = open_(cap, idx, apis[i]);
            opened = is_opened(cap);
            if (opened) grabbed = grab(cap);
            printf("%-10s index %d: open=%d isOpened=%d grab=%d\n", names[i], idx, ok, opened, grabbed);
            fflush(stdout);
            /* the object is leaked on purpose: closing real cameras is not what this tests */
        }
    }
    return 0;
}
