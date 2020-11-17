// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "gfx/mod.h"
GFX_PROTOTYPES(_metal_)

#include <Metal/Metal.h>

#include "gfx/viewport.h"
#include "shaders/metal/quad.h"

#define NUM_STAGING 3

struct gfx_metal_cb {
	float width;
	float height;
	float vp_height;
	uint32_t filter;
	uint32_t effect;
	uint32_t format;
};

struct gfx_metal_res {
	MTLPixelFormat format;
	id<MTLTexture> texture;
	uint32_t width;
	uint32_t height;
};

struct gfx_metal {
	MTY_ColorFormat format;
	struct gfx_metal_cb fcb;
	struct gfx_metal_res staging[NUM_STAGING];
	id<MTLLibrary> library;
	id<MTLFunction> fs;
	id<MTLFunction> vs;
	id<MTLBuffer> vb;
	id<MTLBuffer> ib;
	id<MTLRenderPipelineState> pipeline;
	id<MTLSamplerState> ss_nearest;
	id<MTLSamplerState> ss_linear;
	id<MTLTexture> niltex;
};

struct gfx *gfx_metal_create(MTY_Device *device)
{
	struct gfx_metal *ctx = MTY_Alloc(1, sizeof(struct gfx_metal));
	id<MTLDevice> _device = (__bridge id<MTLDevice>) device;

	bool r = true;

	MTLVertexDescriptor *vdesc = [MTLVertexDescriptor vertexDescriptor];
	MTLRenderPipelineDescriptor *pdesc = [MTLRenderPipelineDescriptor new];
	MTLSamplerDescriptor *sd = [MTLSamplerDescriptor new];
	MTLTextureDescriptor *tdesc = [MTLTextureDescriptor new];

	NSError *nse = nil;
	ctx->library = [_device newLibraryWithSource:[NSString stringWithUTF8String:MTL_LIBRARY] options:nil error:&nse];
	if (nse) {
		r = false;
		MTY_Log([[nse localizedDescription] UTF8String]);
		goto except;
	}

	ctx->vs = [ctx->library newFunctionWithName:@"vs"];
	ctx->fs = [ctx->library newFunctionWithName:@"fs"];

	float vdata[] = {
		-1.0f,  1.0f,	// Position 0
		 0.0f,  0.0f,	// TexCoord 0
		-1.0f, -1.0f,	// Position 1
		 0.0f,  1.0f,	// TexCoord 1
		 1.0f, -1.0f,	// Position 2
		 1.0f,  1.0f,	// TexCoord 2
		 1.0f,  1.0f,	// Position 3
		 1.0f,  0.0f	// TexCoord 3
	};

	ctx->vb = [_device newBufferWithBytes:vdata length:sizeof(vdata) options:MTLResourceCPUCacheModeWriteCombined];

	uint16_t idata[] = {
		0, 1, 2,
		2, 3, 0
	};

	ctx->ib = [_device newBufferWithBytes:idata length:sizeof(idata) options:MTLResourceCPUCacheModeWriteCombined];

	vdesc.attributes[0].offset = 0;
	vdesc.attributes[0].format = MTLVertexFormatFloat2;
	vdesc.attributes[1].offset = 2 * sizeof(float);
	vdesc.attributes[1].format = MTLVertexFormatFloat2;
	vdesc.layouts[0].stepRate = 1;
	vdesc.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
	vdesc.layouts[0].stride = 4 * sizeof(float);

	pdesc.sampleCount = 1;
	pdesc.vertexFunction = ctx->vs;
	pdesc.fragmentFunction = ctx->fs;
	pdesc.vertexDescriptor = vdesc;
	pdesc.colorAttachments[0].pixelFormat = MTLPixelFormatBGRA8Unorm;

	ctx->pipeline = [_device newRenderPipelineStateWithDescriptor:pdesc error:&nse];
	if (nse) {
		r = false;
		MTY_Log([[nse localizedDescription] UTF8String]);
		goto except;
	}

	sd.minFilter = sd.magFilter =  MTLSamplerMinMagFilterLinear;
	sd.sAddressMode = MTLSamplerAddressModeClampToEdge;
	sd.tAddressMode = MTLSamplerAddressModeClampToEdge;
	ctx->ss_linear = [_device newSamplerStateWithDescriptor:sd];

	sd.minFilter = sd.magFilter =  MTLSamplerMinMagFilterNearest;
	ctx->ss_nearest = [_device newSamplerStateWithDescriptor:sd];

