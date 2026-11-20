//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------
#ifndef CBED_VULKAN
#define CBED_VULKAN

#include <vulkan/vulkan.h>
#include "common.h"

typedef struct{
    VkBuffer       handle;
    VkDeviceMemory memory;
    void          *mapped_ptr;
} Vulkan_Buffer;

//
// Platform-specific APIs
//
#ifdef OS_Linux

#include <X11/Xlib.h>
Cbed_API bool vulkan_init(Display *display, Window window, u32 window_w, u32 window_h, const char* app_name, bool debugging);

#endif // OS_Linux

#endif // CBED_VULKAN
