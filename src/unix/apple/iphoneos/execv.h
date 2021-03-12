// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the MIT License.
// If a copy of the MIT License was not distributed with this file,
// You can obtain one at https://spdx.org/licenses/MIT.html.

#pragma once

#include <stdbool.h>
#include <errno.h>

#include <unistd.h>

static bool mty_execv(const char *name, char * const *argv)
{
	execv(name, argv);
	MTY_Log("'execv' failed with errno %d", errno);

	return false;
}
