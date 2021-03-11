// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"

#include <string.h>

#include "rwlock.h"
#include "tlocal.h"

struct MTY_Sync {
	bool signal;
	MTY_Mutex *mutex;
	MTY_Cond *cond;
};

MTY_Sync *MTY_SyncCreate(void)
{
	MTY_Sync *ctx = MTY_Alloc(1, sizeof(struct MTY_Sync));

	ctx->mutex = MTY_MutexCreate();
	ctx->cond = MTY_CondCreate();

	return ctx;
}

bool MTY_SyncWait(MTY_Sync *ctx, int32_t timeout)
{
	MTY_MutexLock(ctx->mutex);

	if (!ctx->signal)
		MTY_CondWait(ctx->cond, ctx->mutex, timeout);

	bool signal = ctx->signal;
	ctx->signal = false;

	MTY_MutexUnlock(ctx->mutex);

	return signal;
}

void MTY_SyncWake(MTY_Sync *ctx)
{
	MTY_MutexLock(ctx->mutex);

	if (!ctx->signal) {
		ctx->signal = true;
		MTY_CondWake(ctx->cond);
	}

	MTY_MutexUnlock(ctx->mutex);
}

void MTY_SyncDestroy(MTY_Sync **sync)
{
	if (!sync || !*sync)
		return;

	MTY_Sync *ctx = *sync;

	MTY_CondDestroy(&ctx->cond);
	MTY_MutexDestroy(&ctx->mutex);

	MTY_Free(ctx);
	*sync = NULL;
}


// ThreadPool

struct thread_info {
	MTY_ThreadState status;
	MTY_AnonFunc func;
	MTY_AnonFunc detach;
	void *opaque;
	MTY_Thread *t;
	MTY_Mutex *m;
};

struct MTY_ThreadPool {
	uint32_t num;
	struct thread_info *ti;
};

MTY_ThreadPool *MTY_ThreadPoolCreate(uint32_t maxThreads)
{
	MTY_ThreadPool *ctx = MTY_Alloc(1, sizeof(MTY_ThreadPool));

	ctx->num = maxThreads + 1;
	ctx->ti = MTY_Alloc(ctx->num, sizeof(struct thread_info));

	for (uint32_t x = 0; x < ctx->num; x++)
		ctx->ti[x].m = MTY_MutexCreate();

	return ctx;
}

static void *thread_pool_func(void *opaque)
{
	struct thread_info *ti = (struct thread_info *) opaque;

	ti->func(ti->opaque);

	MTY_MutexLock(ti->m);

	if (ti->detach) {
		ti->detach(ti->opaque);
		ti->status = MTY_THREAD_STATE_DETACHED;

	} else {
		ti->status = MTY_THREAD_STATE_DONE;
	}

	MTY_MutexUnlock(ti->m);

	return NULL;
}

uint32_t MTY_ThreadPoolStart(MTY_ThreadPool *ctx, MTY_AnonFunc func, void *opaque)
{
	uint32_t index = 0;

	for (uint32_t x = 1; x < ctx->num && index == 0; x++) {
		struct thread_info *ti = &ctx->ti[x];

		MTY_MutexLock(ti->m);

		if (ti->status == MTY_THREAD_STATE_DETACHED) {
			MTY_ThreadDestroy(&ti->t);
			ti->status = MTY_THREAD_STATE_EMPTY;
		}

		if (ti->status == MTY_THREAD_STATE_EMPTY) {
			ti->func = func;
			ti->opaque = opaque;
			ti->detach = NULL;
			ti->status = MTY_THREAD_STATE_RUNNING;
			ti->t = MTY_ThreadCreate(thread_pool_func, ti);
			index = x;
		}

		MTY_MutexUnlock(ti->m);
	}

	if (index == 0)
		MTY_Log("Could not find available index");

	return index;
}

void MTY_ThreadPoolDetach(MTY_ThreadPool *ctx, uint32_t index, MTY_AnonFunc detach)
{
	struct thread_info *ti = &ctx->ti[index];

	MTY_MutexLock(ti->m);

	if (ti->status == MTY_THREAD_STATE_RUNNING) {
		ti->detach = detach;

	} else if (ti->status == MTY_THREAD_STATE_DONE) {
		if (detach)
			detach(ti->opaque);

		ti->status = MTY_THREAD_STATE_DETACHED;
	}

	MTY_MutexUnlock(ti->m);
}

MTY_ThreadState MTY_ThreadPoolState(MTY_ThreadPool *ctx, uint32_t index, void **opaque)
{
	struct thread_info *ti = &ctx->ti[index];

	MTY_MutexLock(ti->m);

	MTY_ThreadState status = ti->status;
	*opaque = ti->opaque;

	MTY_MutexUnlock(ti->m);

	return status;
}

