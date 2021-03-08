// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <math.h>

#define COBJMACROS
#include <wincodec.h>
#include <shlwapi.h>

void *MTY_DecompressImage(const void *input, size_t size, uint32_t *width, uint32_t *height)
{
	void *rgba = NULL;
	IStream *stream = NULL;
	IWICImagingFactory *factory = NULL;
	IWICBitmapDecoder *decoder = NULL;
	IWICBitmapFrameDecode *frame = NULL;
	IWICBitmapSource *sframe = NULL;
	IWICBitmapSource *cframe = NULL;

	HRESULT e = CoCreateInstance(&CLSID_WICImagingFactory1, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, &factory);
	if (e != S_OK) {
		MTY_Log("'CoCreateInstance' failed with HRESULT 0x%X", e);
		goto except;
	}

	stream = SHCreateMemStream(input, (UINT) size);
	if (!stream) {
		MTY_Log("'SHCreateMemStream' failed");
		goto except;
	}

	e = IWICImagingFactory_CreateDecoderFromStream(factory, stream, NULL, WICDecodeMetadataCacheOnDemand, &decoder);
	if (e != S_OK) {
		MTY_Log("'IWICImagingFactory_CreateDecoderFromStream' failed with HRESULT 0x%X", e);
		goto except;
	}

	e = IWICBitmapDecoder_GetFrame(decoder, 0, &frame);
	if (e != S_OK) {
		MTY_Log("'IWICBitmapDecoder_GetFrame' failed with HRESULT 0x%X", e);
		goto except;
	}

	e = IWICBitmapFrameDecode_GetSize(frame, width, height);
	if (e != S_OK) {
		MTY_Log("'IWICBitmapFrameDecode_GetSize' failed with HRESULT 0x%X", e);
		goto except;
	}

	e = IWICBitmapFrameDecode_QueryInterface(frame, &IID_IWICBitmapSource, &sframe);
	if (e != S_OK) {
		MTY_Log("'IWICBitmapFrameDecode_QueryInterface' failed with HRESULT 0x%X", e);
		goto except;
	}

	IWICBitmapFrameDecode_Release(frame);
	frame = NULL;

	e = WICConvertBitmapSource(&GUID_WICPixelFormat32bppRGBA, sframe, &cframe);
	if (e != S_OK) {
		MTY_Log("'WICConvertBitmapSource' failed with HRESULT 0x%X", e);
		goto except;
	}

	IWICBitmapSource_Release(sframe);
	sframe = NULL;

	UINT stride = *width * 4;
	UINT rgba_size = stride * *height;
	rgba = MTY_Alloc(rgba_size, 1);

	e = IWICBitmapSource_CopyPixels(cframe, NULL, stride, rgba_size, rgba);
	if (e != S_OK) {
		MTY_Free(rgba);
		rgba = NULL;

		MTY_Log("'IWICBitmapSource_CopyPixels' failed with HRESULT 0x%X", e);
		goto except;
	}

	except:

	if (cframe)
		IWICBitmapSource_Release(cframe);

	if (sframe)
		IWICBitmapSource_Release(sframe);

	if (frame)
		IWICBitmapFrameDecode_Release(frame);

	if (decoder)
		IWICBitmapDecoder_Release(decoder);

	if (stream)
		IStream_Release(stream);

	if (factory)
		IWICImagingFactory_Release(factory);

	return rgba;
}

