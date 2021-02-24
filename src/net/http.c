// Copyright (c) 2017-2020 Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include "matoya.h"
#include "http.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "tlocal.h"


// Standard header generators

static const char HTTP_REQUEST_FMT[] =
	"%s %s HTTP/1.1\r\n"
	"Host: %s\r\n"
	"%s"
	"\r\n";

static const char HTTP_CONNECT_FMT[] =
	"CONNECT %s:%u HTTP/1.1\r\n"
	"%s"
	"\r\n";

static const char HTTP_RESPONSE_FMT[] =
	"HTTP/1.1 %s %s\r\n"
	"%s"
	"\r\n";

static char *http_request(const char *method, const char *host, const char *path, const char *fields)
{
	if (!fields)
		fields = "";

	size_t size = snprintf(NULL, 0, HTTP_REQUEST_FMT, method, path, host, fields) + 1;
	char *str = MTY_Alloc(size, 1);

	snprintf(str, size, HTTP_REQUEST_FMT, method, path, host, fields);

	return str;
}

static char *http_connect(const char *host, uint16_t port, const char *fields)
{
	if (!fields)
		fields = "";

	size_t size = snprintf(NULL, 0, HTTP_CONNECT_FMT, host, port, fields) + 1;
	char *str = MTY_Alloc(size, 1);

	snprintf(str, size, HTTP_CONNECT_FMT, host, port, fields);

	return str;
}

static char *http_response(const char *code, const char *msg, const char *fields)
{
	if (!fields)
		fields = "";

	size_t size = snprintf(NULL, 0, HTTP_RESPONSE_FMT, code, msg, fields) + 1;
	char *str = MTY_Alloc(size, 1);

	snprintf(str, size, HTTP_RESPONSE_FMT, code, msg, fields);

	return str;
}


// Header parsing, construction

struct http_pair {
	char *key;
	char *val;
};

struct http_header {
	char *first_line;
	struct http_pair *pairs;
	uint32_t npairs;
};

struct http_header *mty_http_parse_header(const char *header)
{
	struct http_header *h = MTY_Alloc(1, sizeof(struct http_header));
	char *dup = MTY_Strdup(header);

	// HTTP header lines are delimited by "\r\n"
	char *ptr = NULL;
	char *line = MTY_Strtok(dup, "\r\n", &ptr);

	for (bool first = true; line; first = false) {

		// First line is special and is stored seperately
		if (first) {
			h->first_line = MTY_Strdup(line);

		// All lines following the first are in the "key: val" format
		} else {
			char *delim = strpbrk(line, ": ");

			if (delim) {
				h->pairs = MTY_Realloc(h->pairs, h->npairs + 1, sizeof(struct http_pair));

				// Place a null character to separate the line
				char save = delim[0];
				delim[0] = '\0';

				// Save the key and remove the null character
				h->pairs[h->npairs].key = MTY_Strdup(line);
				delim[0] = save;

				// Advance the val past whitespace or the : character
				while (*delim && (*delim == ':' || *delim == ' '))
					delim++;

				// Store the val and increment npairs
				h->pairs[h->npairs].val = MTY_Strdup(delim);
				h->npairs++;
			}
		}

		line = MTY_Strtok(NULL, "\r\n", &ptr);
	}

	MTY_Free(dup);

	return h;
}

void mty_http_header_destroy(struct http_header **header)
{
	if (!header || !*header)
		return;

	struct http_header *h = *header;

	for (uint32_t x = 0; x < h->npairs; x++) {
		MTY_Free(h->pairs[x].key);
		MTY_Free(h->pairs[x].val);
	}

	MTY_Free(h->first_line);
	MTY_Free(h->pairs);

	MTY_Free(h);
	*header = NULL;
}

bool mty_http_get_status_code(struct http_header *h, uint16_t *status_code)
{
	bool r = true;
	char *tmp = MTY_Strdup(h->first_line);

	char *ptr = NULL;
	char *tok = MTY_Strtok(tmp, " ", &ptr);
	if (!tok) {
		r = false;
		goto except;
	}

	tok = MTY_Strtok(NULL, " ", &ptr);
	if (!tok) {
		r = false;
		goto except;
	}

	*status_code = (uint16_t) strtol(tok, NULL, 10);

	except:

	MTY_Free(tmp);

	return r;
}

bool mty_http_get_header_int(struct http_header *h, const char *key, int32_t *val)
{
	for (uint32_t x = 0; x < h->npairs; x++) {
		if (!MTY_Strcasecmp(key, h->pairs[x].key)) {
			char *endptr = h->pairs[x].val;
			*val = strtol(h->pairs[x].val, &endptr, 10);

			return endptr != h->pairs[x].val;
		}
	}

	return false;
}