void MTY_ThreadPoolDestroy(MTY_ThreadPool **pool, MTY_AnonFunc detach)
{
	if (!pool || !*pool)
		return;

	MTY_ThreadPool *ctx = *pool;

	for (uint32_t x = 0; x < ctx->num; x++) {
		MTY_ThreadPoolDetach(ctx, x, detach);

		if (ctx->ti[x].status != MTY_THREAD_STATE_EMPTY)
			MTY_ThreadDestroy(&ctx->ti[x].t);

		MTY_MutexDestroy(&ctx->ti[x].m);
	}

	MTY_Free(ctx->ti);
	MTY_Free(ctx);
	*pool = NULL;
}


// RWLock

struct MTY_RWLock {
	mty_rwlock rwlock;
	MTY_Atomic32 yield;
	uint8_t index;
};

static MTY_TLOCAL struct rwlock_state {
	uint16_t taken;
	bool read;
	bool write;
} RWLOCK_STATE[UINT8_MAX];

static MTY_Atomic32 RWLOCK_INIT[UINT8_MAX];

static uint8_t rwlock_index(void)
{
	for (uint8_t x = 0; x < UINT8_MAX; x++)
		if (MTY_Atomic32CAS(&RWLOCK_INIT[x], 0, 1))
			return x;

	MTY_Fatal("Could not find a free rwlock slot, maximum is %u", UINT8_MAX);

	return 0;
}

static void rwlock_yield(MTY_RWLock *ctx)
{
	// Ensure that readers will yield to writers in a tight loop
	while (MTY_Atomic32Get(&ctx->yield) > 0)
		MTY_Sleep(0);
}

MTY_RWLock *MTY_RWLockCreate(void)
{
	MTY_RWLock *ctx = MTY_Alloc(1, sizeof(MTY_RWLock));
	ctx->index = rwlock_index();

	mty_rwlock_create(&ctx->rwlock);

	return ctx;
}

void MTY_RWLockReader(MTY_RWLock *ctx)
{
	struct rwlock_state *rw = &RWLOCK_STATE[ctx->index];

	if (rw->taken == 0) {
		rwlock_yield(ctx);
		mty_rwlock_reader(&ctx->rwlock);
		rw->read = true;
	}

	rw->taken++;
}

bool MTY_RWTryLockReader(MTY_RWLock *ctx)
{
	struct rwlock_state *rw = &RWLOCK_STATE[ctx->index];

	bool r = true;

	if (rw->taken == 0) {
		rwlock_yield(ctx);
		r = mty_rwlock_try_reader(&ctx->rwlock);
		rw->read = true;
	}

	if (r)
		rw->taken++;

	return r;
}

void MTY_RWLockWriter(MTY_RWLock *ctx)
{
	bool relock = false;
	struct rwlock_state *rw = &RWLOCK_STATE[ctx->index];

	if (rw->read) {
		mty_rwlock_unlock_reader(&ctx->rwlock);
		rw->read = false;
		relock = true;
	}

	if (rw->taken == 0 || relock) {
		MTY_Atomic32Add(&ctx->yield, 1);
		mty_rwlock_writer(&ctx->rwlock);
		MTY_Atomic32Add(&ctx->yield, -1);
		rw->write = true;
	}

	rw->taken++;
}

void MTY_RWLockUnlock(MTY_RWLock *ctx)
{
	struct rwlock_state *rw = &RWLOCK_STATE[ctx->index];

	if (--rw->taken == 0) {
		if (rw->read) {
			mty_rwlock_unlock_reader(&ctx->rwlock);
			rw->read = false;

		} else if (rw->write) {
			mty_rwlock_unlock_writer(&ctx->rwlock);
			rw->write = false;
		}
	}
}

void MTY_RWLockDestroy(MTY_RWLock **rwlock)
{
	if (!rwlock || !*rwlock)
		return;

	MTY_RWLock *ctx = *rwlock;

	mty_rwlock_destroy(&ctx->rwlock);
	memset(&RWLOCK_STATE[ctx->index], 0, sizeof(struct rwlock_state));
	MTY_Atomic32Set(&RWLOCK_INIT[ctx->index], 0);

	MTY_Free(ctx);
	*rwlock = NULL;
}


// Global locks

static MTY_Atomic32 THREAD_GINDEX = {1};
static mty_rwlock THREAD_GLOCKS[UINT8_MAX];

void MTY_GlobalLock(MTY_Atomic32 *lock)
{
	entry: {
		uint32_t index = MTY_Atomic32Get(lock);

		// 0 means uninitialized, 1 means currently initializing
		if (index < 2) {
			if (MTY_Atomic32CAS(lock, 0, 1)) {
				index = MTY_Atomic32Add(&THREAD_GINDEX, 1);
				if (index >= UINT8_MAX)
					MTY_Fatal("Global lock index of %u exceeded", UINT8_MAX);

				mty_rwlock_create(&THREAD_GLOCKS[index]);
				MTY_Atomic32Set(lock, index);

			} else {
				MTY_Sleep(0);
				goto entry;
			}
		}

		mty_rwlock_writer(&THREAD_GLOCKS[index]);
	}
}

void MTY_GlobalUnlock(MTY_Atomic32 *lock)
{
	mty_rwlock_unlock_writer(&THREAD_GLOCKS[MTY_Atomic32Get(lock)]);
}
