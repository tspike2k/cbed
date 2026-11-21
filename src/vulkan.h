//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------
#ifndef CBED_VULKAN
#define CBED_VULKAN

#include <vulkan/vulkan.h>
#include "common.h"

#ifdef OS_Linux
#  include <X11/Xlib.h>
#endif

#ifndef Constants_Buffer_Size
#  define Constants_Buffer_Size 1024
#endif

typedef struct{
    VkBuffer       handle;
    VkDeviceMemory memory;
    void          *mapped_ptr;
} Vulkan_Buffer;

typedef struct{
    const char* app_name;
    u32 window_width;
    u32 window_height;
    bool debug_mode;
    Buffer *memory;

#ifdef OS_Linux
    Display *xlib_display;
    Window   xlib_window;
#endif
} Vulkan_Init_Params;

Cbed_API bool vulkan_init(Vulkan_Init_Params *params);
Cbed_API void vulkan_destroy();
Cbed_API bool vulkan_create_buffer(u32 size, VkBufferUsageFlags usage, Vulkan_Buffer *buffer);

#endif // CBED_VULKAN
