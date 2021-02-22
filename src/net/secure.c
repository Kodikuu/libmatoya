// Copyright (c) 2021 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "secure.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define SECURE_PADDING 1024

struct secure {
	TCP_SOCKET socket;
	MTY_TLS *tls;

	uint8_t *buf;
	size_t buf_size;

	uint8_t *pbuf;
	size_t pbuf_size;
	size_t pending;
};

void secure_destroy(struct secure **secure)
{
	if (!secure || !*secure)
		return;

	struct secure *ctx = *secure;

	MTY_Free(ctx->buf);
	MTY_Free(ctx->pbuf);

	MTY_TLSDestroy(&ctx->tls);

	MTY_Free(ctx);
	*secure = NULL;
}

static bool secure_read_message(struct secure *ctx, uint32_t timeout, size_t *size)
{
	// TLS first 5 bytes contain message type, version, and size
	if (!tcp_read(ctx->socket, ctx->buf, 5, timeout))
		return false;

	// Size is bytes 3-4 big endian
	uint16_t len = 0;
	memcpy(&len, ctx->buf + 3, 2);
	len = MTY_SwapFromBE16(len);

	// Grow buf to handle message
	*size = len + 5;
	if (*size > ctx->buf_size) {
		ctx->buf = MTY_Realloc(ctx->buf, *size, 1);
		ctx->buf_size = *size;
	}

	return tcp_read(ctx->socket, ctx->buf + 5, len, timeout);
}

static bool secure_write_callback(const void *buf, size_t size, void *opaque)
{
	struct secure *ctx = opaque;

	return tcp_write(ctx->socket, buf, size);
}

struct secure *secure_connect(TCP_SOCKET socket, const char *host, uint32_t timeout)
{
	bool r = true;

	struct secure *ctx = MTY_Alloc(1, sizeof(struct secure));
	ctx->socket = socket;

	ctx->buf = MTY_Alloc(SECURE_PADDING, 1);
	ctx->buf_size = SECURE_PADDING;

	// TLS context
	ctx->tls = MTY_TLSCreate(MTY_TLS_TYPE_TLS, NULL, host, NULL, 0);
	if (!ctx->tls) {
		r = false;
		goto except;
	}

	// Handshake part 1 (->Client Hello) -- Initiate with NULL message
	MTY_Async a = MTY_TLSHandshake(ctx->tls, NULL, 0, secure_write_callback, ctx);
	if (a != MTY_ASYNC_CONTINUE) {
		r = false;
		goto except;
	}

	// Handshake part 2 (<-Server Hello, ...) -- Loop until handshake complete
	while (true) {
		size_t size = 0;
		r = secure_read_message(ctx, timeout, &size);
		if (!r)
			break;

		a = MTY_TLSHandshake(ctx->tls, ctx->buf, size, secure_write_callback, ctx);
		if (a == MTY_ASYNC_OK) {
			break;

		} else if (a == MTY_ASYNC_ERROR) {
			r = false;
			break;
		}
	}

	except:

	if (!r)
		secure_destroy(&ctx);

	return ctx;
}

bool secure_write(struct secure *ctx, const void *buf, size_t size)
{
	// Output buffer will be slightly larger than input
	if (ctx->buf_size < size + SECURE_PADDING) {
		ctx->buf = MTY_Realloc(ctx->buf, size + SECURE_PADDING, 1);
		ctx->buf_size = size + SECURE_PADDING;
	}

	// Encrypt
	size_t written = 0;
	if (!MTY_TLSEncrypt(ctx->tls, buf, size, ctx->buf, ctx->buf_size, &written))
		return false;

	// Write resulting encrypted message via TCP
	return tcp_write(ctx->socket, ctx->buf, written);
}

bool secure_read(struct secure *ctx, void *buf, size_t size, uint32_t timeout)
{
	while (true) {
		// Check if we have decrypted data in our buffer from a previous read
		if (ctx->pending >= size) {
			memcpy(buf, ctx->pbuf, size);
			ctx->pending -= size;

			memmove(ctx->pbuf, ctx->pbuf + size, ctx->pending);

			return true;
		}

		// We need more data, read a TLS message from the socket
		size_t msg_size = 0;
		if (!secure_read_message(ctx, timeout, &msg_size))
			break;

		// Decrypt
		size_t read = 0;
		if (!MTY_TLSDecrypt(ctx->tls, ctx->buf, msg_size, ctx->buf, ctx->buf_size, &read))
			break;

		// Append any decrypted data to pbuf (pending)
		if (ctx->pending + read > ctx->pbuf_size) {
			ctx->pbuf_size = ctx->pending + read;
			ctx->pbuf = MTY_Realloc(ctx->pbuf, ctx->pbuf_size, 1);
		}

		memcpy(ctx->pbuf + ctx->pending, ctx->buf, read);
		ctx->pending += read;
	}

	return false;
}