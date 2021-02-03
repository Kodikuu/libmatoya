// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>
#include <math.h>

#include "aaudio-dl.h"

#define AUDIO_CHANNELS 2
#define AUDIO_BUF_SIZE (48000 * AUDIO_CHANNELS * 2 * 1) // 1 second at 48khz

struct MTY_Audio {
	AAudioStreamBuilder *builder;
	AAudioStream *stream;

	MTY_Mutex *mutex;
	uint32_t sample_rate;
	uint32_t min_buffer;
	uint32_t max_buffer;
	bool cb_stopped;
	bool flushing;

	uint8_t *buffer;
	size_t size;
};

static void audio_error(AAudioStream *stream, void *userData, aaudio_result_t error)
{
	MTY_Log("'AAudioStream' error %d", error);
}

static aaudio_data_callback_result_t audio_callback(AAudioStream *stream, void *userData,
	void *audioData, int32_t numFrames)
{
	MTY_Audio *ctx = userData;
	bool stop = false;

	MTY_MutexLock(ctx->mutex);

	size_t want_size = numFrames * AUDIO_CHANNELS * 2;

	if (ctx->size >= want_size) {
		memcpy(audioData, ctx->buffer, want_size);
		ctx->size -= want_size;

		memmove(ctx->buffer, ctx->buffer + want_size, ctx->size);

	} else {
		// Underrun
		ctx->size = 0;
		stop = true;
	}

	MTY_MutexUnlock(ctx->mutex);

	return stop ? AAUDIO_CALLBACK_RESULT_STOP : AAUDIO_CALLBACK_RESULT_CONTINUE;
}

MTY_Audio *MTY_AudioCreate(uint32_t sampleRate, uint32_t minBuffer, uint32_t maxBuffer)
{
	if (!aaudio_dl_global_init())
		return NULL;

	MTY_Audio *ctx = MTY_Alloc(1, sizeof(MTY_Audio));

	ctx->sample_rate = sampleRate;
	ctx->mutex = MTY_MutexCreate();
	ctx->buffer = MTY_Alloc(AUDIO_BUF_SIZE, 1);
	ctx->cb_stopped = true;

	uint32_t frames_per_ms = lrint((float) sampleRate / 1000.0f);
	ctx->min_buffer = minBuffer * frames_per_ms * AUDIO_CHANNELS * 2;
	ctx->max_buffer = maxBuffer * frames_per_ms * AUDIO_CHANNELS * 2;

	AAudio_createStreamBuilder(&ctx->builder);
	AAudioStreamBuilder_setDeviceId(ctx->builder, AAUDIO_UNSPECIFIED);
	AAudioStreamBuilder_setSampleRate(ctx->builder, sampleRate);
	AAudioStreamBuilder_setChannelCount(ctx->builder, AUDIO_CHANNELS);
	AAudioStreamBuilder_setFormat(ctx->builder, AAUDIO_FORMAT_PCM_I16);
	AAudioStreamBuilder_setPerformanceMode(ctx->builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
	AAudioStreamBuilder_setErrorCallback(ctx->builder, audio_error, ctx);
	AAudioStreamBuilder_setDataCallback(ctx->builder, audio_callback, ctx);
	AAudioStreamBuilder_setBufferCapacityInFrames(ctx->builder, sampleRate); // 1s

	AAudioStreamBuilder_openStream(ctx->builder, &ctx->stream);
	AAudioStream_setBufferSizeInFrames(ctx->stream, 0);

	return ctx;
}

uint32_t MTY_AudioGetQueuedMs(MTY_Audio *ctx)
{
	return (ctx->size / (AUDIO_CHANNELS * 2)) / ctx->sample_rate * 1000;
}

void MTY_AudioStop(MTY_Audio *ctx)
{
}

void MTY_AudioQueue(MTY_Audio *ctx, const int16_t *frames, uint32_t count)
{
	size_t data_size = count * AUDIO_CHANNELS * 2;

	MTY_MutexLock(ctx->mutex);

	if (ctx->size + data_size >= ctx->max_buffer)
		ctx->flushing = true;

	if (ctx->size == 0) {
		if (!ctx->cb_stopped)
			AAudioStream_requestStop(ctx->stream);

		ctx->cb_stopped = true;
		ctx->flushing = false;
	}

	if (!ctx->flushing && data_size + ctx->size < AUDIO_BUF_SIZE) {
		memcpy(ctx->buffer + ctx->size, frames, data_size);
		ctx->size += data_size;
	}

	if (ctx->cb_stopped && ctx->size >= ctx->min_buffer) {
		AAudioStream_requestStart(ctx->stream);
		ctx->cb_stopped = false;
	}

	MTY_MutexUnlock(ctx->mutex);
}

void MTY_AudioDestroy(MTY_Audio **audio)
{
	if (!audio || !*audio)
		return;

	MTY_Audio *ctx = *audio;

	if (ctx->stream) {
		AAudioStream_requestStop(ctx->stream);
		AAudioStream_close(ctx->stream);
	}

	if (ctx->builder)
		AAudioStreamBuilder_delete(ctx->builder);

	MTY_MutexDestroy(&ctx->mutex);
	MTY_Free(ctx->buffer);

	MTY_Free(ctx);
	*audio = NULL;
}