bool mty_http_get_header_str(struct http_header *h, const char *key, const char **val)
{
	for (uint32_t x = 0; x < h->npairs; x++) {
		if (!MTY_Strcasecmp(key, h->pairs[x].key)) {
			*val = h->pairs[x].val;
			return true;
		}
	}

	return false;
}

char *mty_http_set_header_int(char *header, const char *name, int32_t val)
{
	size_t len = header ? strlen(header) : 0;
	size_t new_len = len + strlen(name) + 2 + 10 + 3;

	header = MTY_Realloc(header, new_len, 1);
	snprintf(header + len, new_len, "%s: %d\r\n", name, val);

	return header;
}

char *mty_http_set_header_str(char *header, const char *name, const char *val)
{
	size_t len = header ? strlen(header) : 0;
	size_t new_len = len + strlen(name) + 2 + strlen(val) + 3;

	header = MTY_Realloc(header, new_len, 1);
	snprintf(header + len, new_len, "%s: %s\r\n", name, val);

	return header;
}

char *mty_http_set_all_headers(char *header, const char *all)
{
	char *dup = MTY_Strdup(all);

	char *ptr = NULL;
	char *tok = MTY_Strtok(dup, "\n", &ptr);

	while (tok) {
		char *ptr2 = NULL;
		char *key = MTY_Strtok(tok, " :", &ptr2);
		char *val = key ? MTY_Strtok(NULL, "", &ptr2) : NULL;

		if (!key || !val)
			break;

		header = mty_http_set_header_str(header, key, val);

		tok = MTY_Strtok(NULL, "\n", &ptr);
	}

	MTY_Free(dup);

	return header;
}


// Wrapped header IO

#define HTTP_HEADER_MAX (16 * 1024)

static char *http_read_header_io(struct net *net, uint32_t timeout)
{
	bool r = false;
	char *h = MTY_Alloc(HTTP_HEADER_MAX, 1);

	for (uint32_t x = 0; x < HTTP_HEADER_MAX - 1; x++) {
		if (!mty_net_read(net, h + x, 1, timeout))
			break;

		if (x > 2 && h[x - 3] == '\r' && h[x - 2] == '\n' && h[x - 1] == '\r' && h[x] == '\n') {
			r = true;
			break;
		}
	}

	if (!r) {
		MTY_Free(h);
		h = NULL;
	}

	return h;
}

struct http_header *mty_http_read_header(struct net *net, uint32_t timeout)
{
	char *header = http_read_header_io(net, timeout);
	if (!header)
		return NULL;

	struct http_header *h = mty_http_parse_header(header);
	MTY_Free(header);

	return h;
}

bool mty_http_write_response_header(struct net *net, const char *code, const char *reason, const char *headers)
{
	char *hstr = http_response(code, reason, headers);
	bool r = mty_net_write(net, hstr, strlen(hstr));

	MTY_Free(hstr);

	return r;
}

bool mty_http_write_request_header(struct net *net, const char *method, const char *path, const char *headers)
{
	char *hstr = http_request(method, mty_net_get_host(net), path, headers);
	bool r = mty_net_write(net, hstr, strlen(hstr));

	MTY_Free(hstr);

	return r;
}


// HTTP Proxy IO and global settings

static MTY_Atomic32 HTTP_GLOCK;
static char HTTP_PROXY[MTY_URL_MAX];
static MTY_TLOCAL char HTTP_PROXY_TLS[MTY_URL_MAX];

bool MTY_HttpSetProxy(const char *proxy)
{
	MTY_GlobalLock(&HTTP_GLOCK);

	bool was_set = HTTP_PROXY[0];

	if (proxy) {
		if (!was_set) {
			snprintf(HTTP_PROXY, MTY_URL_MAX, "%s", proxy);
			was_set = true;
		}

	} else {
		HTTP_PROXY[0] = '\0';
	}

	MTY_GlobalUnlock(&HTTP_GLOCK);

	return was_set;
}

const char *mty_http_get_proxy(void)
{
	const char *proxy = NULL;

	MTY_GlobalLock(&HTTP_GLOCK);

	if (HTTP_PROXY[0]) {
		snprintf(HTTP_PROXY_TLS, MTY_URL_MAX, "%s", HTTP_PROXY);
		proxy = HTTP_PROXY_TLS;
	}

	MTY_GlobalUnlock(&HTTP_GLOCK);

	return proxy;
}

