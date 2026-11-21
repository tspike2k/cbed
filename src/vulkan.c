//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

/*
NOTE: This code is was originally based on the YouTube video "Learn Modern Vulkan in 2 Hours
(Dynamic Rendering, No Render Passes!)" by constref (https://www.youtube.com/watch?v=DC9FBRQKNck).

Moving forwards, code will mostly be based on the following resources:
https://www.howtovulkan.com/#intro
https://vulkan-tutorial.com/

Additional resources that might be worth using in the future:
    https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/00_Setup/00_Base_code.html

    https://www.youtube.com/watch?v=DC9FBRQKNck
    https://www.youtube.com/watch?v=G5EECLl0CSE
    https://www.youtube.com/watch?v=onQyefRdiMA
*/

/*
TODO:
    - Enable the "synchronization validation preset" in debug mode.
*/

#include "vulkan.h"

#define Vulkan__Target_API_Version VK_API_VERSION_1_3
#define Vulkan__Frames_In_Flight_Max 2
#define Vulkan__Texture_Format VK_FORMAT_R8G8B8A8_SRGB

#ifdef OS_Linux
#include <vulkan/vulkan_xlib.h>

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
#endif

typedef struct{
    VkImage        image;
    VkDeviceMemory memory;
    VkImageView    view;
} Vulkan_Image;

typedef struct{
    VkInstance     instance;
    VkDevice       device;
    VkSurfaceKHR   surface;
    VkQueue        device_queue;
    u32            queue_family_index;

    VkSwapchainKHR swapchain;
    u32            swapchain_images_count;
    u32            swapchain_w;
    u32            swapchain_h;

    VkFormat      depth_format;
    Vulkan_Image  depth_buffer;

    VkPhysicalDeviceMemoryProperties memory_properties;

    VkCommandBuffer command_buffers[Vulkan__Frames_In_Flight_Max];
    Vulkan_Buffer   constants_buffers[Vulkan__Frames_In_Flight_Max];
    VkFence         fences[Vulkan__Frames_In_Flight_Max];
    VkSemaphore     image_acquired_semaphores[Vulkan__Frames_In_Flight_Max];
    u32             render_complete_semaphores_count;
    VkSemaphore    *render_complete_semaphores;

    Vulkan_Init_Params init_params; // Client data.
} Vulkan_State;

static Vulkan_State vulkan__state;

static bool vulkan__init_surface();

static VkImageCreateInfo vulkan__get_depth_create_info(){
    Vulkan_State *s = &vulkan__state;

    VkExtent3D extent = {.width = s->swapchain_w, .height = s->swapchain_h, .depth = 1};

    VkImageCreateInfo result = {
        .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
        .imageType = VK_IMAGE_TYPE_2D,
        .format = s->depth_format,
        .extent = extent,
        .mipLevels = 1,
        .arrayLayers = 1,
        .samples = VK_SAMPLE_COUNT_1_BIT,
        .tiling = VK_IMAGE_TILING_OPTIMAL,
        .usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        // .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    return result;
}

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

static u32 vulkan__find_memory_type_index(u32 memory_type_bits, VkMemoryPropertyFlags target_properties){
    Vulkan_State *s = &vulkan__state;

    // TODO: Should we have a better way of reporting an error?
    u32 result = UINT32_MAX;

    VkMemoryType *memory_types = s->memory_properties.memoryTypes;

    // Loop through each memory index and check to see if the bit flags indicate the memory
    // index is supported. Also check to see if the given index supports the memory properties
    // we require.
    for_count(u32, i, s->memory_properties.memoryTypeCount){
        if((memory_type_bits & (1 << i))
        && (memory_types[i].propertyFlags & target_properties) == target_properties){
            result = i;
            break;
        }
    }

    // TODO: Better way of flagging if we can't find a valid memory type index?
    // This should certainly be exposed to the caller somehow.
    assert(result != UINT32_MAX);
    return result;
}

Cbed_API bool vulkan_create_buffer(u32 size, VkBufferUsageFlags usage, Vulkan_Buffer *buffer){
    Vulkan_State *s = &vulkan__state;
    bool success = false;

    VkBufferCreateInfo buffer_info = {
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .size = size,
        .usage = usage,
        .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
    };

    if(vkCreateBuffer(s->device, &buffer_info, NULL, &buffer->handle) == VK_SUCCESS){
        VkMemoryRequirements requirments;
        vkGetBufferMemoryRequirements(s->device, buffer->handle, &requirments);

        // This seems to be (RE)BAR/SAM access.
        VkMemoryPropertyFlags target_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
                                                | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT
                                                | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

        VkMemoryAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = requirments.size,
            .memoryTypeIndex = vulkan__find_memory_type_index(requirments.memoryTypeBits, target_properties),
        };

        if(vkAllocateMemory(s->device, &alloc_info, NULL, &buffer->memory) == VK_SUCCESS){
            // TODO: I guess you don't need to bind the buffer if you're mapping the buffer?
            /*vkBindBufferMemory(s->device, buffer->handle, buffer->memory, 0); // TODO: Can this fail?*/

            if(vkMapMemory(s->device, buffer->memory, 0, VK_WHOLE_SIZE, 0, &buffer->mapped_ptr) == VK_SUCCESS){
                success = true;
            }
            else{
                fmt_msg_puts("ERROR: Unable to map memory for Vulkan buffer.\n");
            }
        }
        else{
            fmt_msg_puts("ERROR: Unable to allocate Vulkan buffer memory.\n");
        }
    }
    else{
        fmt_msg_puts("ERROR: Unable to create Vulkan buffer.\n");
    }

    return success;
}

