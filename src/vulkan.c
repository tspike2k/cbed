//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

/*
    NOTE: This is based off the following resources:

    https://www.howtovulkan.com/
    https://vulkan-tutorial.com/
    "Learn Modern Vulkan in 2 Hours (Dynamic Rendering, No Render Passes!)" by constref (https://www.youtube.com/watch?v=DC9FBRQKNck)
*/

#include "vulkan.h"
#include <vulkan/vulkan.h>

typedef struct{
    VkInstance   instance;
    VkSurfaceKHR surface;
} Vulkan_State;

static Vulkan_State vulkan__state;

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan__debug_msg_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT severity,
    VkDebugUtilsMessageTypeFlagBitsEXT type,
    const VkDebugUtilsMessengerCallbackDataEXT *callback_data,
    void *user_data
){
    fmt_msg_puts(callback_data->pMessage);
    fmt_msg_puts("\n");

    return VK_FALSE;
}

static bool vulkan__create_instance(const char *app_name, const char *extensions[], u32 extensions_count, bool debug_mode){
    Vulkan_State *s = &vulkan__state;

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = app_name,
        .apiVersion = VK_API_VERSION_1_4
    };

    const char *layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };
    u32 layers_count = Array_Len(layers);

    VkDebugUtilsMessengerCreateInfoEXT debug_info = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT,
        .pfnUserCallback = vulkan__debug_msg_callback,
    };

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &app_info,
        .enabledExtensionCount   = extensions_count,
        .ppEnabledExtensionNames = extensions,
    };

    if(debug_mode){
        instance_info.pNext               = &debug_info;
        instance_info.enabledLayerCount   = layers_count;
        instance_info.ppEnabledLayerNames = layers;
    }

    if(vkCreateInstance(&instance_info, NULL, &s->instance) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to create Vulkan instance.\n");
        return false;
    }

    //
    // Find the physical device.
    //
    // TODO: Allow player to configure which device to prefer. By index? By name?

    // NOTE: We have to cal this function once to get the count, then another time
    // once we have the memory for it.
    u32 device_count = 0;
    VkPhysicalDevice device = NULL;

    vkEnumeratePhysicalDevices(s->instance, &device_count, NULL);
    if(device_count){
        VkPhysicalDevice devices[device_count];
        vkEnumeratePhysicalDevices(s->instance, &device_count, devices);

        for_count(u32, i, device_count){
            // TODO: Is there a good rule of thumb for which device to choose?
            VkPhysicalDeviceProperties props;
            vkGetPhysicalDeviceProperties(devices[i], &props);
            if(props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
                const char *device_name = props.deviceName;
                fmt_msg("Target Vulkan device[{0}]: {1}\n", fmt_i(i), fmt_cstr(device_name));
                device = devices[i];
                break;
            }
        }
    }

    if(!device){
        fmt_msg_puts("ERROR: Unable to get physical Vulkan device.\n");
        return false;
    }

    //
    // Get the graphics queue.
    //
#if 0
    u32 queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(device, &queue_families_count, NULL);

    if(queue_families_count){
        VkQueueFamilyProperties2 queue_famiy_props[queue_families_count];
        vkGetPhysicalDeviceQueueFamilyProperties2(device, &queue_families_count, queue_famiy_props);

        for_count(u32, i, queue_families_count){
            VkBool32 can_present = VK_FALSE;

        }
    }
#endif


    return true;
}

////
//
// Linux
//
////
#ifdef OS_Linux

#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>

Cbed_API bool vulkan_init(Display *display, Window window, const char* app_name, bool debugging){
    Vulkan_State *s = &vulkan__state;

    bool result = true;

    const char* extensions[] ={
        "VK_KHR_surface",
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    u32 extensions_count = Array_Len(extensions);
    if(!debugging){
        // If we're not debugging, trim off the debug extension string.
        extensions_count--;
    }

    if(!vulkan__create_instance(app_name, extensions, extensions_count, debugging)){
        // TODO: Report error?
        return false;
    }

    VkXlibSurfaceCreateInfoKHR surface_info = {
        .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
        // .flags = ???, // TODO: What flags? Do we need to set flags?
        .dpy = display,
        .window = window,
    };

    if(vkCreateXlibSurfaceKHR(s->instance, &surface_info, NULL, &s->surface) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to create Vulkan surface.\n");
        return false;
    }


#if 0


    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(devices[device_index], &queue_family_count, NULL);
    VkQueueFamilyProperties families[queue_family_count];
    vkGetPhysicalDeviceQueueFamilyProperties(devices[decive_index], &queue_family_count, queueFamilies);
    u32 family_index = 0;
    for_count(u32, i, queue_family_count){
        if(families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            family_index = i;
            break;
        }
    }

    // TODO: Verify the family can, in fact, present graphics to the surface.

    float priority[queue_family_count] = {1.0f};
    VkDeviceQueueCreateInfo queue_info{
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = family_index,
        .queueCount = 1,
        .pQueuePriorities = priority
    };
#endif
    VkSurfaceKHR surface;
    /*vkCreateXlibSurfaceKHR(window, instance, NULL, &surface);*/

    return result;
}

#endif // Linux

Cbed_API void vulkan_destroy(){
    Vulkan_State *s = &vulkan__state;

    if(s->surface){
        vkDestroySurfaceKHR(s->instance, s->surface, NULL);
    }

    if(s->instance){
        vkDestroyInstance(s->instance, NULL);
    }
}
