// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdio.h>

#include <windows.h>
#include <userenv.h>

#include "tlocal.h"

typedef _Return_type_success_(return >= 0) LONG NTSTATUS;

MTY_SO *MTY_SOLoad(const char *name)
{
	wchar_t *wname = MTY_MultiToWideD(name);

	MTY_SO *so = (MTY_SO *) LoadLibrary(wname);
	MTY_Free(wname);

	if (!so)
		MTY_Log("'LoadLibrary' failed to find '%s' with error 0x%X", name, GetLastError());

	return so;
}

void *MTY_SOGetSymbol(MTY_SO *so, const char *name)
{
	void *sym = (void *) GetProcAddress((HMODULE) so, name);
	if (!sym)
		MTY_Log("'GetProcAddress' failed to find '%s' with error 0x%X", name, GetLastError());

	return sym;
}

void MTY_SOUnload(MTY_SO **so)
{
	if (!so || !*so)
		return;

	if (!FreeLibrary((HMODULE) *so))
		MTY_Log("'FreeLibrary' failed with error 0x%X", GetLastError());

	*so = NULL;
}

const char *MTY_Hostname(void)
{
	DWORD lenw = MAX_COMPUTERNAME_LENGTH + 1;
	WCHAR tmp[MAX_COMPUTERNAME_LENGTH + 1] = {0};

	if (!GetComputerName(tmp, &lenw)) {
		MTY_Log("'GetComputerName' failed with error 0x%X", GetLastError());
		return "noname";
	}

	return mty_tlocal_strcpyw(tmp);
}

void MTY_ProtocolHandler(const char *uri, void *token)
{
	VOID *env = NULL;

	if (token) {
		if (!CreateEnvironmentBlock(&env, token, FALSE)) {
			MTY_Log("'CreateEnvironmentBlock' failed with error 0x%X", GetLastError());
			return;
		}
	}

	WCHAR *wuri = MTY_MultiToWideD(uri);
	WCHAR *cmd = MTY_Alloc(MAX_PATH, 1);
	_snwprintf_s(cmd, MAX_PATH, _TRUNCATE, L"rundll32 url.dll,FileProtocolHandler %s", wuri);

	STARTUPINFO si = {0};
	si.cb = sizeof(STARTUPINFO);
	si.lpDesktop = L"winsta0\\default";

	PROCESS_INFORMATION pi = {0};
	DWORD flags = NORMAL_PRIORITY_CLASS | CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT;

	BOOL success = FALSE;

	if (token) {
		success = CreateProcessAsUser(token, NULL, cmd, NULL, NULL, FALSE, flags, env, NULL, &si, &pi);
		if (!success)
			MTY_Log("'CreateProcessAsUser' failed with error 0x%X", GetLastError());

	} else {
		success = CreateProcess(NULL, cmd, NULL, NULL, FALSE, flags, env, NULL, &si, &pi);
		if (!success)
			MTY_Log("'CreateProcess' failed with error 0x%X", GetLastError());
	}

	if (success) {
		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}

	MTY_Free(wuri);
	MTY_Free(cmd);

	if (env)
		DestroyEnvironmentBlock(env);
}

uint32_t MTY_GetPlatform(void)
{
	uint32_t v = MTY_OS_WINDOWS;

	HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
	if (ntdll) {
		NTSTATUS (WINAPI *RtlGetVersion)(RTL_OSVERSIONINFOW *info) =
			(void *) GetProcAddress(ntdll, "RtlGetVersion");

		if (RtlGetVersion) {
			RTL_OSVERSIONINFOW info = {0};
			info.dwOSVersionInfoSize = sizeof(RTL_OSVERSIONINFOW);

			RtlGetVersion(&info);

			v |= (uint32_t) info.dwMajorVersion << 8;
			v |= (uint32_t) info.dwMinorVersion;
		}
	}

	return v;
}

uint32_t MTY_GetPlatformNoWeb(void)
{
	return MTY_GetPlatform();
}
