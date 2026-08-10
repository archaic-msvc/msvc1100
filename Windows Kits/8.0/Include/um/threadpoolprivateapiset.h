 
/********************************************************************************
*                                                                               *
* threadpoolapi.h -- ApiSet Contract for api-ms-win-core-threadpool-l1          *
*                                                                               *
* Copyright (c) Microsoft Corporation. All rights reserved.                     *
*                                                                               *
********************************************************************************/

#ifdef _MSC_VER
#pragma once
#endif // _MSC_VER

#ifndef _THREADPOOLPRIVATEAPISET_H_
#define _THREADPOOLPRIVATEAPISET_H_

#include <apiset.h>
#include <apisetcconv.h>
#include <minwindef.h>
#include <minwinbase.h>

/* APISET_NAME: api-ms-win-core-threadpool-private-l1 */

#ifndef _APISET_THREADPOOL_PRIVATE_VER
#ifdef _APISET_MINWIN_VERSION
#if _APISET_MINWIN_VERSION >= 0x0101
#define _APISET_THREADPOOL_PRIVATE_VER 0x0100
#endif
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

//
// Thread pool API's
//

#pragma region Desktop Family

#if WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP)

WINBASEAPI
HANDLE
WINAPI
RegisterWaitForSingleObjectEx(
    _In_ HANDLE hObject,
    _In_ WAITORTIMERCALLBACK Callback,
    _In_opt_ PVOID Context,
    _In_ ULONG dwMilliseconds,
    _In_ ULONG dwFlags
    );


WINBASEAPI
BOOL
WINAPI
SetThreadpoolTimerEx(
    _Inout_ PTP_TIMER pti,
    _In_opt_ PFILETIME pftDueTime,
    _In_ DWORD msPeriod,
    _In_opt_ DWORD msWindowLength
    );


WINBASEAPI
BOOL
WINAPI
SetThreadpoolWaitEx(
    _Inout_ PTP_WAIT pwa,
    _In_opt_ HANDLE h,
    _In_opt_ PFILETIME pftTimeout,
    _In_ _Reserved_ PVOID Reserved
    );


#endif /* WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP) */
#pragma endregion

#ifdef __cplusplus
}
#endif

#endif // _THREADPOOLPRIVATEAPISET_H_
