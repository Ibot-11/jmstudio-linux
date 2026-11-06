/* Mimics Qt 6.2's QWindowsMediaDevices::videoInputs(). */
#define COBJMACROS
#include <stdio.h>
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>

int main(void)
{
    IMFAttributes *attr;
    IMFActivate **devices = NULL;
    UINT32 count = 0, i, len;
    HRESULT hr;

    CoInitialize(NULL);
    hr = MFStartup(MF_VERSION, MFSTARTUP_FULL);
    printf("MFStartup: %08lx\n", hr);
    MFCreateAttributes(&attr, 1);
    IMFAttributes_SetGUID(attr, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE, &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID);

    hr = MFEnumDeviceSources(attr, &devices, &count);
    printf("MFEnumDeviceSources: hr=%08lx count=%u\n", hr, count);

    for (i = 0; i < count; i++)
    {
        WCHAR *name = NULL, *link = NULL;
        IMFMediaSource *source = NULL;
        IMFSourceReader *reader = NULL;

        IMFActivate_GetAllocatedString(devices[i], &MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &name, &len);
        IMFActivate_GetAllocatedString(devices[i], &MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_SYMBOLIC_LINK, &link, &len);
        wprintf(L"  [%u] name = %ls\n      link = %ls\n", i, name ? name : L"(none)", link ? link : L"(none)");

        hr = IMFActivate_ActivateObject(devices[i], &IID_IMFMediaSource, (void **)&source);
        printf("      ActivateObject: %08lx (source %p)\n", hr, source);
        hr = MFCreateSourceReaderFromMediaSource(source, NULL, &reader);
        printf("      MFCreateSourceReaderFromMediaSource: %08lx (reader %p)\n", hr, reader);

        if (reader) IMFSourceReader_Release(reader);
        if (source) IMFMediaSource_Release(source);
        CoTaskMemFree(name);
        CoTaskMemFree(link);
        IMFActivate_Release(devices[i]);
    }
    CoTaskMemFree(devices);
    IMFAttributes_Release(attr);
    MFShutdown();
    printf("done, no crash\n");
    return 0;
}
