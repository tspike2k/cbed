//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#include "os.h"

#ifdef OS_Linux
//------------------------------------------------------------------------------
// Linux
//------------------------------------------------------------------------------

#include <time.h>

Ceabed_API void os_sleep_ns(u64 nanoseconds){
    if(nanoseconds > 0){
        struct timespec ts;
        ts.tv_sec  = nanoseconds / 1000000000;
        ts.tv_nsec = nanoseconds % 1000000000;
        // NOTE: nanosleep can fail when a signal is raised. If this happens it returns -1.
        // In that case we try the function again.
        while(nanosleep(&ts, NULL) == -1){

        }
    }
}

Ceabed_API u64 os_timestamp_ns(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    u64 result = ts.tv_sec * 1000000000 + ts.tv_nsec;
    return result;
}

//------------------------------------------------------------------------------
// Linux
//------------------------------------------------------------------------------
#endif
