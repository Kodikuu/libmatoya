// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdbool.h>
#include <stdio.h>

#include <sys/stat.h>

static FILE *mty_fopen(const char *path, const char *mode)
{
	wchar_t *wpath = MTY_MultiToWideD(path);
	wchar_t *wmode = MTY_MultiToWideD(mode);

	FILE *f = NULL;
	errno_t e = _wfopen_s(&f, wpath, wmode);

	MTY_Free(wpath);
	MTY_Free(wmode);

	if (e != 0) {
		MTY_Log("'_wfopen_s' failed to open '%s' with errno %d", MTY_GetFileName(path, true), e);
		return NULL;
	}

	return f;
}

static size_t mty_file_size(const char *path)
{
	wchar_t *wpath = MTY_MultiToWideD(path);

	struct __stat64 st;
	int32_t e = _wstat64(wpath, &st);

	MTY_Free(wpath);

	if (e != 0) {
		// Since these functions are lazily used to check the existence of files, don't log ENOENT
		if (errno != ENOENT)
			MTY_Log("'_wstat64' failed to query '%s' with errno %d", MTY_GetFileName(path, true), errno);

		return 0;
	}

	return (size_t) st.st_size;
}