	tdesc.pixelFormat = MTLPixelFormatRGBA8Unorm;
	tdesc.storageMode = MTLStorageModePrivate;
	tdesc.width = 64;
	tdesc.height = 64;
	ctx->niltex = [_device newTextureWithDescriptor:tdesc];

	except:

	if (!r)
		gfx_metal_destroy((struct gfx **) &ctx);

	return (struct gfx *) ctx;
}

static void gfx_metal_refresh_resource(struct gfx_metal_res *res, id<MTLDevice> device,
	MTLPixelFormat format, uint32_t width, uint32_t height)
{
	if (!res->texture || width != res->width || height != res->height || format != res->format) {
		res->texture = nil;

		MTLTextureDescriptor *tdesc = [MTLTextureDescriptor new];
		tdesc.pixelFormat = format;
		tdesc.cpuCacheMode = MTLCPUCacheModeWriteCombined;
		tdesc.width = width;
		tdesc.height = height;

		res->texture = [device newTextureWithDescriptor:tdesc];

		res->width = width;
		res->height = height;
		res->format = format;
	}
}

static void gfx_metal_reload_textures(struct gfx_metal *ctx, id<MTLDevice> device, const void *image, const MTY_RenderDesc *desc)
{
	switch (desc->format) {
		case MTY_COLOR_FORMAT_RGBA: {
			gfx_metal_refresh_resource(&ctx->staging[0], device, MTLPixelFormatRGBA8Unorm, desc->cropWidth, desc->cropHeight);

			MTLRegion region = MTLRegionMake2D(0, 0, desc->cropWidth, desc->cropHeight);
			[ctx->staging[0].texture replaceRegion:region mipmapLevel:0 withBytes:image bytesPerRow:4 * desc->imageWidth];
			break;
		}
		case MTY_COLOR_FORMAT_NV12:
		case MTY_COLOR_FORMAT_NV16: {
			uint32_t div = desc->format == MTY_COLOR_FORMAT_NV12 ? 2 : 1;

			// Y
			gfx_metal_refresh_resource(&ctx->staging[0], device, MTLPixelFormatR8Unorm, desc->cropWidth, desc->cropHeight);

			MTLRegion region = MTLRegionMake2D(0, 0, desc->cropWidth, desc->cropHeight);
			[ctx->staging[0].texture replaceRegion:region mipmapLevel:0 withBytes:image bytesPerRow:desc->imageWidth];

			// UV
			gfx_metal_refresh_resource(&ctx->staging[1], device, MTLPixelFormatRG8Unorm, desc->cropWidth / 2, desc->cropHeight / div);

			region = MTLRegionMake2D(0, 0, desc->cropWidth / 2, desc->cropHeight / div);
			[ctx->staging[1].texture replaceRegion:region mipmapLevel:0 withBytes:(uint8_t *) image + desc->imageWidth * desc->imageHeight bytesPerRow:desc->imageWidth];
			break;
		}
		case MTY_COLOR_FORMAT_I420:
		case MTY_COLOR_FORMAT_I444: {
			uint32_t div = desc->format == MTY_COLOR_FORMAT_I420 ? 2 : 1;

			// Y
			gfx_metal_refresh_resource(&ctx->staging[0], device, MTLPixelFormatR8Unorm, desc->cropWidth, desc->cropHeight);

			MTLRegion region = MTLRegionMake2D(0, 0, desc->cropWidth, desc->cropHeight);
			[ctx->staging[0].texture replaceRegion:region mipmapLevel:0 withBytes:image bytesPerRow:desc->imageWidth];

			// U
			uint8_t *p = (uint8_t *) image + desc->imageWidth * desc->imageHeight;
			gfx_metal_refresh_resource(&ctx->staging[1], device, MTLPixelFormatR8Unorm, desc->cropWidth / div, desc->cropHeight / div);

			region = MTLRegionMake2D(0, 0, desc->cropWidth / div, desc->cropHeight / div);
			[ctx->staging[1].texture replaceRegion:region mipmapLevel:0 withBytes:p bytesPerRow:desc->imageWidth / div];

			// V
			p += (desc->imageWidth / div) * (desc->imageHeight / div);
			gfx_metal_refresh_resource(&ctx->staging[2], device, MTLPixelFormatR8Unorm, desc->cropWidth / div, desc->cropHeight / div);

			region = MTLRegionMake2D(0, 0, desc->cropWidth / div, desc->cropHeight / div);
			[ctx->staging[2].texture replaceRegion:region mipmapLevel:0 withBytes:p bytesPerRow:desc->imageWidth / div];
			break;
		}
	}
}

