// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdio.h>
#include <stdlib.h>

#include "tlocal.h"

static void log_none(const char *msg, void *opaque)
{
}

static void (*LOG_CALLBACK)(const char *msg, void *opaque) = log_none;
static void *LOG_OPAQUE;

static MTY_TLOCAL char *LOG_MSG;
static MTY_TLOCAL bool LOG_PREVENT_RECURSIVE;
static MTY_Atomic32 LOG_DISABLED;

static void log_internal(const char *func, const char *msg, va_list args)
{
	if (MTY_Atomic32Get(&LOG_DISABLED) || LOG_PREVENT_RECURSIVE)
		return;

	char *fmt = MTY_SprintfD("%s: %s", func, msg);
	size_t len = vsnprintf(NULL, 0, fmt, args) + 1;

	LOG_MSG = mty_tlocal(len);
	vsnprintf(LOG_MSG, len, fmt, args);

	MTY_Free(fmt);

	LOG_PREVENT_RECURSIVE = true;
	LOG_CALLBACK(LOG_MSG, LOG_OPAQUE);
	LOG_PREVENT_RECURSIVE = false;
}

void MTY_LogParams(const char *func, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	log_internal(func, msg, args);
	va_end(args);
}

void MTY_FatalParams(const char *func, const char *msg, ...)
{
	va_list args;
	va_start(args, msg);
	log_internal(func, msg, args);
	va_end(args);

	_Exit(EXIT_FAILURE);
}

void MTY_SetLogCallback(void (*callback)(const char *msg, void *opaque), void *opaque)
{
	LOG_CALLBACK = callback ? callback : log_none;
	LOG_OPAQUE = opaque;
}

void MTY_DisableLog(bool disabled)
{
	MTY_Atomic32Set(&LOG_DISABLED, disabled ? 1 : 0);
}

const char *MTY_GetLog(void)
{
	return LOG_MSG ? LOG_MSG : "";
}
