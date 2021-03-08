// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
	#include <emmintrin.h>
#endif

#if defined(MTY_NEON)
	#define STBI_NEON
	#include <arm_neon.h>
#endif

#define STBI_MALLOC(size)        MTY_Alloc(size, 1)
#define STBI_REALLOC(ptr, size)  MTY_Realloc(ptr, size, 1)
#define STBI_FREE(ptr)           MTY_Free(ptr)
#define STBI_ASSERT(x)

#define STBIW_MALLOC(size)       MTY_Alloc(size, 1)
#define STBIW_REALLOC(ptr, size) MTY_Realloc(ptr, size, 1)
#define STBIW_FREE(ptr)          MTY_Free(ptr)
#define STBIW_ASSERT(x)

#include "stb_image.h"
#include "stb_image_write.h"

struct image_write {
	void *output;
	size_t size;
};

void *MTY_DecompressImage(const void *input, size_t size, uint32_t *width, uint32_t *height)
{
	int32_t channels = 0;
	void *output = stbi_load_from_memory(input, (int32_t) size, (int32_t *) width, (int32_t *) height, &channels, 4);

	if (!output) {
		if (stbi__g_failure_reason)
			MTY_Log("%s", stbi__g_failure_reason);

		MTY_Log("'stbi_load_from_memory' failed");
	}

	return output;
}

static void image_compress_write_func(void *context, void *data, int size)
{
	struct image_write *ctx = (struct image_write *) context;
	ctx->size = size;

	ctx->output = MTY_Alloc(ctx->size, 1);
	memcpy(ctx->output, data, ctx->size);
}

void *MTY_CompressImage(MTY_Image type, const void *input, uint32_t width, uint32_t height, size_t *outputSize)
{
	struct image_write ctx = {0};
	int32_t e = 0;

	switch (type) {
		case MTY_IMAGE_PNG:
			e = stbi_write_png_to_func(image_compress_write_func, &ctx, width, height, 4, input, width * 4);
			break;
		case MTY_IMAGE_JPG:
			// Quality defaults to 90
			e = stbi_write_jpg_to_func(image_compress_write_func, &ctx, width, height, 4, input, 0);
			break;
		default:
			MTY_Log("MTY_Image type %d not supported", type);
			return NULL;
	}

	if (e != 0) {
		*outputSize = ctx.size;

	} else {
		MTY_Log("'stbi_write_xxx_to_func' failed");
		MTY_Free(ctx.output);
		ctx.output = NULL;
	}

	return ctx.output;
}
