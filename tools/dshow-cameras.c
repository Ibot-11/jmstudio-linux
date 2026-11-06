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
            IPropertyBag_Release(bag);
        }
        {
            LPOLESTR display_name = NULL;
            IBindCtx *bind_ctx = NULL;
            IMoniker *parsed = NULL;
            ULONG eaten = 0;

            hr = IMoniker_GetDisplayName(moniker, NULL, NULL, &display_name);
            wprintf(L"  %-12ls = %ls (hr=%08lx)\n", L"DisplayName", display_name ? display_name : L"(none)", hr);
            if (display_name && SUCCEEDED(CreateBindCtx(0, &bind_ctx)))
            {
                hr = MkParseDisplayName(bind_ctx, display_name, &eaten, &parsed);
                wprintf(L"  %-12ls : MkParseDisplayName hr=%08lx%ls\n", L"round trip", hr,
                        parsed ? L" (moniker found)" : L"");
                if (parsed) IMoniker_Release(parsed);
                IBindCtx_Release(bind_ctx);
            }
            CoTaskMemFree(display_name);
            wprintf(L"\n");
        }
        IMoniker_Release(moniker);
    }
    return 0;
}
