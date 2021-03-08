// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdio.h>

#include <windows.h>
#include <process.h>

#include "tlocal.h"

static MTY_TLOCAL char PROC_NAME[MTY_PATH_MAX];
static MTY_TLOCAL char PROC_HOSTNAME[MTY_PATH_MAX];

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

const char *MTY_ProcessName(void)
{
	wchar_t tmp[MTY_PATH_MAX];
	memset(PROC_NAME, 0, MTY_PATH_MAX);

	DWORD e = GetModuleFileName(NULL, tmp, MTY_PATH_MAX);
	if (e > 0) {
		MTY_WideToMulti(tmp, PROC_NAME, MTY_PATH_MAX);

	} else {
		MTY_Log("'GetModuleFileName' failed with error 0x%X", GetLastError());
	}

	return PROC_NAME;
}

const char *MTY_Hostname(void)
{
	memset(PROC_HOSTNAME, 0, MTY_PATH_MAX);

	DWORD lenw = MAX_COMPUTERNAME_LENGTH + 1;
	wchar_t hostnamew[MAX_COMPUTERNAME_LENGTH + 1];

	if (GetComputerName(hostnamew, &lenw)) {
		MTY_WideToMulti(hostnamew, PROC_HOSTNAME, MTY_PATH_MAX);

	} else {
		MTY_Log("'GetComputerName' failed with error 0x%X", GetLastError());
		snprintf(PROC_HOSTNAME, MTY_PATH_MAX, "noname");
	}

	return PROC_HOSTNAME;
}

bool MTY_RestartProcess(int32_t argc, char * const *argv)
{
	WCHAR *name = MTY_MultiToWideD(MTY_ProcessName());
	WCHAR **argvn = NULL;

	for (int32_t x = 0; x < argc && argv[x]; x++) {
		argvn = MTY_Realloc(argvn, x + 2, sizeof(char *));
		argvn[x] = MTY_MultiToWideD(argv[x]);
		argvn[x + 1] = NULL;
	}

	bool r = _wexecv(name, argvn);
	MTY_Log("'_wexecv' failed with errno %d", errno);

	if (argvn) {
		for (int32_t x = 0; argvn[x]; x++)
			MTY_Free(argvn[x]);

		MTY_Free(argvn);
	}

	MTY_Free(name);

	return r;
}
