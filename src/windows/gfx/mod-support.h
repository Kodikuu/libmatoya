// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#define GFX_API_SUPPORTED(api) \
	((api) == MTY_GFX_GL || (api) == MTY_GFX_D3D9 || (api) == MTY_GFX_D3D11)

#define GFX_DECLARE_TABLE() \
	static const struct { \
		GFX_DECLARE_API(_, GFX_FP) \
	} GFX_API[] = { \
		GFX_DECLARE_ROW(GL, _gl_) \
		GFX_DECLARE_ROW(D3D9, _d3d9_) \
		GFX_DECLARE_ROW(D3D11, _d3d11_) \
	};

#define GFX_UI_DECLARE_TABLE() \
	static const struct { \
		GFX_UI_DECLARE_API(_, GFX_UI_FP) \
	} GFX_UI_API[] = { \
		GFX_UI_DECLARE_ROW(GL, _gl_) \
		GFX_UI_DECLARE_ROW(D3D9, _d3d9_) \
		GFX_UI_DECLARE_ROW(D3D11, _d3d11_) \
	};

#define GFX_CTX_DECLARE_TABLE() \
	static const struct { \
		GFX_CTX_DECLARE_API(_, GFX_CTX_FP) \
	} GFX_CTX_API[] = { \
		GFX_CTX_DECLARE_ROW(GL, _gl_) \
		GFX_CTX_DECLARE_ROW(D3D9, _d3d9_) \
		GFX_CTX_DECLARE_ROW(D3D11, _d3d11_) \
	};
