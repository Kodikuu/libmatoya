// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdio.h>
#include <signal.h>

#include <windows.h>
#include <process.h>

#include "tlocal.h"

static MTY_CrashFunc PROC_CRASH_FUNC;
static void *PROC_OPAQUE;

const char *MTY_ProcessName(void)
{
	WCHAR tmp[MTY_PATH_MAX] = {0};

	if (GetModuleFileName(NULL, tmp, MTY_PATH_MAX) <= 0)
		MTY_Log("'GetModuleFileName' failed with error 0x%X", GetLastError());

	return mty_tlocal_strcpyw(tmp);
}

bool MTY_RestartProcess(char * const *argv)
{
	WCHAR *name = MTY_MultiToWideD(MTY_ProcessName());
	WCHAR **argvn = NULL;

	for (uint32_t x = 0; argv && argv[x]; x++) {
		argvn = MTY_Realloc(argvn, x + 2, sizeof(char *));
		argvn[x] = MTY_MultiToWideD(argv[x]);
		argvn[x + 1] = NULL;
	}

	bool r = _wexecv(name, argvn);
	MTY_Log("'_wexecv' failed with errno %d", errno);

	for (uint32_t x = 0; argvn && argvn[x]; x++)
		MTY_Free(argvn[x]);

	MTY_Free(argvn);
	MTY_Free(name);

	return r;
}

static LONG WINAPI proc_exception_handler(EXCEPTION_POINTERS *ex)
{
	if (PROC_CRASH_FUNC)
		PROC_CRASH_FUNC(false, PROC_OPAQUE);

	return EXCEPTION_CONTINUE_SEARCH;
}

static void proc_signal_handler(int32_t sig)
{
	if (PROC_CRASH_FUNC)
		PROC_CRASH_FUNC(sig == SIGTERM || sig == SIGINT, PROC_OPAQUE);

	signal(sig, SIG_DFL);
	raise(sig);
}

void MTY_SetCrashFunc(MTY_CrashFunc func, void *opaque)
{
	SetUnhandledExceptionFilter(proc_exception_handler);

	signal(SIGINT, proc_signal_handler);
	signal(SIGSEGV, proc_signal_handler);
	signal(SIGABRT, proc_signal_handler);
	signal(SIGTERM, proc_signal_handler);

	PROC_CRASH_FUNC = func;
	PROC_OPAQUE = opaque;
}
