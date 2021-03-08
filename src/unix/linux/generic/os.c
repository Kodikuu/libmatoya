// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void MTY_ProtocolHandler(const char *uri, void *token)
{
	const char *fmt = "xdg-open \"%s\" 2> /dev/null &";

	size_t size = snprintf(NULL, 0, fmt, uri) + 1;

	char *cmd = MTY_Alloc(size, 1);
	snprintf(cmd, size, fmt, uri);

	if (system(cmd) == -1)
		MTY_Log("'system' failed with errno %d", errno);

	MTY_Free(cmd);
}

uint32_t MTY_GetPlatform(void)
{
	return MTY_OS_LINUX;
}

uint32_t MTY_GetPlatformNoWeb(void)
{
	return MTY_GetPlatform();
}
