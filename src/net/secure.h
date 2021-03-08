// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "tcp.h"

struct secure;

struct secure *mty_secure_connect(struct tcp *tcp, const char *host, uint32_t timeout);
void mty_secure_destroy(struct secure **secure);

bool mty_secure_write(struct secure *ctx, struct tcp *tcp, const void *buf, size_t size);
bool mty_secure_read(struct secure *ctx, struct tcp *tcp, void *buf, size_t size, uint32_t timeout);
