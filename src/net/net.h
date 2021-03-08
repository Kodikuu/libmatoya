// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

struct net;

struct net *mty_net_connect(const char *host, uint16_t port, bool secure, uint32_t timeout);
struct net *mty_net_listen(const char *ip, uint16_t port);
struct net *mty_net_accept(struct net *ctx, uint32_t timeout);
void mty_net_destroy(struct net **net);

MTY_Async mty_net_poll(struct net *ctx, uint32_t timeout);
bool mty_net_write(struct net *ctx, const void *buf, size_t size);
bool mty_net_read(struct net *ctx, void *buf, size_t size, uint32_t timeout);

const char *mty_net_get_host(struct net *ctx);
