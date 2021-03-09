// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <string.h>

#include <mach-o/dyld.h>

static bool mty_proc_name(char *name, size_t size)
{
	uint32_t bsize = size - 1;
	int32_t e = _NSGetExecutablePath(name, &bsize);
	if (e != 0) {
		MTY_Log("'_NSGetExecutablePath' failed with error %d", e);
		return false;
	}

	return true;
}