static bool vulkan__create_image(VkImageCreateInfo *c_info, Vulkan_Image* img){
    Vulkan_State *s = &vulkan__state;

    bool result = false;

    // TODO: Are these the only properties you'll need for images? It seems like it.
    VkMemoryPropertyFlags memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    if(vkCreateImage(s->device, c_info, NULL, &img->image) == VK_SUCCESS){
        VkMemoryRequirements requirments;
        vkGetImageMemoryRequirements(s->device, img->image, &requirments);

        VkMemoryAllocateInfo alloc_info = {
            .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize = requirments.size,
            .memoryTypeIndex = vulkan__find_memory_type_index(requirments.memoryTypeBits, memory_properties),
        };

        if(vkAllocateMemory(s->device, &alloc_info, NULL, &img->memory) == VK_SUCCESS){
            vkBindImageMemory(s->device, img->image, img->memory, 0); // TODO: Can this fail?

            // NOTE: subresourceRange refers to the aspects of the image we want to access.
            // This could be color, depth, or stencil data.

            u32 aspect_mask = 0;
            if(c_info->usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT){
                aspect_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
            }

            // TODO: Do all images really need a view? Maybe not.
            VkImageViewCreateInfo view_info = {
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = img->image,
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format = c_info->format,
                .subresourceRange = { .aspectMask = aspect_mask, .levelCount = 1, .layerCount = 1 }
            };

            if(vkCreateImageView(s->device, &view_info, NULL, &img->view) == VK_SUCCESS){
                result = true;
            }
            else{
                fmt_msg_puts("ERROR: Unable to create view for Vulkan image.\n");
            }
        }
        else{
            fmt_msg_puts("ERROR: Unable to allocate memory for Vulkan image.\n");
        }
    }
    else{
        fmt_msg_puts("ERROR: Unable to create Vulkan image.\n");
    }

    return result;
}

