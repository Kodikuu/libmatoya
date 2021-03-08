// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "app.h"

GFX_CTX_PROTOTYPES(_gl_)
GFX_CTX_PROTOTYPES(_d3d9_)
GFX_CTX_PROTOTYPES(_d3d11_)
GFX_CTX_PROTOTYPES(_metal_)
GFX_CTX_DECLARE_TABLE()

void MTY_WindowPresent(MTY_App *app, MTY_Window window, uint32_t numFrames)
{
	struct gfx_ctx *gfx_ctx = NULL;
	MTY_GFX api = mty_window_get_gfx(app, window, &gfx_ctx);

	if (api != MTY_GFX_NONE)
		GFX_CTX_API[api].present(gfx_ctx, numFrames);
}

MTY_Device *MTY_WindowGetDevice(MTY_App *app, MTY_Window window)
{
	struct gfx_ctx *gfx_ctx = NULL;
	MTY_GFX api = mty_window_get_gfx(app, window, &gfx_ctx);

	return api != MTY_GFX_NONE ? GFX_CTX_API[api].get_device(gfx_ctx) : NULL;
}

MTY_Context *MTY_WindowGetContext(MTY_App *app, MTY_Window window)
{
	struct gfx_ctx *gfx_ctx = NULL;
	MTY_GFX api = mty_window_get_gfx(app, window, &gfx_ctx);

	return api != MTY_GFX_NONE ? GFX_CTX_API[api].get_context(gfx_ctx) : NULL;
}

MTY_Texture *MTY_WindowGetBackBuffer(MTY_App *app, MTY_Window window)
{
	struct gfx_ctx *gfx_ctx = NULL;
	MTY_GFX api = mty_window_get_gfx(app, window, &gfx_ctx);

	return api != MTY_GFX_NONE ? GFX_CTX_API[api].get_buffer(gfx_ctx) : NULL;
}

void MTY_WindowDrawQuad(MTY_App *app, MTY_Window window, const void *image, const MTY_RenderDesc *desc)
{
	struct gfx_ctx *gfx_ctx = NULL;
	MTY_GFX api = mty_window_get_gfx(app, window, &gfx_ctx);

	if (api != MTY_GFX_NONE)
		GFX_CTX_API[api].draw_quad(gfx_ctx, image, desc);
}

void MTY_WindowDrawUI(MTY_App *app, MTY_Window window, const MTY_DrawData *dd)
{
	struct gfx_ctx *gfx_ctx = NULL;
	MTY_GFX api = mty_window_get_gfx(app, window, &gfx_ctx);

	if (api != MTY_GFX_NONE)
		GFX_CTX_API[api].draw_ui(gfx_ctx, dd);
}

void MTY_WindowSetUITexture(MTY_App *app, MTY_Window window, uint32_t id, const void *rgba, uint32_t width, uint32_t height)
{
	struct gfx_ctx *gfx_ctx = NULL;
	MTY_GFX api = mty_window_get_gfx(app, window, &gfx_ctx);

	if (api != MTY_GFX_NONE)
		GFX_CTX_API[api].set_ui_texture(gfx_ctx, id, rgba, width, height);
}

void *MTY_WindowGetUITexture(MTY_App *app, MTY_Window window, uint32_t id)
{
	struct gfx_ctx *gfx_ctx = NULL;
	MTY_GFX api = mty_window_get_gfx(app, window, &gfx_ctx);

	return api != MTY_GFX_NONE ? GFX_CTX_API[api].get_ui_texture(gfx_ctx, id) : NULL;
}

MTY_GFX MTY_WindowGetGFX(MTY_App *app, MTY_Window window)
{
	return mty_window_get_gfx(app, window, NULL);
}

bool MTY_WindowSetGFX(MTY_App *app, MTY_Window window, MTY_GFX api, bool vsync)
{
	struct gfx_ctx *gfx_ctx = NULL;
	MTY_GFX wapi = mty_window_get_gfx(app, window, &gfx_ctx);

	if (wapi != MTY_GFX_NONE) {
		GFX_CTX_API[wapi].destroy(&gfx_ctx);
		mty_window_set_gfx(app, window, MTY_GFX_NONE, NULL);
	}

	void *native = mty_window_get_native(app, window);

	if (native && api != MTY_GFX_NONE) {
		gfx_ctx = GFX_CTX_API[api].create(native, vsync);

		// Fallback
		if (!gfx_ctx) {
			if (api == MTY_GFX_D3D11)
				return MTY_WindowSetGFX(app, window, MTY_GFX_D3D9, vsync);

			if (api == MTY_GFX_D3D9 || api == MTY_GFX_METAL)
				return MTY_WindowSetGFX(app, window, MTY_GFX_GL, vsync);

		} else {
			mty_window_set_gfx(app, window, api, gfx_ctx);
		}
	}

	return gfx_ctx ? true : false;
}
