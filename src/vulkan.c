//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------
#include "vulkan.h"
#include <vulkan/vulkan.h>

#ifdef OS_Linux
#include <X11/Xlib.h>
#include <vulkan/vulkan_xlib.h>
#endif

/*
    NOTE: This is based off the following resources:
    https://www.howtovulkan.com/
    https://vulkan-tutorial.com/
*/

Cbed_API bool vulkan_init(Window window, const char* app_name, bool debug_mode){
    bool result = true;

    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = app_name,
        .apiVersion = VK_API_VERSION_1_3
    };

#ifdef OS_Linux
    const char* instance_extensions[] = {"VK_KHR_surface", VK_KHR_XLIB_SURFACE_EXTENSION_NAME, VK_EXT_DEBUG_UTILS_EXTENSION_NAME};
#endif

    u32 instance_extensions_count = Array_Len(instance_extensions);
    if(!debug_mode){
        // Trim off the debug extensions
        instance_extensions_count -= 1;
    }

    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledExtensionCount = instance_extensions_count,
        .ppEnabledExtensionNames = instance_extensions,
    };
    VkInstance instance;
    vkCreateInstance(&instance_info, NULL, &instance);

    // NOTE: We have to cal this function once to get the count, then another time
    // once we have the memory for it.
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(instance, &device_count, NULL);
    VkPhysicalDevice devices[device_count];
    vkEnumeratePhysicalDevices(instance, &device_count, devices);

    u32 device_index = 0;
    for_count(u32, i, device_count){
        // TODO: Is there a good rule of thumb for which device to choose?
        VkPhysicalDeviceProperties2 props = {.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
        vkGetPhysicalDeviceProperties2(devices[i], &props);

        printf("vkDevice[%d]: %s\n", i, props.properties.deviceName);
    }

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

    /*VkSurfaceKHR surface;*/
    /*vkCreateXlibSurfaceKHR(window, instance, NULL, &surface);*/

    return result;
}

Cbed_API void vulkan_destroy(){

}
