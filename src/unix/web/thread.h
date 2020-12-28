// Copyright (c) 2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

typedef int32_t pthread_t;

typedef struct pthread_mutex_t {
	uint8_t _;
} pthread_mutex_t;

typedef struct pthread_cond_t {
	uint8_t _;
} pthread_cond_t;

typedef struct pthread_rwlock_t {
	uint8_t _;
} pthread_rwlock_t;

typedef struct pthread_rwlockattr_t {
	uint8_t _;
} pthread_rwlockattr_t;

#define PTHREAD_MUTEX_INITIALIZER {0}
#define PTHREAD_COND_INITIALIZER {0}

#define pthread_create(t, attr, func, opaque) ((func)(opaque), 0)
#define pthread_join(t, ret) 0
#define pthread_detach(t) 0
#define pthread_self() 0

#define pthread_mutex_init(mutex, attr) ((void) (mutex), 0)
#define pthread_mutex_destroy(mutex) 0
#define pthread_mutex_lock(mutex) 0
#define pthread_mutex_unlock(mutex) 0
#define pthread_mutex_trylock(mutex) 0

#define pthread_cond_init(cond, attr) ((void) (cond), 0)
#define pthread_cond_destroy(cond) 0
#define pthread_cond_signal(cond) 0
#define pthread_cond_broadcast(cond) 0
#define pthread_cond_wait(cond, mutex) 0
#define pthread_cond_timedwait(cond, mutex, ts) 0

#define pthread_rwlockattr_init(attr) 0
#define pthread_rwlockattr_destroy(attr) 0
#define pthread_rwlock_init(rw, attr) 0
#define pthread_rwlock_destroy(rw) 0
#define pthread_rwlock_unlock(rw) 0
#define pthread_rwlock_lock(rw) 0
#define pthread_rwlock_rdlock(rw) 0
#define pthread_rwlock_wrlock(rw) 0
