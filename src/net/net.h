// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <stdint.h>
#include <stdbool.h>

struct mty_net;

enum mty_net_status {
	MTY_NET_OK          = 0,
	MTY_NET_ERR_DEFAULT = -1,
};

char *mty_net_get_proxy(void);

struct mty_net *mty_net_connect(const char *host, uint16_t port, bool secure, uint32_t timeout);
struct mty_net *mty_net_listen(const char *ip, uint16_t port);
struct mty_net *mty_net_accept(struct mty_net *ctx, bool secure, uint32_t timeout);
void mty_net_destroy(struct mty_net **net);

bool mty_net_write(struct mty_net *ctx, const void *buf, size_t size);
bool mty_net_read(struct mty_net *ctx, void *buf, size_t size, uint32_t timeout);
MTY_Async mty_net_poll(struct mty_net *ctx, uint32_t timeout);

const char *mty_net_get_host(struct mty_net *ctx);
