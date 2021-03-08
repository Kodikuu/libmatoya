// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>
#include <errno.h>

#include "fsutil.h"

void *MTY_ReadFile(const char *path, size_t *size)
{
	size_t tmp = 0;
	if (!size)
		size = &tmp;

	*size = mty_file_size(path);
	if (*size == 0)
		return NULL;

	FILE *f = mty_fopen(path, "rb");
	if (!f)
		return NULL;

	void *data = MTY_Alloc(*size + 1, 1);

	if (fread(data, 1, *size, f) != *size) {
		MTY_Log("'fread' failed with ferror %d", ferror(f));
		MTY_Free(data);
		data = NULL;
		*size = 0;
	}

	fclose(f);

	return data;
}

bool MTY_WriteFile(const char *path, const void *data, size_t size)
{
	FILE *f = mty_fopen(path, "wb");
	if (!f)
		return false;

	bool r = true;
	if (fwrite(data, 1, size, f) != size) {
		MTY_Log("'fwrite' failed with ferror %d", ferror(f));
		r = false;
	}

	fclose(f);

	return r;
}

static bool fs_vfprintf(const char *path, const char *mode, const char *fmt, va_list args)
{
	FILE *f = mty_fopen(path, mode);
	if (!f)
		return false;

	bool r = true;
	if (vfprintf(f, fmt, args) < (int32_t) strlen(fmt)) {
		MTY_Log("'vfprintf' failed with ferror %d", ferror(f));
		r = false;
	}

	fclose(f);

	return r;
}

bool MTY_WriteTextFile(const char *path, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	bool r = fs_vfprintf(path, "w", fmt, args);

	va_end(args);

	return r;
}

bool MTY_AppendTextToFile(const char *path, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	bool r = fs_vfprintf(path, "a", fmt, args);

	va_end(args);

	return r;
}

void MTY_FreeFileList(MTY_FileList **fl)
{
	if (!fl || !*fl)
		return;

	for (uint32_t x = 0; x < (*fl)->len; x++) {
		MTY_Free((*fl)->files[x].name);
		MTY_Free((*fl)->files[x].path);
	}

	MTY_Free((*fl)->files);

	MTY_Free(*fl);
	*fl = NULL;
}
