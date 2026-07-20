//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#ifndef CBED_OS_H
#define CBED_OS_H

#include "common.h"

Cbed_API void os_sleep_ns(u64 nanoseconds);
Cbed_API u64  os_timestamp_ns();

#endif // CBED_OS_H
