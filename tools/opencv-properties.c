/* Opens camera 0 through JMStudio's OpenCV (DirectShow backend) and sets camera properties,
 * the way JMStudio's Moose module does: VideoCapture::set(propId, value). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

int _fltused;
typedef void *(*ctor_t)(void *self);
typedef unsigned char (*open_t)(void *self, int index, int api);
typedef unsigned char (*bool0_t)(void *self);
typedef unsigned char (*set_t)(void *self, int prop, double value);
typedef double (*get_t)(void *self, int prop);

static const struct { int id; const char *name; } props[] = {
    { 10, "BRIGHTNESS" }, { 11, "CONTRAST" }, { 12, "SATURATION" }, { 13, "HUE" },
    { 14, "GAIN" }, { 15, "EXPOSURE" }, { 20, "SHARPNESS" }, { 22, "GAMMA" },
    { 27, "ZOOM" }, { 28, "FOCUS" }, { 32, "BACKLIGHT" }, { 33, "PAN" }, { 34, "TILT" }, { 35, "ROLL" },
};

int main(int argc, char **argv)
{
    const char *dir = argv[1];
    char path[MAX_PATH];
    HMODULE mod;
    void *cap = calloc(1, 512);
    ctor_t ctor;
    open_t open_;
    bool0_t is_opened;
    set_t set_;
    get_t get_;
    unsigned int i;

    if (argc > 2) freopen(argv[2], "w", stdout);
    SetCurrentDirectoryA(dir);
    SetDllDirectoryA(dir);
    snprintf(path, sizeof(path), "%s\\opencv_videoio3416.dll", dir);
    if (!(mod = LoadLibraryA(path))) { printf("LoadLibrary failed: %lu\n", GetLastError()); return 1; }

    ctor = (ctor_t)GetProcAddress(mod, "??0VideoCapture@cv@@QEAA@XZ");
    open_ = (open_t)GetProcAddress(mod, "?open@VideoCapture@cv@@QEAA_NHH@Z");
    is_opened = (bool0_t)GetProcAddress(mod, "?isOpened@VideoCapture@cv@@UEBA_NXZ");
    set_ = (set_t)GetProcAddress(mod, "?set@VideoCapture@cv@@UEAA_NHN@Z");
    get_ = (get_t)GetProcAddress(mod, "?get@VideoCapture@cv@@UEBANH@Z");
    printf("symbols: ctor=%p open=%p isOpened=%p set=%p get=%p\n", ctor, open_, is_opened, set_, get_);
    if (!ctor || !open_ || !is_opened || !set_ || !get_) return 2;

    ctor(cap);
    printf("open(0, CAP_DSHOW): %d, isOpened: %d\n", open_(cap, 0, 700), is_opened(cap));
    fflush(stdout);

    if (argc > 4)
    {
        int id = atoi(argv[3]);
        double value = atof(argv[4]);
        unsigned char ok = set_(cap, id, value);
        printf("set(%d, %g) -> ok=%d, get -> %g\n", id, value, ok, get_(cap, id));
        return 0;
    }

    for (i = 0; i < sizeof(props) / sizeof(props[0]); i++)
    {
        double before, after;
        unsigned char ok;

        printf("%-10s (%2d): get...", props[i].name, props[i].id); fflush(stdout);
        before = get_(cap, props[i].id);
        printf(" %g, set(%g)...", before, before); fflush(stdout);
        ok = set_(cap, props[i].id, before);
        printf(" ok=%d", ok); fflush(stdout);
        after = get_(cap, props[i].id);
        printf(", now %g\n", after); fflush(stdout);
    }
    printf("done, no crash\n");
    return 0;
}