bool gfx_metal_render(struct gfx *gfx, MTY_Device *device, MTY_Context *context,
	const void *image, const MTY_RenderDesc *desc, MTY_Texture *dest)
{
	struct gfx_metal *ctx = (struct gfx_metal *) gfx;
	id<MTLDevice> _device = (__bridge id<MTLDevice>) device;
	id<MTLCommandQueue> cq = (__bridge id<MTLCommandQueue>) context;
	id<MTLTexture> _dest = (__bridge id<MTLTexture>) dest;

	// Don't do anything until we have real data
	if (desc->format != MTY_COLOR_FORMAT_UNKNOWN)
		ctx->format = desc->format;

	if (ctx->format == MTY_COLOR_FORMAT_UNKNOWN)
		return true;

	gfx_metal_reload_textures(ctx, _device, image, desc);

	// Begin render pass
	MTLRenderPassDescriptor *rpd = [MTLRenderPassDescriptor new];
	rpd.colorAttachments[0].texture = _dest;
	rpd.colorAttachments[0].clearColor = MTLClearColorMake(0.0, 0.0, 0.0, 1.0);
	rpd.colorAttachments[0].loadAction = MTLLoadActionClear;
	rpd.colorAttachments[0].storeAction = MTLStoreActionStore;

	id<MTLCommandBuffer> cb = [cq commandBuffer];
	id<MTLRenderCommandEncoder> re = [cb renderCommandEncoderWithDescriptor:rpd];

	// Set both vertex and fragment shaders as part of the pipeline object
	[re setRenderPipelineState:ctx->pipeline];

	// Viewport
	float vpx, vpy, vpw, vph;
	mty_viewport(desc->cropWidth, desc->cropHeight, desc->viewWidth, desc->viewHeight,
		desc->aspectRatio, desc->scale, &vpx, &vpy, &vpw, &vph);

	MTLViewport vp = {0};
	vp.originX = vpx;
	vp.originY = vpy;
	vp.width = vpw;
	vp.height = vph;
	[re setViewport:vp];

	// Vertex shader
	[re setVertexBuffer:ctx->vb offset:0 atIndex:0];

	// Fragment shader
	id<MTLSamplerState> sampler = desc->filter == MTY_FILTER_NEAREST ? ctx->ss_nearest : ctx->ss_linear;
	[re setFragmentSamplerState:sampler atIndex:0];

	// Uniforms
	for (uint8_t x = 0; x < NUM_STAGING; x++) {
		id<MTLTexture> tex = ctx->staging[x].texture ? ctx->staging[x].texture : ctx->niltex;
		[re setFragmentTexture:tex atIndex:x];
	}

	ctx->fcb.format = ctx->format;
	ctx->fcb.effect = desc->effect;
	ctx->fcb.filter = desc->filter;
	ctx->fcb.width = desc->cropWidth;
	ctx->fcb.height = desc->cropHeight;
	ctx->fcb.vp_height = vph;
	[re setFragmentBytes:&ctx->fcb length:sizeof(struct gfx_metal_cb) atIndex:0];

	// Draw
	[re drawIndexedPrimitives:MTLPrimitiveTypeTriangleStrip indexCount:6
		indexType:MTLIndexTypeUInt16 indexBuffer:ctx->ib indexBufferOffset:0];

	// TODO Experiment with waitUntilScheduled?
	[re endEncoding];
	[cb commit];
	[cb waitUntilCompleted];

	return true;
}

void gfx_metal_destroy(struct gfx **gfx)
{
	if (!gfx || !*gfx)
		return;

	struct gfx_metal *ctx = (struct gfx_metal *) *gfx;

	for (uint8_t x = 0; x < NUM_STAGING; x++)
		ctx->staging[x].texture = nil;

	ctx->library = nil;
	ctx->fs = nil;
	ctx->vs = nil;
	ctx->vb = nil;
	ctx->ib = nil;
	ctx->pipeline = nil;
	ctx->ss_linear = nil;
	ctx->ss_nearest = nil;
	ctx->niltex = nil;

	MTY_Free(ctx);
	*gfx = NULL;
}


// State

void *gfx_metal_get_state(MTY_Device *device, MTY_Context *context)
{
	return NULL;
}

void gfx_metal_set_state(MTY_Device *device, MTY_Context *context, void *state)
{
}

void gfx_metal_free_state(void **state)
{
}
