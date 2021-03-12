// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#include "matoya.h"

#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include "tlocal.h"
#include "procname.h"
#include "execv.h"

static MTY_CrashFunc PROC_CRASH_FUNC;
static void *PROC_OPAQUE;

const char *MTY_ProcessName(void)
{
	char name[MTY_PATH_MAX] = {0};
	mty_proc_name(name, MTY_PATH_MAX);

	return mty_tlocal_strcpy(name);
}

bool MTY_RestartProcess(char * const *argv)
{
	return mty_execv(MTY_ProcessName(), argv);
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
	signal(SIGINT, proc_signal_handler);
	signal(SIGSEGV, proc_signal_handler);
	signal(SIGABRT, proc_signal_handler);
	signal(SIGTERM, proc_signal_handler);

	PROC_CRASH_FUNC = func;
	PROC_OPAQUE = opaque;
}
