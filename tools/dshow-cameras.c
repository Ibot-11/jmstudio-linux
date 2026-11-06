/* Lists DirectShow video capture devices and prints the properties JMStudio reads. */
#define COBJMACROS
#include <stdio.h>
#include <windows.h>
#include <dshow.h>

int main(void)
{
    ICreateDevEnum *dev_enum;
    IEnumMoniker *enum_moniker;
    IMoniker *moniker;
    HRESULT hr;
    ULONG count;

    CoInitialize(NULL);
    hr = CoCreateInstance(&CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER, &IID_ICreateDevEnum, (void **)&dev_enum);
    if (FAILED(hr)) { printf("CoCreateInstance failed: %08lx\n", hr); return 1; }

    hr = ICreateDevEnum_CreateClassEnumerator(dev_enum, &CLSID_VideoInputDeviceCategory, &enum_moniker, 0);
    if (hr != S_OK) { printf("no video input devices (hr=%08lx)\n", hr); return 0; }

    while (IEnumMoniker_Next(enum_moniker, 1, &moniker, &count) == S_OK)
    {
        static const WCHAR *names[] = { L"FriendlyName", L"DevicePath", L"Description" };
        IPropertyBag *bag;
        unsigned int i;

        if (SUCCEEDED(IMoniker_BindToStorage(moniker, NULL, NULL, &IID_IPropertyBag, (void **)&bag)))
        {
            for (i = 0; i < 3; i++)
            {
                VARIANT v;
                VariantInit(&v);
                hr = IPropertyBag_Read(bag, names[i], &v, NULL);
                if (SUCCEEDED(hr) && V_VT(&v) == VT_BSTR) wprintf(L"  %-12ls = %ls\n", names[i], V_BSTR(&v));
                else wprintf(L"  %-12ls : not available (hr=%08lx)\n", names[i], hr);
                VariantClear(&v);
            }
            wprintf(L"\n");
            IPropertyBag_Release(bag);
        }
        IMoniker_Release(moniker);
    }
    return 0;
}
