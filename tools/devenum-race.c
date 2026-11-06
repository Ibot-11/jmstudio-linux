/* Several threads list the DirectShow video capture devices at the same time, like JMStudio
 * does when it opens both scanner cameras. Counts how often a thread sees too few cameras. */
#define COBJMACROS
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <dshow.h>

static int threads = 2, iterations = 200, expected = 2;
static volatile LONG short_lists, errors, total;
static volatile LONG min_seen = 1000;

static DWORD WINAPI worker(void *arg)
{
    int i;

    CoInitializeEx(NULL, COINIT_MULTITHREADED);
    for (i = 0; i < iterations; i++)
    {
        ICreateDevEnum *dev_enum = NULL;
        IEnumMoniker *enum_moniker = NULL;
        IMoniker *moniker;
        int count = 0;
        HRESULT hr;

        hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, &IID_ICreateDevEnum, (void **)&dev_enum);
        if (FAILED(hr)) { InterlockedIncrement(&errors); continue; }
        hr = ICreateDevEnum_CreateClassEnumerator(dev_enum, &CLSID_VideoInputDeviceCategory, &enum_moniker, 0);
        if (hr == S_OK)
        {
            while (IEnumMoniker_Next(enum_moniker, 1, &moniker, NULL) == S_OK)
            {
                IPropertyBag *bag;
                VARIANT v;

                VariantInit(&v);
                if (SUCCEEDED(IMoniker_BindToStorage(moniker, NULL, NULL, &IID_IPropertyBag, (void **)&bag)))
                {
                    /* a camera counts only if its properties can be read */
                    if (SUCCEEDED(IPropertyBag_Read(bag, L"FriendlyName", &v, NULL))) count++;
                    VariantClear(&v);
                    IPropertyBag_Release(bag);
                }
                IMoniker_Release(moniker);
            }
            IEnumMoniker_Release(enum_moniker);
        }
        ICreateDevEnum_Release(dev_enum);

        InterlockedIncrement(&total);
        if (count < expected) InterlockedIncrement(&short_lists);
        if (count < min_seen) InterlockedExchange(&min_seen, count);
    }
    CoUninitialize();
    return 0;
}

int main(int argc, char **argv)
{
    HANDLE h[16];
    int i;

    if (argc > 1) threads = atoi(argv[1]);
    if (argc > 2) iterations = atoi(argv[2]);
    if (argc > 3) expected = atoi(argv[3]);
    if (threads > 16) threads = 16;

    for (i = 0; i < threads; i++) h[i] = CreateThread(NULL, 0, worker, NULL, 0, NULL);
    WaitForMultipleObjects(threads, h, TRUE, INFINITE);

    printf("threads=%d iterations=%d: %ld listings, %ld too short (min %ld cameras), %ld errors\n",
            threads, iterations, (long)total, (long)short_lists, (long)min_seen, (long)errors);
    return 0;
}
