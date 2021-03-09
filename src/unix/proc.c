// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdio.h>
#include <signal.h>
#include <errno.h>

#include "tlocal.h"
#include "procname.h"
#include "execv.h"

static void (*PROC_CRASH_HANDLER)(bool forced, void *opaque);
static void *PROC_OPAQUE;

const char *MTY_ProcessName(void)
{
	char *name = MTY_Alloc(MTY_PATH_MAX, 1);
	mty_proc_name(name, MTY_PATH_MAX);

	char *local = mty_tlocal_strcpy(name);
	MTY_Free(name);

	return local;
}

bool MTY_RestartProcess(char * const *argv)
{
	return mty_execv(MTY_ProcessName(), argv);
}

static void proc_signal_handler(int32_t sig)
{
	if (PROC_CRASH_HANDLER)
		PROC_CRASH_HANDLER(sig == SIGTERM || sig == SIGINT, PROC_OPAQUE);

	signal(sig, SIG_DFL);
	raise(sig);
}

void MTY_SetCrashHandler(void (*func)(bool forced, void *opaque), void *opaque)
{
	signal(SIGINT, proc_signal_handler);
	signal(SIGSEGV, proc_signal_handler);
	signal(SIGABRT, proc_signal_handler);
	signal(SIGTERM, proc_signal_handler);

	PROC_CRASH_HANDLER = func;
	PROC_OPAQUE = opaque;
}
