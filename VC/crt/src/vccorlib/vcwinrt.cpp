//
// Copyright (C) Microsoft Corporation
// All rights reserved.
//

#include "pch.h"
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include "activation.h"
#pragma hdrstop
#include <Strsafe.h>

CPPCLI_FUNC HRESULT __stdcall __getActivationFactoryByHSTRING(HSTRING str, ::Platform::Guid& riid, PVOID * ppActivationFactory)
{
    HRESULT hr = S_OK;
    IActivationFactory* pActivationFactory;
    hr = Windows::Foundation::GetActivationFactory(str, &pActivationFactory);
    if (SUCCEEDED(hr))
    {
        hr = pActivationFactory->QueryInterface(reinterpret_cast<REFIID>(riid), ppActivationFactory);
        pActivationFactory->Release();
    }

    return hr;
}

using namespace Microsoft::WRL;
using namespace std;

class PerApartmentFactoryCache;
class FactoryCache;

class PerApartmentFactoryCache : public RuntimeClass<RuntimeClassFlags<ClassicCom>, IInitializeSpy>
{
    unordered_map<std::wstring, ComPtr<IUnknown>> _factoryCache;
    ULARGE_INTEGER _ulCoInitSpyCookie;
    DWORD _apartmentID;
    Wrappers::CriticalSection _criticalSection;
    bool _apartmentIsSTA;

public:
    HRESULT RuntimeClassInitialize(DWORD apartmentID)
    {
        _ulCoInitSpyCookie.QuadPart = 0;
        _apartmentID = apartmentID;
        _apartmentIsSTA = (apartmentID == 0 || apartmentID == -1);
        return S_OK;
    }

    HRESULT SetCookie(ULARGE_INTEGER cookie)
    {
        _ulCoInitSpyCookie = cookie;
        return S_OK;
    }
    
    HRESULT AddFactory(LPCWSTR acid, IUnknown *factory)
    {
        try
        {
            wstring activatableID(acid);

            auto lock  = _criticalSection.Lock();
            auto ret = _factoryCache.insert(pair<wstring, ComPtr<IUnknown>>(activatableID, factory));
            if (ret.second == false)
            {
                return S_FALSE;
            }
            return S_OK;
        }
        catch (bad_alloc e)
        {
            return E_OUTOFMEMORY;
        }
        catch (exception e)
        {
            return E_FAIL;
        }
    }
    
    HRESULT GetFactory(LPCWSTR acid, Platform::Guid& iid, void** pFactory)
    {
        *pFactory = nullptr;
        try
        {
            wstring activatableID(acid);
            decltype(_factoryCache.find(activatableID)) it;

            if (!_apartmentIsSTA)
            {
                auto lock  = _criticalSection.Lock();
                it = _factoryCache.find(activatableID);
                if (it == _factoryCache.end())
                {
                    return E_FAIL;
                }
            }
            else
            {
                it = _factoryCache.find(activatableID);
                if (it == _factoryCache.end())
                {
                    return E_FAIL;
                }
            }
            return it->second.CopyTo(iid, pFactory);
        }
        catch (bad_alloc e)
        {
            return E_OUTOFMEMORY;
        }
        catch (exception e)
        {
            return E_FAIL;
        }
    }

    IFACEMETHODIMP PreInitialize(DWORD /*dwCoInit*/, DWORD /*cCurThreadAptRefs*/) 
    {
        return S_OK; 
    }

    IFACEMETHODIMP PostInitialize(HRESULT hrCoInit, DWORD /*dwCoInit*/, DWORD /*cNewThreadAptRefs*/) 
    { 
        return hrCoInit; 
    }

    IFACEMETHODIMP PreUninitialize(DWORD cCurThreadAptRefs);
    IFACEMETHODIMP PostUninitialize(DWORD /*cNewThreadAptRefs*/) 
    { 
        return S_OK; 
    }
    
};

