//
// Copyright (C) Microsoft Corporation
// All rights reserved.
//

#include "pch.h"
#pragma hdrstop

#pragma warning(disable:4691)

namespace __abi_details
{
	__declspec(dllexport) Platform::Object ^ __stdcall __abi_cast_String_to_Object(Platform::String ^ p)
	{
		Platform::Object ^pTo = nullptr;

		if (p) 
		{
			pTo = Windows::Foundation::PropertyValue::CreateString(p);
		}
		return pTo;
	}

	__declspec(dllexport) Platform::String ^ __stdcall __abi_cast_Object_to_String(bool isDynamicCast, Platform::Object ^p)
	{
		if (p) 
		{
			if (isDynamicCast)
			{
				try
				{
					return ((Platform::IBox<Platform::String^>^)p)->Value;
				}
				catch (Platform::COMException^)
				{
				}
			}
			else
			{
				return ((Platform::IBox<Platform::String^>^)p)->Value;
			}
		}
		return nullptr;
	}
}

#pragma warning(disable:4483)
namespace Platform
{
	VCCORLIB_API String ^ String::Concat(Object^ __str1Arg, Object^ __str2Arg)
	{
		HSTRING __hstr = nullptr;
		__abi_ThrowIfFailed(WindowsConcatString(reinterpret_cast<HSTRING>(__str1Arg->ToString()), reinterpret_cast<HSTRING>(__str2Arg->ToString()), &__hstr));
		return *reinterpret_cast<String^*>(&__hstr);
	}

	VCCORLIB_API void String::__identifier("<Dispose>")()
	{
		if (!IsFastPass())
		{
			__Platform_WindowsDeleteString(reinterpret_cast<HSTRING>(this));
		}
	}
}