/*
 * Copyright (c) 2021 Apple Inc. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

 // dllmain.cpp : Defines the entry point for the DLL application.

#pragma comment(lib,"ws2_32.lib")
#pragma comment(lib,"netapi32.lib")
#pragma comment(lib,"iphlpapi.lib")
#pragma comment(lib,"powrprof.lib")

#include <windows.h>
#include "DebugServices.h"
#include <stdlib.h>
#include "dns_sd.h"

BOOL APIENTRY DllMain(HMODULE /*hModule*/,
	DWORD  ul_reason_for_call,
	LPVOID /*lpReserved*/
)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}

//判断本机bonjour服务是否启动
EXTERN_C BOOL DNSSD_API IsBonjourServiceDisabled()
{
	ENUM_SERVICE_STATUSA* lpService = NULL;
	SC_HANDLE				sc;
	BOOL					installed = FALSE;
	BOOL					ret = FALSE;
	BOOL					ok;
	DWORD					bytesNeeded = 0;
	DWORD					srvCount;
	DWORD					resumeHandle = 0;
	DWORD					srvType;
	DWORD					srvState;
	DWORD					dwBytes = 0;
	DWORD					i;
	OSStatus				err;

	sc = OpenSCManager(NULL, NULL, SC_MANAGER_ENUMERATE_SERVICE);
	err = translate_errno(sc, GetLastError(), kUnknownErr);
	require_noerr(err, exit);

	srvType = SERVICE_WIN32;
	srvState = SERVICE_STATE_ALL;

	for (;; )
	{
		// Call EnumServicesStatus using the handle returned by OpenSCManager

		ok = EnumServicesStatusA(sc, srvType, srvState, lpService, dwBytes, &bytesNeeded, &srvCount, &resumeHandle);

		if (ok || (GetLastError() != ERROR_MORE_DATA))
		{
			break;
		}

		if (lpService)
		{
			free(lpService);
		}

		dwBytes = bytesNeeded;

		lpService = (ENUM_SERVICE_STATUSA*)malloc(dwBytes);
		require_action(lpService, exit, ret = FALSE);
	}

	err = translate_errno(ok, GetLastError(), kUnknownErr);
	require_noerr(err, exit);

	for (i = 0; i < srvCount; i++)
	{
		if (strcmp(lpService[i].lpServiceName, "Bonjour Service") == 0)
		{
			installed = TRUE;

			if ((lpService[i].ServiceStatus.dwCurrentState == SERVICE_PAUSED) || (lpService[i].ServiceStatus.dwCurrentState == SERVICE_STOPPED))
			{
				ret = TRUE;
			}

			break;
		}
	}

exit:

	if (lpService)
	{
		free(lpService);
	}

	if (sc)
	{
		CloseServiceHandle(sc);
	}

	return (ret || installed == FALSE);
}



