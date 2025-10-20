//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

#ifdef __gnu_linux
//------------------------------------------------------------------------------
// Linux
//------------------------------------------------------------------------------

#include <time.h>

void os_sleep_ns(uint64_t nanoseconds){
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

uint64_t os_timestamp_ns(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    uint64_t result = ts.tv_sec * 1000000000 + ts.tv_nsec;
    return result;
}

//------------------------------------------------------------------------------
// Linux
//------------------------------------------------------------------------------
#endif
