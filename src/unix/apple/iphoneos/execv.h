// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

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
