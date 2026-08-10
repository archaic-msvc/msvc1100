//
// Copyright (C) Microsoft Corporation
// All rights reserved.
//

#include "pch.h"

extern __abi_Module* __abi_module;

namespace Platform {
	namespace Details {

		//Forward declarations from vccorlib110.dll
		void STDMETHODCALLTYPE RunServer(_In_ Microsoft::WRL::Details::ModuleBase**, __abi_Module** abiModule, _In_z_ const ::default::char16*);

	} // namespace Details

	void STDMETHODCALLTYPE Module::RunServer(_In_opt_z_ const ::default::char16* serverName)
	{
		Platform::Details::RunServer(&Microsoft::WRL::Details::ModuleBase::module_, &__abi_module, serverName);
	}

} // namespace Platform