bool mty_http_should_proxy(const char **host, uint16_t *port)
{
	const char *proxy = mty_http_get_proxy();
	if (!proxy)
		return false;

	bool r = false;
	bool psecure = false;
	char *phost = NULL;
	char *ppath = NULL;

	if (mty_http_parse_url(proxy, &psecure, &phost, port, &ppath)) {
		if (phost && phost[0]) {
			snprintf(HTTP_PROXY_TLS, MTY_URL_MAX, "%s", phost);
			*host = HTTP_PROXY_TLS;
			r = true;
		}

		MTY_Free(phost);
		MTY_Free(ppath);
	}

	return r;
}

bool mty_http_proxy_connect(struct net *net, uint16_t port, uint32_t timeout)
{
	struct http_header *hdr = NULL;
	char *h = http_connect(mty_net_get_host(net), port, NULL);

	//write the header to the HTTP client/server
	bool r = mty_net_write(net, h, strlen(h));
	MTY_Free(h);

	if (!r)
		goto except;

	//read the response header
	hdr = mty_http_read_header(net, timeout);
	if (!hdr) {
		r = false;
		goto except;
	}

	//get the status code
	uint16_t status_code = 0;
	r = mty_http_get_status_code(hdr, &status_code);
	if (!r)
		goto except;

	if (status_code != 200) {
		r = false;
		goto except;
	}

	except:

	if (hdr)
		mty_http_header_destroy(&hdr);

	return r;
}


// URL utilities

bool mty_http_parse_url(const char *url, bool *secure, char **host, uint16_t *port, char **path)
{
	if (!url || !url[0])
		return false;

	bool r = true;
	char *dup = MTY_Strdup(url);

	*host = NULL;
	*path = NULL;
	*secure = false;
	*port = 0;

	// Scheme
	char *tok, *ptr = NULL;

	if (strstr(dup, "http") || strstr(dup, "ws")) {
		tok = MTY_Strtok(dup, ":", &ptr);
		if (!tok) {
			r = false;
			goto except;
		}

		if (!MTY_Strcasecmp(tok, "https") || !MTY_Strcasecmp(tok, "wss")) {
			*secure = true;

		} else if (!MTY_Strcasecmp(tok, "http") || !MTY_Strcasecmp(tok, "ws")) {
			*secure = false;

		} else {
			r = false;
			goto except;
		}

		// Host + port
		tok = MTY_Strtok(NULL, "/", &ptr);
		if (!tok) {
			r = false;
			goto except;
		}

	// No scheme, assume host:port/path syntax
	} else {
		*secure = false;
		tok = MTY_Strtok(dup, "/", &ptr);
	}

	if (!tok) {
		r = false;
		goto except;
	}

	// Try to find a port
	*host = MTY_Strdup(tok);
	if (!*host) {
		r = false;
		goto except;
	}

	char *tok2, *ptr2 = NULL;
	tok2 = MTY_Strtok(*host, ":", &ptr2);
	if (tok2)
		tok2 = MTY_Strtok(NULL, ":", &ptr2);

	if (tok2) { // We have a port
		*port = (uint16_t) atoi(tok2);
	} else {
		*port = *secure ? HTTP_PORT_S : HTTP_PORT;
	}

	// Path
	tok = MTY_Strtok(NULL, "", &ptr);
	if (!tok) tok = "";

	size_t path_len = strlen(tok) + 2;
	*path = MTY_Alloc(strlen(tok) + 2, 1);
	snprintf(*path, path_len, "/%s", tok);

	except:

	MTY_Free(dup);

	return r;
}

bool MTY_HttpParseUrl(const char *url, char *host, size_t hostSize, char *path, size_t pathSize)
{
	bool r = false;

	bool secure = false;
	uint16_t port = 0;
	char *phost = NULL;
	char *ppath = NULL;

	if (mty_http_parse_url(url, &secure, &phost, &port, &ppath)) {
		snprintf(host, hostSize, "%s", phost);
		snprintf(path, pathSize, "%s", ppath);
		r = true;
	}

	MTY_Free(phost);
	MTY_Free(ppath);

	return r;
}

void MTY_HttpEncodeUrl(const char *src, char *dst, size_t size)
{
	char table[256];
	dst[0] = '\0';

	for (int32_t i = 0; i < 256; i++)
		table[i] = (char) (isalnum(i) || i == '*' || i == '-' || i == '.' || i == '_' ?
			i : (i == ' ') ? '+' : 0);

	for (size_t x = 0; x < strlen(src); x++){
		int32_t c = src[x];
		int32_t inc = 1;

		if (table[c]) {
			snprintf(dst, size, "%c", table[c]);

		} else {
			snprintf(dst, size, "%%%02X", c);
			inc = 3;
		}

		dst += inc;
		size -= inc;
	}
}
