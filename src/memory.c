// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
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

static MTY_TLOCAL char MEMORY_VER[32];

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


// Platform parsing

const char *MTY_VersionString(uint32_t platform)
{
	uint8_t major = (platform & 0xFF00) >> 8;
	uint8_t minor = platform & 0xFF;

	if (minor > 0) {
		snprintf(MEMORY_VER, 32, "%u.%u", major, minor);

	} else {
		snprintf(MEMORY_VER, 32, "%u", major);
	}

	return MEMORY_VER;
}


// Misc protocol analysis

// 0x14   - Change Cipher Spec
// 0x16   - Handshake
// 0x17   - Application Data

// 0xFEFF - DTLS 1.0
// 0xFEFD - DTLS 1.2
// 0x0303 - TLS 1.2

bool MTY_TLSIsHandshake(const void *buf, size_t size)
{
	const uint8_t *d = buf;

	return size > 2 && (d[0] == 0x14 || d[0] == 0x16) && (
		(d[1] == 0xFE || d[1] == 0x03) &&
		(d[2] == 0xFD || d[2] == 0xFF || d[2] == 0x03)
	);
}

bool MTY_TLSIsApplicationData(const void *buf, size_t size)
{
	const uint8_t *d = buf;

	return size > 2 && d[0] == 0x17 && (
		(d[1] == 0xFE || d[1] == 0x03) &&
		(d[2] == 0xFD || d[2] == 0xFF || d[2] == 0x03)
	);
}
