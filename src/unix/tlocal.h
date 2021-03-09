// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#define MTY_TLOCAL __thread

void *mty_tlocal(size_t size);
char *mty_tlocal_strcpy(const char *str);
char *mty_tlocal_strcpyw(wchar_t *wstr);;
