#include <dshow.h>
#include <mfapi.h>
#include <mfidl.h>
#include <iostream>

#pragma comment(lib, "strmiids")
#pragma comment(lib, "mfplat")
#pragma comment(lib, "mf")
#pragma comment(lib, "mfreadwrite")
#pragma comment(lib, "mfuuid")

// listDevicesDirectShow is used to enumerate and print DirectShow devices
void listDevicesDirectShow(IMoniker* pMoniker) {
    IPropertyBag* pPropBagRaw = nullptr;
    HRESULT hr = pMoniker->BindToStorage(nullptr, nullptr, IID_PPV_ARGS(&pPropBagRaw));
    if (FAILED(hr)) {
        pMoniker->Release();
        return;
    }
    std::unique_ptr<IPropertyBag, void(*)(IPropertyBag*)> pPropBag{pPropBagRaw, [](IPropertyBag* pb){pb->Release();}};

    // Get the device name
    VARIANT varName;
    VariantInit(&varName);
    hr = pPropBag->Read(L"FriendlyName", &varName, nullptr);
    if (SUCCEEDED(hr)) {
        std::wcout << L"DirectShow: " << varName.bstrVal << std::endl;
    }
}

// listDevicesMediaFoundation is used to enumerate and print Media Foundation devices
void listDevicesMediaFoundation(IMFActivate* pActivate) {
    WCHAR* szFriendlyNameRaw = nullptr;

    // Get the device name
    HRESULT hr = pActivate->GetAllocatedString(MF_DEVSOURCE_ATTRIBUTE_FRIENDLY_NAME, &szFriendlyNameRaw, nullptr);
    if (SUCCEEDED(hr)) {
        std::unique_ptr<WCHAR, void(*)(WCHAR*)> szFriendlyName{szFriendlyNameRaw, [](WCHAR* str){CoTaskMemFree(str);}};
        std::wcout << L"Media Foundation: " << szFriendlyName.get() << std::endl;
    }
}

int main() {
    // Initialize COM
    CoInitialize(nullptr);

    // Enumerate DirectShow devices
    {
        ICreateDevEnum* pDevEnumRaw = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_SystemDeviceEnum, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDevEnumRaw));
        if (SUCCEEDED(hr)) {
            std::unique_ptr<ICreateDevEnum, void(*)(ICreateDevEnum*)> pDevEnum{pDevEnumRaw, [](ICreateDevEnum* de){de->Release();}};

            IEnumMoniker* pEnumRaw = nullptr;
            hr = pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pEnumRaw, 0);
            if(hr == S_OK) {
                std::unique_ptr<IEnumMoniker, void(*)(IEnumMoniker*)> pEnum{pEnumRaw, [](IEnumMoniker* em){em->Release();}};

                IMoniker* pMoniker = nullptr;
                while (pEnum->Next(1, &pMoniker, nullptr) == S_OK) {
                    listDevicesDirectShow(pMoniker);
                }
            }
        }else{
            std::cout << "Failed to create DirectShow device enumerator" << std::endl;
        }
    }

    // Enumerate Media Foundation devices
    {
        IMFAttributes *pAttributes = nullptr;
        HRESULT hr = MFCreateAttributes(&pAttributes, 1);
        if (SUCCEEDED(hr)) {
            hr = pAttributes->SetGUID(
                    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE,
                    MF_DEVSOURCE_ATTRIBUTE_SOURCE_TYPE_VIDCAP_GUID
            );

            if (SUCCEEDED(hr)) {
                IMFActivate** ppDevices = nullptr;
                UINT32 deviceCount = 0;

                hr = MFStartup(MF_VERSION);
                if (SUCCEEDED(hr)) {
                    hr = MFEnumDeviceSources(pAttributes, &ppDevices, &deviceCount);
                    if (SUCCEEDED(hr)) {
                        // Use smart pointers to manage the array of device interfaces
                        std::unique_ptr<IMFActivate*[], void(*)(IMFActivate**)> devices{ppDevices, [](IMFActivate** d){CoTaskMemFree(d);}};
                        for(UINT32 i = 0; i < deviceCount; ++i) {
                            listDevicesMediaFoundation(devices[i]);
                            devices[i]->Release();
                        }
                    }
                    MFShutdown();
                }
            }else{
                std::cout << "Failed to set Media Foundation device attribute" << std::endl;
            }
            pAttributes->Release();
        }else{
            std::cout << "Failed to create Media Foundation attribute store" << std::endl;
        }
    }

    // Uninitialize COM
    CoUninitialize();
    return 0;
}
