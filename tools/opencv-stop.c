/* Start/stop stress for the camera filter, the way JMStudio drives it: one thread per camera opens the
 * camera through JMStudio's own OpenCV 3.4.16 (DirectShow), grabs a few frames and releases it again,
 * several times. Like JMStudio, open() and release() run under one global lock. Reports the time of each
 * step and gives up with "HUNG" when a step takes longer than the limit (a release() that waits for a
 * camera that stopped sending frames shows up this way).
 *
 *   opencv-stop.exe JMSTUDIO_DIR CYCLES OUTFILE [MAX_FRAMES] [HANG_SECONDS] */
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

typedef void *(*ctor_t)(void *self);
typedef unsigned char (*open_t)(void *self, int index, int api);
typedef unsigned char (*bool0_t)(void *self);
typedef void (*void0_t)(void *self);

static ctor_t cv_ctor;
static open_t cv_open;
static bool0_t cv_is_opened, cv_grab;
static void0_t cv_release;
static CRITICAL_SECTION lock;
static FILE *out;
static int cycles, max_frames = 8, hang_seconds = 20;

struct job
{
    int index;
    volatile const char *phase;
    volatile ULONGLONG phase_start;
    volatile LONG done;
};

static void set_phase(struct job *j, const char *name)
{
    j->phase = name;
    j->phase_start = GetTickCount64();
}

static DWORD WINAPI camera_thread(void *arg)
{
    struct job *j = arg;
    int c;

    srand(GetTickCount() + j->index * 7919);
    for (c = 0; c < cycles; c++)
    {
        void *cap = calloc(1, 512);
        int frames = rand() % (max_frames + 1), f, got = 0;
        ULONGLONG t0, t1, t2, t3;
        unsigned char ok;

        cv_ctor(cap);
        t0 = GetTickCount64();
        set_phase(j, "waiting for the lock to open");
        EnterCriticalSection(&lock);
        set_phase(j, "open");
        ok = cv_open(cap, j->index, 700 /* CAP_DSHOW */) && cv_is_opened(cap);
        LeaveCriticalSection(&lock);
        t1 = GetTickCount64();
        set_phase(j, "grab");
        if (ok) for (f = 0; f < frames; f++) got += cv_grab(cap) ? 1 : 0;
        t2 = GetTickCount64();
        set_phase(j, "waiting for the lock to release");
        EnterCriticalSection(&lock);
        set_phase(j, "release");
        cv_release(cap);
        LeaveCriticalSection(&lock);
        t3 = GetTickCount64();
        fprintf(out, "camera %d cycle %2d: open %s %4llu ms, %d/%d frames %4llu ms, release %4llu ms\n",
                j->index, c, ok ? "ok  " : "FAIL", t1 - t0, got, frames, t2 - t1, t3 - t2);
        fflush(out);
        set_phase(j, "pause");
        Sleep(rand() % 400);
    }
    InterlockedExchange(&j->done, 1);
    return 0;
}

int main(int argc, char **argv)
{
    char path[MAX_PATH];
    struct job jobs[2] = { { 0 }, { 1 } };
    HANDLE threads[2];
    HMODULE mod;
    int i;

    if (argc < 4) { fprintf(stderr, "usage: opencv-stop JMSTUDIO_DIR CYCLES OUTFILE [MAX_FRAMES] [HANG_SECONDS]\n"); return 1; }
    cycles = atoi(argv[2]);
    if (!(out = fopen(argv[3], "w"))) return 1;
    if (argc > 4) max_frames = atoi(argv[4]);
    if (argc > 5) hang_seconds = atoi(argv[5]);

    SetCurrentDirectoryA(argv[1]);
    SetDllDirectoryA(argv[1]);
    snprintf(path, sizeof(path), "%s\\opencv_videoio3416.dll", argv[1]);
    if (!(mod = LoadLibraryA(path))) { fprintf(out, "LoadLibrary failed: %lu\n", GetLastError()); return 1; }
    cv_ctor = (ctor_t)GetProcAddress(mod, "??0VideoCapture@cv@@QEAA@XZ");
    cv_open = (open_t)GetProcAddress(mod, "?open@VideoCapture@cv@@QEAA_NHH@Z");
    cv_is_opened = (bool0_t)GetProcAddress(mod, "?isOpened@VideoCapture@cv@@UEBA_NXZ");
    cv_grab = (bool0_t)GetProcAddress(mod, "?grab@VideoCapture@cv@@UEAA_NXZ");
    cv_release = (void0_t)GetProcAddress(mod, "?release@VideoCapture@cv@@UEAAXXZ");
    if (!cv_ctor || !cv_open || !cv_is_opened || !cv_grab || !cv_release) { fprintf(out, "missing symbols\n"); return 2; }

    InitializeCriticalSection(&lock);
    for (i = 0; i < 2; i++)
    {
        set_phase(&jobs[i], "start");
        threads[i] = CreateThread(NULL, 0, camera_thread, &jobs[i], 0, NULL);
    }
    for (;;)
    {
        Sleep(500);
        if (jobs[0].done && jobs[1].done) break;
        for (i = 0; i < 2; i++)
        {
            if (!jobs[i].done && GetTickCount64() - jobs[i].phase_start > hang_seconds * 1000ULL)
            {
                fprintf(out, "HUNG: camera %d has been in \"%s\" for %d s\n", jobs[i].index, jobs[i].phase, hang_seconds);
                fflush(out);
                ExitProcess(3);
            }
        }
    }
    fprintf(out, "all %d cycles per camera finished\n", cycles);
    fclose(out);
    return 0;
}
