// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <winsock2.h>
#include <ws2tcpip.h>

#define SOCK_ERROR       WSAGetLastError()
#define SOCK_WOULD_BLOCK WSAEWOULDBLOCK
#define SOCK_IN_PROGRESS WSAEWOULDBLOCK

#define poll             WSAPoll
#define SHUT_RDWR        2

typedef int32_t socklen_t;

static bool sock_set_nonblocking(SOCKET s)
{
	u_long mode = 1;

	return ioctlsocket(s, FIONBIO, &mode) == 0;
}
