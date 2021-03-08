// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "matoya.h"

struct tcp;

struct tcp *mty_tcp_connect(const char *ip, uint16_t port, uint32_t timeout);
struct tcp *mty_tcp_listen(const char *ip, uint16_t port);
struct tcp *mty_tcp_accept(struct tcp *ctx, uint32_t timeout);
void mty_tcp_destroy(struct tcp **tcp);

MTY_Async mty_tcp_poll(struct tcp *ctx, bool out, uint32_t timeout);
bool mty_tcp_write(struct tcp *ctx, const void *buf, size_t size);
bool mty_tcp_read(struct tcp *ctx, void *buf, size_t size, uint32_t timeout);

bool mty_dns_query(const char *host, char *ip, size_t size);