Cbed_API bool vulkan_init(Vulkan_Init_Params *params){
    Vulkan_State *s = &vulkan__state;
    s->init_params = *params;

    //
    // Create a new Vulkan instance
    //
    VkApplicationInfo app_info = {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = params->app_name,
        .apiVersion = Vulkan__Target_API_Version
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

    const char **instance_extensions = vulkan__instance_extensions;
    u32 instance_extensions_count = Array_Len(vulkan__instance_extensions);
    if(params->debug_mode){
        instance_extensions = vulkan__instance_extensions_debug;
        instance_extensions_count = Array_Len(vulkan__instance_extensions_debug);
    }

    // TODO: Should we use vkEnumerateInstanceExtensionProperties to make sure the extensions
    // are supported before we try to call vkCreateInstance? It seems rather uneeded.
    VkInstanceCreateInfo instance_info = {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo        = &app_info,
        .enabledExtensionCount   = instance_extensions_count,
        .ppEnabledExtensionNames = instance_extensions,
    };

    if(params->debug_mode){
        instance_info.pNext               = &debug_info;
        instance_info.enabledLayerCount   = layers_count;
        instance_info.ppEnabledLayerNames = layers;
    }

    if(vkCreateInstance(&instance_info, NULL, &s->instance) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to create Vulkan instance.\n");
        return false;
    }

    // NOTE: According to the official "Khronos Vulkan Tutorial", the surface must be created
    // right after creating the Vulkan instance. This is because it can affect the physical
    // device list. See here for more information:
    // https://docs.vulkan.org/tutorial/latest/03_Drawing_a_triangle/01_Presentation/00_Window_surface.html
    if(!vulkan__init_surface()){
        // TODO: Report error?
        return false;
    }

    //
    // Find the physical device. This is a list of Vulkan-capable hardware connected
    // to the host PC.
    //

    u32 device_count = 0;
    VkPhysicalDevice physical_device = NULL;

    // TODO: Allow player to configure which device to prefer. By index? By name?
    //
    // NOTE: We have to call this function once to get the count, then another time
    // once we have the memory for it. Several Vulkan functions work this way.
    vkEnumeratePhysicalDevices(s->instance, &device_count, NULL);
    if(device_count){
        VkPhysicalDevice devices[device_count];
        vkEnumeratePhysicalDevices(s->instance, &device_count, devices);

        for_count(u32, i, device_count){
            // TODO: Is there a good rule of thumb for which device to choose?
            VkPhysicalDeviceProperties2 props = { .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2};
            vkGetPhysicalDeviceProperties2(devices[i], &props);
            if(props.properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU
            && props.properties.apiVersion >= Vulkan__Target_API_Version){
                const char *device_name = props.properties.deviceName;
                fmt_msg("Target Vulkan physical device[{0}]: {1}\n", fmt_i(i), fmt_cstr(device_name));
                physical_device = devices[i];
                break;
            }
        }
    }

    if(!physical_device){
        fmt_msg_puts("ERROR: Unable to get physical Vulkan device.\n");
        return false;
    }

    vkGetPhysicalDeviceMemoryProperties(physical_device, &s->memory_properties);

    VkImageFormatProperties image_properties;
    VkResult format_result = vkGetPhysicalDeviceImageFormatProperties(
        physical_device,
        Vulkan__Texture_Format,
        VK_IMAGE_TYPE_2D,
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        0,
        &image_properties
    );
    if(format_result != VK_SUCCESS){
        fmt_msg_puts("ERROR: Vulkan doesn't support required texture format.\n");
        return false;
    }

    //
    // Get the graphics queue. This is used to submit work from the CPU to the GPU. // TODO: Is this accurate?
    // Normally, an application will only need one of these.
    //
    u32 queue_families_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_families_count, NULL);
    bool found_queue_family = false;

    if(queue_families_count){
        VkQueueFamilyProperties2 queue_family_props[queue_families_count];
        for_count(u32, i, queue_families_count){
            queue_family_props[i] = (VkQueueFamilyProperties2){
                .sType = VK_STRUCTURE_TYPE_QUEUE_FAMILY_PROPERTIES_2,
            };
        }
        vkGetPhysicalDeviceQueueFamilyProperties2(physical_device, &queue_families_count, queue_family_props);

        for_count(u32, i, queue_families_count){
            VkBool32 can_present = VK_FALSE;

            vkGetPhysicalDeviceSurfaceSupportKHR(physical_device, i, s->surface, &can_present);

            VkQueueFamilyProperties2 *p = &queue_family_props[i];
            if(p->queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT && can_present){
                s->queue_family_index = i;
                found_queue_family = true;
                break;
            }
        }
    }

    if(!found_queue_family){
        fmt_msg_puts("ERROR: Unable to find Vulkan queue family.\n");
        return false;
    }

#if 0
    //
    // Get the features of each major Vulkan version the physical device supports. We chain each
    // version feature structure together to get the information all in one call.
    //
    // TODO: Do we really need to do this? Can't we just request the features we want and
    // check for errors?
	VkPhysicalDeviceVulkan14Features features_1_4 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_4_FEATURES
    };
	VkPhysicalDeviceVulkan13Features features_1_3 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = &features_1_4
    };
	VkPhysicalDeviceVulkan12Features features_1_2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &features_1_3
    };
	VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features_1_2
    };
	vkGetPhysicalDeviceFeatures2(physical_device, &features);

    // Make sure the features we'll be relying on are supported.
    bool supports_features = features_1_3.dynamicRendering && features_1_3.synchronization2
                          && features_1_2.timelineSemaphore;
	if(!supports_features){
		fmt_msg_puts("ERROR: Vulkan device doesn't support required features.\n");
		return false;
	}