void *MTY_CompressImage(MTY_Image type, const void *input, uint32_t width, uint32_t height, size_t *outputSize)
{
	void *cmp = NULL;
	IWICImagingFactory *factory = NULL;
	IWICBitmapEncoder *encoder = NULL;
	IWICBitmapFrameEncode *frame = NULL;
	IStream *stream = NULL;

	HRESULT e = CoCreateInstance(&CLSID_WICImagingFactory1, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, &factory);
	if (e != S_OK) {
		MTY_Log("'CoCreateInstance' failed with HRESULT 0x%X", e);
		goto except;
	}

	GUID format = {0};
	switch (type) {
		case MTY_IMAGE_PNG: format = GUID_ContainerFormatPng;  break;
		case MTY_IMAGE_JPG: format = GUID_ContainerFormatJpeg; break;
		default:
			MTY_Log("MTY_Image type %d not supported", type);
			goto except;
	}

	e = IWICImagingFactory_CreateEncoder(factory, &format, NULL, &encoder);
	if (e != S_OK) {
		MTY_Log("'IWICImagingFactory_CreateEncoder' failed with HRESULT 0x%X", e);
		goto except;
	}

	e = CreateStreamOnHGlobal(NULL, TRUE, &stream);
	if (e != S_OK) {
		MTY_Log("'CreateStreamOnHGlobal' failed with HRESULT 0x%X", e);
		goto except;
	}

	e = IWICBitmapEncoder_Initialize(encoder, stream, WICBitmapEncoderNoCache);
	if (e != S_OK) {
		MTY_Log("'IWICBitmapEncoder_Initialize' failed with HRESULT 0x%X", e);
		goto except;
	}

	e = IWICBitmapEncoder_CreateNewFrame(encoder, &frame, NULL);
	if (e != S_OK) {
		MTY_Log("'IWICBitmapDecoder_CreateNewFrame' failed with HRESULT 0x%X", e);
		goto except;
	}

	e = IWICBitmapFrameEncode_Initialize(frame, NULL);
	if (e != S_OK) {
		MTY_Log("'IWICBitmapFrameEncode_Initialize' failed with HRESULT 0x%X", e);
		goto except;
	}

	e = IWICBitmapFrameEncode_SetSize(frame, width, height);
	if (e != S_OK) {
		MTY_Log("'IWICBitmapFrameEncode_SetSize' failed with HRESULT 0x%X", e);
		goto except;
	}

	WICPixelFormatGUID wic_format = GUID_WICPixelFormat32bppBGRA;
	e = IWICBitmapFrameEncode_SetPixelFormat(frame, &wic_format);
	if (e != S_OK) {
		MTY_Log("'IWICBitmapFrameEncode_SetPixelFormat' failed with HRESULT 0x%X", e);
		goto except;
	}

	if (memcmp(&wic_format, &GUID_WICPixelFormat32bppBGRA, sizeof(WICPixelFormatGUID))) {
		MTY_Log("GUID_WICPixelFormat32bppRGBA is not available for MTY_Image type %d", type);
		goto except;
	}

	UINT stride = width * 4;
	UINT rgba_size = stride * height;
	e = IWICBitmapFrameEncode_WritePixels(frame, height, stride, rgba_size, (BYTE *) input);
	if (e != S_OK) {
		MTY_Log("'IWICBitmapFrameEncode_WritePixels' failed with HRESULT 0x%X", e);
		goto except;
	}

	e = IWICBitmapFrameEncode_Commit(frame);
	if (e != S_OK) {
		MTY_Log("'IWICBitmapFrameEncode_Commit' failed with HRESULT 0x%X", e);
		goto except;
	}

	e = IWICBitmapEncoder_Commit(encoder);
	if (e != S_OK) {
		MTY_Log("'IWICBitmapEncoder_Commit' failed with HRESULT 0x%X", e);
		goto except;
	}

	HGLOBAL mem = NULL;
	e = GetHGlobalFromStream(stream, &mem);
	if (e != S_OK) {
		MTY_Log("'GetHGlobalFromStream' failed with HRESULT 0x%X", e);
		goto except;
	}

	const void *lmem = GlobalLock(mem);
	if (lmem) {
		*outputSize = GlobalSize(mem);
		cmp = MTY_Dup(lmem, *outputSize);

		GlobalUnlock(mem);

	} else {
		MTY_Log("'GlobalLock' failed with error 0x%X", GetLastError());
		goto except;
	}

	except:

	if (frame)
		IWICBitmapFrameEncode_Release(frame);

	if (stream)
		IStream_Release(stream);

	if (encoder)
		IWICBitmapEncoder_Release(encoder);

	if (factory)
		IWICImagingFactory_Release(factory);

	return cmp;
}
