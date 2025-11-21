//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2025
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------
#include "memory.h"

//------------------------------------------------------------------------------
// OS_Linux
//------------------------------------------------------------------------------
#ifdef __gnu_linux__

#include <sys/mman.h>

// TODO: Do mmap/munmap need to be gaurded against EINTR as well?

void *alloc_memory(size_t size, uint32_t flags){
    void *result = NULL;

    void *memory = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS|MAP_PRIVATE|MAP_NORESERVE, -1, 0);
    if (memory != (void*)-1){
        result = memory;
    }
    else{
        /*log("Unable to allocate memory.\n");*/
    }

    return result;
}

void dealloc_memory(void *memory, size_t size){
    assert(memory);
    if(munmap(memory, size) == -1){
        /*log("Unable to free memory.\n");*/
    }
}

//------------------------------------------------------------------------------
// OS_Linux
//------------------------------------------------------------------------------
#endif
