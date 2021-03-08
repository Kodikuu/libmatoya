// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <errno.h>

#include <sys/stat.h>

static FILE *mty_fopen(const char *path, const char *mode)
{
	FILE *f = fopen(path, mode);
	if (!f) {
		MTY_Log("'fopen' failed to open '%s' with errno %d", MTY_GetFileName(path, true), errno);
		return NULL;
	}

	return f;
}

static size_t mty_file_size(const char *path)
{
	struct stat st;
	int32_t e = stat(path, &st);

	if (e != 0) {
		// Since these functions are lazily used to check the existence of files, don't log ENOENT
		if (errno != ENOENT)
			MTY_Log("'stat' failed to query '%s' with errno %d", MTY_GetFileName(path, true), errno);

		return 0;
	}

	return st.st_size;
}