class FactoryCache
{
    vector<pair<DWORD, pair<ULARGE_INTEGER, ComPtr<PerApartmentFactoryCache>>>> perApartmentCache;
    Wrappers::CriticalSection _criticalSection;
    static bool _cacheEnabled;
public:
    static void Enable()
    {
        _cacheEnabled = true;
    }
    static void Disable()
    {
        _cacheEnabled = false;
    }
    static bool IsEnabled()
    {
        return _cacheEnabled;
    }
    ~FactoryCache()
    {
        Flush();
    }
    void Flush()
    {
        for(auto it = perApartmentCache.begin(); it != perApartmentCache.end(); it++)
        {
            CoRevokeInitializeSpy(it->second.first);
        }
        perApartmentCache.clear();
    }
    HRESULT GetFactory(LPCWSTR acid, Platform::Guid& iid, void** pFactory)
    {
        DWORD apartmentID;
        HRESULT hr = CoGetCallerTID(&apartmentID);
        ComPtr<PerApartmentFactoryCache> apartmentCache;
        if (SUCCEEDED(hr))
        {
            auto lock  = _criticalSection.Lock();
            for(auto it = perApartmentCache.begin(); it != perApartmentCache.end(); it++)
            {
                if (it->first == apartmentID)
                {
                    lock.Unlock();
                    hr = it->second.second->GetFactory(acid, iid, pFactory);
                    if (SUCCEEDED(hr))
                    {
                        return hr;
                    }
                    apartmentCache = it->second.second;
                    break;
                }
            }
            if (apartmentCache == nullptr)
            {
                hr = MakeAndInitialize<PerApartmentFactoryCache, PerApartmentFactoryCache>(&apartmentCache, apartmentID);
                if (SUCCEEDED(hr))
                {
                    ULARGE_INTEGER cookie;
                    hr = CoRegisterInitializeSpy(apartmentCache.Get(), &cookie);
                    if (SUCCEEDED(hr))
                    {
                        apartmentCache->SetCookie(cookie);
                        perApartmentCache.push_back(pair<DWORD, pair<ULARGE_INTEGER, ComPtr<PerApartmentFactoryCache>>>(apartmentID, pair<ULARGE_INTEGER, ComPtr<PerApartmentFactoryCache>>(cookie, apartmentCache.Get())));
                    }
                }
            }
        }

        // Create Factory
        HSTRING className;
        hr = ::WindowsCreateString(acid, static_cast<UINT32>(wcslen(acid)), &className); 
        if (FAILED(hr))
        {
            return hr;
        }

        ComPtr<IUnknown> factory;
        Platform::Guid riidUnknown(__uuidof(IUnknown));
        hr = __getActivationFactoryByHSTRING(className, riidUnknown, &factory);

        ::WindowsDeleteString(className);

        if (FAILED(hr))
        {
            return hr;
        }
            
        if (apartmentCache != nullptr)
        {
            apartmentCache->AddFactory(acid, factory.Get());
        }
    
        return factory.CopyTo(iid, pFactory);
    }
    
    HRESULT RemoveApartmentCache(ULARGE_INTEGER cookie)
    {
        CoRevokeInitializeSpy(cookie);

        auto lock  = _criticalSection.Lock();

        
        for(auto it = perApartmentCache.begin(); it != perApartmentCache.end(); it++)
        {
            if (it->second.first.QuadPart == cookie.QuadPart)
            {
                perApartmentCache.erase(it);
                return S_OK;
            }
        }
        return E_FAIL;
    }
};

bool FactoryCache::_cacheEnabled = false;
FactoryCache g_FactoryCache;

IFACEMETHODIMP PerApartmentFactoryCache::PreUninitialize(DWORD cCurThreadAptRefs)
{
    if (cCurThreadAptRefs == 1) // equals one since this is "Pre" Uninitialize 
    {
        g_FactoryCache.RemoveApartmentCache(_ulCoInitSpyCookie);
    }
    return S_OK;
}

CPPCLI_FUNC void EnableFactoryCache()
{
    FactoryCache::Enable();
}

CPPCLI_FUNC void __stdcall FlushFactoryCache()
{
    g_FactoryCache.Flush();
}

CPPCLI_FUNC HRESULT __stdcall GetActivationFactoryByPCWSTR(void* str, ::Platform::Guid& riid, void** ppActivationFactory)
{
    wchar_t* acid =  static_cast<wchar_t*>(str);
    if (!FactoryCache::IsEnabled())
    {
        HSTRING className;
        HRESULT hr = ::WindowsCreateString(acid, static_cast<UINT32>(wcslen(acid)), &className); 
        if (SUCCEEDED(hr))
        {
            hr = __getActivationFactoryByHSTRING(className, riid, ppActivationFactory);
            ::WindowsDeleteString(className);
        }
        return hr;
    }
    return g_FactoryCache.GetFactory(acid, riid, ppActivationFactory);
}

CPPCLI_FUNC HRESULT __stdcall GetIidsFn(int nIids, unsigned long* iidCount, const __s_GUID* pIids, ::Platform::Guid** ppDuplicated)
{
    int nBytes = nIids * sizeof(::Platform::Guid);

    *ppDuplicated = (::Platform::Guid*)CoTaskMemAlloc(nBytes);
    if (*ppDuplicated)
    {
        memcpy(*ppDuplicated, pIids, nBytes);
        *iidCount = nIids;
        return S_OK;
    }

    *iidCount = 0;
    return E_OUTOFMEMORY;
}

#include "compiler.cpp"
#include "activation.cpp"
