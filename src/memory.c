// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <wchar.h>

#include "tlocal.h"

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

char *MTY_Strdup(const char *str)
{
	return MTY_Dup(str, strlen(str) + 1);
}

void MTY_Strcat(char *dst, size_t size, const char *src)
{
	size_t dst_len = strlen(dst);
	size_t src_len = strlen(src);

	if (dst_len + src_len + 1 >= size)
		return;

	memcpy(dst + dst_len, src, src_len + 1);
}

char *MTY_VsprintfD(const char *fmt, va_list args)
{
	size_t size = vsnprintf(NULL, 0, fmt, args) + 1;
	char *str = MTY_Alloc(size, 1);

	vsnprintf(str, size, fmt, args);

	return str;
}

char *MTY_SprintfD(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);

	char *str = MTY_VsprintfD(fmt, args);

	va_end(args);

	return str;
}

void MTY_Free(void *mem)
{
	free(mem);
}

char *MTY_WideToMultiD(const wchar_t *src)
{
	if (!src)
		return NULL;

	// UTF8 may be up to 4 bytes per character
	size_t len = (wcslen(src) + 1) * 4;
	char *dst = MTY_Alloc(len, 1);

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


// Stable qsort

struct element {
	int32_t (*compare)(const void *, const void *);
	const void *orig;
};

static int32_t sort_compare(const void *a, const void *b)
{
	const struct element *element_a = a;
	const struct element *element_b = b;

	int32_t r = element_a->compare(element_a->orig, element_b->orig);

	// If zero is returned from original comare, use the memory address as the tie breaker
	return r != 0 ? r : (int32_t) ((uint8_t *) element_a->orig - (uint8_t *) element_b->orig);
}

void MTY_Sort(void *base, size_t nElements, size_t size, int32_t (*compare)(const void *a, const void *b))
{
	// Temporary copy of the base array for wrapping
	uint8_t *tmp = MTY_Alloc(nElements, size);
	memcpy(tmp, base, nElements * size);

	// Wrap the base array elements in a struct now with 'orig' memory addresses in ascending order
	struct element *wrapped = MTY_Alloc(nElements, sizeof(struct element));

	for (size_t x = 0; x < nElements; x++) {
		wrapped[x].compare = compare;
		wrapped[x].orig = tmp + x * size;
	}

	// Perform qsort using the original compare function, falling back to memory address comparison
	qsort(wrapped, nElements, sizeof(struct element), sort_compare);

	// Copy the reordered elements back to the base array
	for (size_t x = 0; x < nElements; x++)
		memcpy((uint8_t *) base + x * size, wrapped[x].orig, size);

	MTY_Free(tmp);
	MTY_Free(wrapped);
}


// Platform parsing

const char *MTY_VersionString(uint32_t platform)
{
	uint8_t major = (platform & 0xFF00) >> 8;
	uint8_t minor = platform & 0xFF;

	char *ver = mty_tlocal(16);

	if (minor > 0) {
		snprintf(ver, 16, "%u.%u", major, minor);

	} else {
		snprintf(ver, 16, "%u", major);
	}

	return ver;
}


// TLS protocol analysis

bool MTY_IsTLSHandshake(const void *buf, size_t size)
{
	const uint8_t *d = buf;

	return size > 2 &&
		((d[0] == 0x14 || d[0] == 0x16) &&  // Change Cipher Spec, Handshake
		((d[1] == 0xFE && d[2] == 0xFD) ||  // DTLS 1.2
		(d[1] == 0x03 && d[2] == 0x03)));   // TLS 1.2
}

bool MTY_IsTLSApplicationData(const void *buf, size_t size)
{
	const uint8_t *d = buf;

	return size > 2 &&
		d[0] == 0x17 &&                    // Application Data
		((d[1] == 0xFE && d[2] == 0xFD) || // DTLS 1.2
		(d[1] == 0x03 && d[2] == 0x03));   // TLS 1.2
}
