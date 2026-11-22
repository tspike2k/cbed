//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

/*
NOTE: This code is based on the following resource:
https://vulkan-tutorial.com/
https://www.howtovulkan.com/
*/

#include "vulkan.h"

#define Vulkan__Target_API_Version VK_API_VERSION_1_3

typedef struct{
    VkInstance     instance;
    VkSurfaceKHR   surface;
    u32            queue_family_index;

    Vulkan_Init_Params init_params; // Client data.
} Vulkan_State;

static Vulkan_State vulkan__state;

#ifdef OS_Linux
#   include <vulkan/vulkan_xlib.h>

    // Target instance extensions
    const char* vulkan__instance_extensions[] = {
        "VK_KHR_surface",
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
    };

    const char* vulkan__instance_extensions_debug[] = {
        "VK_KHR_surface",
        VK_KHR_XLIB_SURFACE_EXTENSION_NAME,
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };

    static bool vulkan__init_surface(){
        Vulkan_State *s = &vulkan__state;
        Vulkan_Init_Params *params = &s->init_params;

        VkXlibSurfaceCreateInfoKHR surface_info = {
            .sType = VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR,
            // .flags = ???, // TODO: What flags? Do we need to set flags?
            .dpy    = params->xlib_display,
            .window = params->xlib_window,
        };

        if(vkCreateXlibSurfaceKHR(s->instance, &surface_info, NULL, &s->surface) != VK_SUCCESS){
            fmt_msg_puts("ERROR: Unable to create Vulkan surface.\n");
            return false;
        }

        return true;
    }
#endif

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

// TODO: vulkan-tutorial.com sugests ranking physical devices based on their specs and chooing
// the best one. We should probably do this.
static bool vulkan__test_physical_device(VkPhysicalDevice device, VkPhysicalDeviceProperties2 *props, u32 *queue_family_index){
    Vulkan_State *s = &vulkan__state;

    bool is_valid_device = false;

    // Check to see if the physical device meets our requirements.
    *props = (VkPhysicalDeviceProperties2){.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
    vkGetPhysicalDeviceProperties2(device, props);

    if(props->properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
    && props->properties.apiVersion >= Vulkan__Target_API_Version){
        // Check to see if the device has a valid queue family. If it does, return it though
        // the queue_family_index parameter.
        u32 queue_families_count = 0;
        vkGetPhysicalDeviceQueueFamilyProperties2(device, &queue_families_count, NULL);
        if(queue_families_count > 0){
            VkQueueFamilyProperties2 queue_families[queue_families_count];
            for_count(u32, i, queue_families_count){
                queue_families[i] = (VkQueueFamilyProperties2){
                    .sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2,
                };
            }

            vkGetPhysicalDeviceQueueFamilyProperties2(device, &queue_families_count, queue_families);
            for_count(u32, i, queue_families_count){
                VkBool32 can_present = VK_FALSE;
                if(vkGetPhysicalDeviceSurfaceSupportKHR(device, i, s->surface, &can_present) != VK_SUCCESS){
                    can_present = VK_FALSE; // NOTE: Sanity check
                }

                VkQueueFamilyProperties2 *p = &queue_families[i];
                if(p->queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT && can_present){
                    *queue_family_index = i;
                    is_valid_device = true;
                    break;
                }
            }
        }
    }

    return is_valid_device;
}

Cbed_API bool vulkan_init(Vulkan_Init_Params *params){
    Vulkan_State *s = &vulkan__state;

    s->init_params = *params;
    /*Buffer *scratch = params->scratch;*/

    //
    // Create a new Vulkan instance
    //
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = params->app_name,
        .apiVersion = Vulkan__Target_API_Version
    };

    // TODO: Should we use vkEnumerateInstanceExtensionProperties to make sure the extensions
    // are supported before we try to call vkCreateInstance? It seems rather uneeded.
    VkInstanceCreateInfo instance_cinfo = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
    };

    const char *validation_layers[] = {
        "VK_LAYER_KHRONOS_validation",
    };

    VkDebugUtilsMessengerCreateInfoEXT debug_callback_cinfo = {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
        .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
                         | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
        .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
                     | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
        .pfnUserCallback = vulkan__debug_msg_callback,
    };

    if(params->debug_mode){
        instance_cinfo.ppEnabledExtensionNames = vulkan__instance_extensions_debug;
        instance_cinfo.enabledExtensionCount   = Array_Len(vulkan__instance_extensions_debug);

        instance_cinfo.ppEnabledLayerNames = validation_layers;
        instance_cinfo.enabledLayerCount   = Array_Len(validation_layers);
        instance_cinfo.pNext = &debug_callback_cinfo;
    }
    else{
        instance_cinfo.ppEnabledExtensionNames = vulkan__instance_extensions;
        instance_cinfo.enabledExtensionCount   = Array_Len(vulkan__instance_extensions);
    }

    if(vkCreateInstance(&instance_cinfo, NULL, &s->instance) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to create Vulkan instance.\n");
        return false;
    }

    if(!vulkan__init_surface()){
        return false;
    }

    // TODO: Allow player to configure which device to prefer. By index? By name?
    VkPhysicalDevice physical_device = VK_NULL_HANDLE;

    u32 physical_device_count;
    if(vkEnumeratePhysicalDevices(s->instance, &physical_device_count, NULL) == VK_SUCCESS
    && physical_device_count){
        VkPhysicalDevice devices[physical_device_count];
        VkPhysicalDeviceProperties2 properties;

        if(vkEnumeratePhysicalDevices(s->instance, &physical_device_count, devices) == VK_SUCCESS){
            for_count(u32, device_index, physical_device_count){
                VkPhysicalDevice device = devices[device_index];

                u32 queue_family_index;
                if(vulkan__test_physical_device(device, &properties, &queue_family_index)){
                    const char *device_name = properties.properties.deviceName;
                    fmt_msg("Target Vulkan physical device[{0}]: {1}\n", fmt_i(device_index), fmt_cstr(device_name));
                    physical_device = device;
                    s->queue_family_index = queue_family_index;
                }
            }
        }
    }

    if(physical_device == VK_NULL_HANDLE){
        fmt_msg_puts("ERROR: Unable to get physical Vulkan device.\n");
        return false;
    }

    return true;
}

Cbed_API void vulkan_destroy(){
    Vulkan_State *s = &vulkan__state;

    // TODO: Are all of these null if invalid/not created?
/*
    if(s->swapchain){
        vkDestroySwapchainKHR(s->device, s->swapchain, NULL);
    }

    if(s->device){
        vkDestroyDevice(s->device, NULL);
    }
    */

    if(s->surface){
        vkDestroySurfaceKHR(s->instance, s->surface, NULL);
    }

    if(s->instance){
        vkDestroyInstance(s->instance, NULL);
    }
}
