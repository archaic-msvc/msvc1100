//
// Copyright (C) Microsoft Corporation
// All rights reserved.
//

#include "pch.h"

#include <wrl/wrappers/corewrappers.h>


#pragma warning(disable: 4672 4673)

namespace Platform {
	namespace Details {

		class OutOfProcModule :
			public Microsoft::WRL::Module<Microsoft::WRL::OutOfProcDisableCaching, OutOfProcModule>,
			public __abi_Module
		{
		private:
			Microsoft::WRL::Details::ModuleBase **baseModulePtr;
			__abi_Module **abiModulePtr;
			//Store old values of module pointers
			Microsoft::WRL::Details::ModuleBase *baseModule;
			__abi_Module *abiModule;
		public:
			OutOfProcModule(ModuleBase **base, __abi_Module **abi) :
				baseModulePtr(base),
				abiModulePtr(abi),
				baseModule(*base),
				abiModule(*abi)
			{
				void* oldPtr = ::InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(baseModulePtr), static_cast<ModuleBase*>(this), baseModule);        
				if (oldPtr != baseModule)
				{
					__abi_WinRTraiseFailureException();
				}

				oldPtr = ::InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(abiModulePtr), static_cast<__abi_Module*>(this), abiModule);
				if (oldPtr != abiModule)
				{
					__abi_WinRTraiseFailureException();
				}
			}

			OutOfProcModule()	
			{
				throw ref new Platform::FailureException();
			}

			virtual ~OutOfProcModule()
			{
				void* oldPtr = ::InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(baseModulePtr), baseModule, static_cast<ModuleBase*>(this));
				if (oldPtr != static_cast<ModuleBase*>(this))
				{
					__abi_WinRTraiseFailureException();
				}

				oldPtr = ::InterlockedCompareExchangePointer(reinterpret_cast<void* volatile*>(abiModulePtr), abiModule, static_cast<__abi_Module*>(this));
				if (oldPtr != static_cast<__abi_Module*>(this))
				{
					__abi_WinRTraiseFailureException();
				}
			}

			virtual unsigned long __stdcall __abi_IncrementObjectCount()
			{
				return Module::IncrementObjectCount();
			}

			virtual unsigned long __stdcall __abi_DecrementObjectCount()
			{
				return Module::DecrementObjectCount();
			}

			STDMETHOD_(const Microsoft::WRL::Details::CreatorMap**, GetFirstEntryPointer)() const
			{
				return baseModule->GetFirstEntryPointer();
			}

			STDMETHOD_(const Microsoft::WRL::Details::CreatorMap**, GetMidEntryPointer)() const
			{
				return baseModule->GetMidEntryPointer();
			}

			STDMETHOD_(const Microsoft::WRL::Details::CreatorMap**, GetLastEntryPointer)() const
			{
				return baseModule->GetLastEntryPointer();
			}

			STDMETHOD_(SRWLOCK*, GetLock)() const
			{
				return baseModule->GetLock();
			}

			void Run(_In_z_ const ::default::char16* serverName)
			{
				::Microsoft::WRL::Wrappers::Event event(::CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS));
				if (!event.IsValid())
				{
					__abi_WinRTraiseException(HRESULT_FROM_WIN32(GetLastError()));
				}

				HANDLE hwnd = event.Get();
				HRESULT hr = Initialize([hwnd](){
					if (hwnd != nullptr)
					{
						if (!::SetEvent(hwnd))
						{
							__abi_WinRTraiseException(HRESULT_FROM_WIN32(GetLastError()));
						}
					}
				});

				if (FAILED(hr))
				{
					__abi_WinRTraiseException(hr);
				}

				hr = RegisterObjects(serverName);
				if (FAILED(hr))
				{
					__abi_WinRTraiseException(hr);
				}

				DWORD signaled = 0;    
				hr = ::CoWaitForMultipleHandles(COWAIT_WAITALL, INFINITE, 1, &hwnd, &signaled);
				if (FAILED(hr))
				{
					__abi_WinRTraiseException(hr);
				}

				hr = UnregisterObjects(serverName);
				if (FAILED(hr))
				{
					__abi_WinRTraiseException(hr);
				}
			}
		};

		VCCORLIB_API void STDMETHODCALLTYPE RunServer(_In_ Microsoft::WRL::Details::ModuleBase** module, __abi_Module** abiModule, _In_opt_z_ const ::default::char16* serverName)
		{
			OutOfProcModule serverModule(module, abiModule);
			serverModule.Run(serverName);
		}

	} }  // namespace Platform::Details