#endif

    // Now request the device feature we need.
	VkPhysicalDeviceVulkan13Features features_1_3 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.synchronization2 = VK_TRUE, // Better synchronization features
		.dynamicRendering = VK_TRUE, // Removed the need for the old render pass use
	};
    VkPhysicalDeviceVulkan12Features features_1_2 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &features_1_3,
		.timelineSemaphore = VK_TRUE
	};
    VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features_1_2
    };

    float queue_priority[] = {1.0f};
	VkDeviceQueueCreateInfo queue_create_info = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = s->queue_family_index,
		.queueCount = Array_Len(queue_priority),
		.pQueuePriorities = queue_priority
	};

    const char *device_extensions[] = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

    VkDeviceCreateInfo device_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = &features,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &queue_create_info,
		.enabledExtensionCount = Array_Len(device_extensions),
		.ppEnabledExtensionNames = device_extensions,
    };

    // Create an instance of the physical device, called a "logical device."
	if(vkCreateDevice(physical_device, &device_create_info, NULL, &s->device) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to create Vulkan logical device with requested features.\n");
		return false;
	}

    s->device_queue = 0;
    vkGetDeviceQueue(s->device, s->queue_family_index, 0, &s->device_queue);
    if(!s->device_queue){
        fmt_msg_puts("ERROR: Unable to create get the device queue.\n");
        return false;
    }

    //
    // Choose and setup the swapchain
    //

    VkSurfaceCapabilitiesKHR surface_capabilities;
    if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physical_device, s->surface, &surface_capabilities) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to query surface capabilities of Vulkan physical device.\n");
        return false;
    }

    VkExtent2D swapchain_extent = surface_capabilities.currentExtent;
    if(swapchain_extent.width == 0xFFFFFFFF){
        // NOTE: This is an odd Wayland design decision. If the surface capability extents are
        // 0xFFFFFFFF, then the swapchain needs its own extents set to the size of the window.
        // As a window manager, you'd think Wayland would know the window extents and pass that
        // to us, but I guess it needs our help here.
        swapchain_extent = (VkExtent2D){params->window_width, params->window_height};
    }

    // TODO: Some tutorials say we should use less. Should we? Two is double buffering, right?
    // Why would we want more than that? Or is this the queue that Vsync will block on once
    // it's full?
    u32 min_image_count = surface_capabilities.minImageCount;

    // This determines the way images are presented to the screen. This is appearently Vsync mode
    // and should be usable on all devices that support Vulkan.
    // https://www.howtovulkan.com/#swapchain
    u32 present_mode = VK_PRESENT_MODE_FIFO_KHR;

    // Appearently, this pairing of image formats and colorspace should be availible on all
    // Vulkan devices.
    // https://www.howtovulkan.com/#swapchain
    VkFormat image_format = VK_FORMAT_B8G8R8A8_SRGB;
    u32 image_colorspace  = VK_COLORSPACE_SRGB_NONLINEAR_KHR;

    VkSwapchainCreateInfoKHR swapchain_creat_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = s->surface,
        .minImageCount = min_image_count,
        .imageFormat = image_format,
        .imageColorSpace = image_colorspace,
        .imageExtent = swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = present_mode
    };

    s->swapchain_w = swapchain_extent.width;
    s->swapchain_h = swapchain_extent.height;

    if(vkCreateSwapchainKHR(s->device, &swapchain_creat_info, NULL, &s->swapchain) != VK_SUCCESS){
        // TODO: Give some ideas as to what sort of formats we were questing? Would that help
        // a user?
        fmt_msg_puts("ERROR: Unable to create Vulkan swapchain.\n");
        return false;
    }

    if(vkGetSwapchainImagesKHR(s->device, s->swapchain, &s->swapchain_images_count, NULL) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to get Vulkan swapchain images count.\n");
        return false;
    }


    /*chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr));*/
    /*swapchainImages.resize(imageCount);*/
    /*chk(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapchainImages.data()));*/
    /*swapchainImageViews.resize(imageCount);*/

    //
    // Create depth attachment for the swapchain
    //

    // One of these two depth formats are supposed to be supported by all Vulkan devices:
    // https://www.howtovulkan.com/#swapchain
    VkFormat target_depth_formats[] = {VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
    s->depth_format = VK_FORMAT_UNDEFINED;
    for_count(u32, i, Array_Len(target_depth_formats)){
        VkFormat target_format = target_depth_formats[i];

        VkFormatProperties2 props = {.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2};
        vkGetPhysicalDeviceFormatProperties2(physical_device, target_format, &props);
        if(props.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT){
            s->depth_format = target_format;
            break;
        }
    }

    if(s->depth_format == VK_FORMAT_UNDEFINED){
        fmt_msg_puts("ERROR: Unable to get depth buffer format for Vulkan surface.\n");
        return false;
    }

    VkImageCreateInfo depth_create_info = vulkan__get_depth_create_info();
    if(!vulkan__create_image(&depth_create_info, &s->depth_buffer)){
        fmt_msg_puts("ERROR: Unable to create Vulkan depth buffer.\n");
        return false;
    }

    //
    // Create the constant buffer
    //

    VkBufferUsageFlags constants_usage = VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
    for_count(u32, i, Array_Len(s->constants_buffers)){
        // TODO: Do we need to use vkGetBufferDeviceAddress? See here for more:
        // https://www.howtovulkan.com/#shader-data-buffers
        if(!vulkan_create_buffer(Constants_Buffer_Size, constants_usage, &s->constants_buffers[i])){
            fmt_msg_puts("ERROR: Unable to create Vulkan constants buffer.\n");
            return false;
        }
    }

    //
    // Crate synchronization data
    //
    VkFenceCreateInfo fence_cinfo = {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .flags = VK_FENCE_CREATE_SIGNALED_BIT
    };

    for_count(u32, i, Array_Len(s->fences)){
        if(vkCreateFence(s->device, &fence_cinfo, NULL, &s->fences[i]) != VK_SUCCESS){
            fmt_msg_puts("ERROR: Unable to create Vulkan fence.\n");
            return false;
        }
    }

    VkSemaphoreCreateInfo semaphore_cinfo = {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
    };

    for_count(u32, i, Array_Len(s->image_acquired_semaphores)){
        if(vkCreateSemaphore(s->device, &semaphore_cinfo, NULL, &s->image_acquired_semaphores[i]) != VK_SUCCESS){
            fmt_msg_puts("ERROR: Unable to create Vulkan semaphore.\n");
            return false;
        }
    }

    s->render_complete_semaphores_count = s->swapchain_images_count;
    s->render_complete_semaphores = buffer_push_array(VkSemaphore, params->memory, s->render_complete_semaphores_count);

    for_count(u32, i, s->render_complete_semaphores_count){
        VkSemaphore *semaphore = &s->render_complete_semaphores[i];
        if(vkCreateSemaphore(s->device, &semaphore_cinfo, NULL, semaphore) != VK_SUCCESS){
            fmt_msg_puts("ERROR: Unable to create Vulkan semaphore.\n");
            return false;
        }
    }

    //
    // Command buffer initialization
    //
    VkCommandPoolCreateInfo command_pool_cinfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = s->queue_family_index
    };

    // NOTE: If multiple threads are to fill command buffers, each thread would need their own
    // command pool.
    VkCommandPool command_pool;
    if(vkCreateCommandPool(s->device, &command_pool_cinfo, NULL, &command_pool) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to create Vulkan command pool.\n");
        return false;
    }

    VkCommandBufferAllocateInfo command_buffer_cinfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = command_pool,
        .commandBufferCount = Array_Len(s->command_buffers)
    };

    if(vkAllocateCommandBuffers(s->device, &command_buffer_cinfo, s->command_buffers) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to create Vulkan command buffers.\n");
        return false;
    }

    return true;
}

Cbed_API void vulkan_destroy(){
    Vulkan_State *s = &vulkan__state;

    if(s->swapchain){
        vkDestroySwapchainKHR(s->device, s->swapchain, NULL);
    }

    if(s->device){
        vkDestroyDevice(s->device, NULL);
    }

    if(s->surface){
        vkDestroySurfaceKHR(s->instance, s->surface, NULL);
    }

    if(s->instance){
        vkDestroyInstance(s->instance, NULL);
    }
}

////
//
// Linux
//
////
#ifdef OS_Linux

#include <vulkan/vulkan_xlib.h>

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

////
//
// Linux
//
////
#endif
