// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>

void *MTY_Alloc(size_t nelem, size_t elsize)
{
	void *mem = calloc(nelem, elsize);

	if (!mem)
		MTY_Fatal("'calloc' failed with errno %d", errno);

	return mem;
}

void *MTY_Realloc(void *mem, size_t nelem, size_t elsize)
{
	size_t size = elsize * nelem;

	void *new_mem = realloc(mem, size);

	if (!new_mem && size > 0)
		MTY_Fatal("'realloc' failed with errno %d", errno);

	return new_mem;
}

void *MTY_Dup(const void *mem, size_t size)
{
	void *dup = MTY_Alloc(size, 1);
	memcpy(dup, mem, size);

	return dup;
}

void *MTY_Strdup(const void *str)
{
	return MTY_Dup(str, strlen(str) + 1);
}

void MTY_Free(void *mem)
{
	free(mem);
}

char *MTY_WideToMultiD(const wchar_t *src)
{
	if (!src)
		return NULL;

	size_t len = wcslen(src) + 1;
	char *dst = MTY_Alloc(len, sizeof(wchar_t));

	MTY_WideToMulti(src, dst, len);

	return dst;
}

wchar_t *MTY_MultiToWideD(const char *src)
{
	if (!src)
		return NULL;

	uint32_t len = (uint32_t) strlen(src) + 1;
	wchar_t *dst = MTY_Alloc(len, sizeof(wchar_t));

	MTY_MultiToWide(src, dst, len);

	return dst;
}
