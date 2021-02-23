// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "matoya.h"

struct async_state {
	MTY_Async status;
	MTY_HttpAsyncFunc func;
	uint32_t timeout;

	struct {
		char *host;
		bool secure;
		char *method;
		char *path;
		char *headers;
		void *body;
		size_t body_size;
	} req;

	struct {
		uint16_t code;
		void *body;
		size_t body_size;
	} res;
};

static MTY_Atomic32 ASYNC_GLOCK;
static MTY_ThreadPool *CTX;

static void http_async_free_state(void *opaque)
{
	struct async_state *s = opaque;

	if (s) {
		MTY_Free(s->req.host);
		MTY_Free(s->req.method);
		MTY_Free(s->req.path);
		MTY_Free(s->req.headers);
		MTY_Free(s->res.body);
		MTY_Free(s);
	}
}

void MTY_HttpAsyncCreate(uint32_t maxThreads)
{
	MTY_GlobalLock(&ASYNC_GLOCK);

	if (!CTX)
		CTX = MTY_ThreadPoolCreate(maxThreads);

	MTY_GlobalUnlock(&ASYNC_GLOCK);
}

void MTY_HttpAsyncDestroy(void)
{
	MTY_GlobalLock(&ASYNC_GLOCK);

	MTY_ThreadPoolDestroy(&CTX, http_async_free_state);

	MTY_GlobalUnlock(&ASYNC_GLOCK);
}

static void http_async_thread(void *opaque)
{
	struct async_state *s = opaque;

	bool ok = MTY_HttpRequest(s->req.host, s->req.secure, s->req.method, s->req.path,
		s->req.headers, s->req.body, s->req.body_size, s->timeout,
		&s->res.body, &s->res.body_size, &s->res.code);

	if (ok && s->func && s->res.body && s->res.body_size > 0)
		s->func(s->res.code, &s->res.body, &s->res.body_size);

	s->status = !ok ? MTY_ASYNC_ERROR : MTY_ASYNC_OK;
}

void MTY_HttpAsyncRequest(uint32_t *index, const char *host, bool secure, const char *method,
	const char *path, const char *headers, const void *body, size_t size, uint32_t timeout,
	MTY_HttpAsyncFunc func)
{
	if (!CTX)
		return;

	if (*index != 0)
		MTY_ThreadPoolDetach(CTX, *index, http_async_free_state);

	struct async_state *s = MTY_Alloc(1, sizeof(struct async_state));
	s->timeout = timeout;
	s->func = func;

	s->req.secure = secure;
	s->req.body_size = size;
	s->req.method = MTY_Strdup(method);
	s->req.host = MTY_Strdup(host);
	s->req.path = MTY_Strdup(path);
	s->req.headers = headers ? MTY_Strdup(headers) : MTY_Alloc(1, 1);
	s->req.body = body ? MTY_Dup(body, size) : NULL;

	*index = MTY_ThreadPoolStart(CTX, http_async_thread, s);

	if (*index == 0) {
		MTY_Log("Failed to start %s%s", host, path);
		http_async_free_state(s);
	}
}

MTY_Async MTY_HttpAsyncPoll(uint32_t index, void **response, size_t *size, uint16_t *status)
{
	if (!CTX)
		return MTY_ASYNC_ERROR;

	if (index == 0)
		return MTY_ASYNC_DONE;

	struct async_state *s = NULL;
	MTY_Async r = MTY_ASYNC_DONE;
	MTY_ThreadState pstatus = MTY_ThreadPoolState(CTX, index, (void **) &s);

	if (pstatus == MTY_THREAD_STATE_DONE) {
		*response = s->res.body;
		*size = s->res.body_size;
		*status = s->res.code;

		r = s->status;

	} else if (pstatus == MTY_THREAD_STATE_RUNNING) {
		r = MTY_ASYNC_CONTINUE;
	}

	return r;
}

void MTY_HttpAsyncClear(uint32_t *index)
{
	if (!CTX)
		return;

	MTY_ThreadPoolDetach(CTX, *index, http_async_free_state);
	*index = 0;
}
