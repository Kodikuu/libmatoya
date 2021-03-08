// Copyright (c) Christopher D. Dickson <cdd@matoya.group>
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#define MTY_DLOPEN_FLAGS 0

#define dlsym(so, name) NULL
#define dlerror() NULL
#define dlopen(name, flags) NULL
#define dlclose(so) -1
