//------------------------------------------------------------------------------
// Authors:   tspike (github.com/tspike2k)
// Copyright: Copyright (c) 2026
// License:   Boost Software License 1.0 (https://www.boost.org/LICENSE_1_0.txt)
//------------------------------------------------------------------------------

/*
NOTE: This code is based on the following resource:
https://www.howtovulkan.com/
https://vulkan-tutorial.com/
*/

#include "vulkan.h"
#include "files.h" // TODO: This is only to load the test shader.

#define Vulkan__Target_API_Version VK_API_VERSION_1_3

// This determines the way images are presented to the screen. This is appearently Vsync mode
// and should be usable on all devices that support Vulkan.
// https://www.howtovulkan.com/#swapchain
#define Vulkan__Target_Present_Mode VK_PRESENT_MODE_FIFO_KHR

// TODO: According to https://www.howtovulkan.com/#swapchain, this pairing of surface format and colorspace
// should be availible on all Vulkan capable devices. However, this is the only place I've seen
// that says this, so I can't confirm. See if we can find proof of this somewhere.
#define Vulkan__Target_Surface_Format VK_FORMAT_B8G8R8A8_SRGB
#define Vulkan__Target_Surface_Colorspace VK_COLORSPACE_SRGB_NONLINEAR_KHR

typedef struct{
    VkInstance       instance;
    VkPhysicalDevice physical_device;
    VkSurfaceKHR     surface;
    VkDevice         device;
    VkQueue          graphics_queue;
    u32              queue_family_index;

    VkPipelineLayout pipeline_layout;
    VkPipeline       graphics_pipeline;

    VkCommandPool   command_pool;
    VkCommandBuffer command_buffer;

    VkSwapchainKHR swapchain;
    VkExtent2D     swapchain_extent;
    u32            swapchain_images_count;
    VkImage       *swapchain_images;
    VkImageView   *swapchain_image_views;

    Vulkan_Init_Params init_params; // Client data.
} Vulkan_State;

static Vulkan_State vulkan__state;

// These are the required device extensions. After querying the physical device to ensure they
// are supported, we then use them when creating the logical device (the VkDevice).
const char *vulkan__device_extensions[] = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

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

static bool vulkan__device_supports_required_extensions(VkPhysicalDevice device){
    bool supported = false;

    // These are the extensions the physical device must support in order for this Vulkan
    // code to operate correctly.

    const char **extensions = vulkan__device_extensions;
    u32 extension_count     = Array_Len(vulkan__device_extensions);

    u32 props_count;
    if(vkEnumerateDeviceExtensionProperties(device, NULL, &props_count, NULL) == VK_SUCCESS
    && props_count){
        // TODO: I'm not a big fan of using malloc/free here. We should probably pass vulkan_init
        // a scratch buffer at the very least and allocate from it.
        VkExtensionProperties *props = malloc(sizeof(VkExtensionProperties) * props_count);
        if(vkEnumerateDeviceExtensionProperties(device, NULL, &props_count, props) == VK_SUCCESS){
            for_count(u32, prop_index, props_count){
                const char *src_ext_name = props[prop_index].extensionName;

                for_count(u32, extension_index, extension_count){
                    const char *target_ext_name = extensions[extension_index];

                    // If the extension is supported, remove it from our array of extensions to
                    // check for. If the array is empty when we're done, all extensions are
                    // supported.
                    // This is adapted from vulkan-tutorial.com, which used an std::set for this.
                    if(strcmp(src_ext_name, target_ext_name) == 0){
                        extensions[extension_index] = extensions[extension_count-1];
                        extension_count--;
                        break;
                    }
                }
            }

            supported = extension_count == 0;
        }

        free(props);
    }
    return supported;
}

static bool vulkan__test_physical_device(VkPhysicalDevice device, VkPhysicalDeviceProperties2 *props, u32 *queue_family_index){
    Vulkan_State *s = &vulkan__state;

    bool is_valid_device = false;
    if(vulkan__device_supports_required_extensions(device)){
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
                    if((p->queueFamilyProperties.queueFlags & VK_QUEUE_GRAPHICS_BIT) && can_present){
                        *queue_family_index = i;
                        is_valid_device = true;
                        break;
                    }
                }
            }
        }
    }

    //
    // Check to make sure the physical device supports the features we require.
    //
    VkPhysicalDeviceVulkan13Features features_1_3 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
    };
	VkPhysicalDeviceVulkan12Features features_1_2 = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
        .pNext = &features_1_3
    };
	VkPhysicalDeviceFeatures2 features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &features_1_2
    };

	vkGetPhysicalDeviceFeatures2(device, &features);
    bool supports_features = features_1_3.dynamicRendering
                      && features_1_3.synchronization2
                      && features_1_2.timelineSemaphore;

    bool result = is_valid_device && supports_features;
    return result;
}

// TODO: We also need a vulkan__destroy_swapchain function.
static bool vulkan__create_swapchain(u32 target_w, u32 target_h){
    Vulkan_State *s = &vulkan__state;

    VkSurfaceCapabilitiesKHR surface_capabilities;
    if(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(s->physical_device, s->surface, &surface_capabilities) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to query surface capabilities of Vulkan physical device.\n");
        return false;
    }

    VkExtent2D swapchain_extent = surface_capabilities.currentExtent;
    if(swapchain_extent.width == 0xFFFFFFFF){
        // NOTE: This is an odd Wayland design decision. If the surface capability extents are
        // 0xFFFFFFFF, then the swapchain needs its own extents set to the size of the window.
        // As a window manager, you'd think Wayland would know the window extents and pass that
        // to us, but I guess it needs our help here.
        swapchain_extent = (VkExtent2D){target_w, target_h};
    }

    // TODO: Some tutorials say we should use less. Should we? Two is double buffering, right?
    // Why would we want more than that? Or is this the queue that Vsync will block on once
    // it's full?
    u32 min_image_count = surface_capabilities.minImageCount;

    // TODO: Should we query supported color formats and colorspaces and pick one?

    VkSwapchainCreateInfoKHR swapchain_creat_info = {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = s->surface,
        .minImageCount = min_image_count,
        .imageFormat = Vulkan__Target_Surface_Format,
        .imageColorSpace = Vulkan__Target_Surface_Colorspace,
        .imageExtent = swapchain_extent,
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        .presentMode = Vulkan__Target_Present_Mode,
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,

        // Investigate which preTransform to use. The two tutorials differ on their choice.
        /*.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR,*/
        .preTransform = surface_capabilities.currentTransform,
    };

    s->swapchain_extent = swapchain_extent;

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

    // TODO: malloc is about the only thing I can think of to do here. Can we tell Vulkan what
    // our maximum images count can be and use a fixed buffer instead?
    s->swapchain_images = malloc(sizeof(VkImage)*s->swapchain_images_count);
    if(vkGetSwapchainImagesKHR(s->device, s->swapchain, &s->swapchain_images_count, s->swapchain_images) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to get Vulkan swapchain images.\n");
        return false;
    }

    s->swapchain_image_views = malloc(sizeof(VkImageView)*s->swapchain_images_count);
    for_count(u32, i, s->swapchain_images_count){
        VkImageViewCreateInfo view_cinfo = {
            .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .image    = s->swapchain_images[i],
            .viewType = VK_IMAGE_VIEW_TYPE_2D,
            .format   = Vulkan__Target_Surface_Format,
            .components.r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .components.a = VK_COMPONENT_SWIZZLE_IDENTITY,
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .levelCount = 1,
                .layerCount = 1,
            },
        };

        if(vkCreateImageView(s->device, &view_cinfo, NULL, &s->swapchain_image_views[i]) != VK_SUCCESS){
            fmt_msg_puts("ERROR: Unable to create Vulkan swapchain image view.\n");
            return false;
        }
    }

    return true;
}

static bool vulkan__create_shader_module(String src, VkShaderModule *module){
    Vulkan_State *s = &vulkan__state;

    // TODO: Does the code pointer need special alignment?
    VkShaderModuleCreateInfo shader_cinfo = {
        .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
        .codeSize = src.size,
        .pCode = (u32*)src.text
    };

    bool success = false;
    if(vkCreateShaderModule(s->device, &shader_cinfo, NULL, module) == VK_SUCCESS){
        success = true;
    }
    else{
        fmt_msg_puts("ERROR: Unable to create Vulkan shader module.\n");
    }

    return success;
}

static void vulkan__destroy_swapchain(){
    Vulkan_State *s = &vulkan__state;

    if(s->swapchain_image_views){
        // TODO: What if not all the image views were initialized?
        for_count(u32, i, s->swapchain_images_count){
            vkDestroyImageView(s->device, s->swapchain_image_views[i], NULL);
        }

        free(s->swapchain_image_views);
        s->swapchain_image_views = NULL;
    }

    if(s->swapchain){
        vkDestroySwapchainKHR(s->device, s->swapchain, NULL);
    }
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
    s->physical_device = VK_NULL_HANDLE;

    u32 physical_device_count;
    if(vkEnumeratePhysicalDevices(s->instance, &physical_device_count, NULL) == VK_SUCCESS
    && physical_device_count){
        VkPhysicalDevice devices[physical_device_count];
        VkPhysicalDeviceProperties2 properties;

        if(vkEnumeratePhysicalDevices(s->instance, &physical_device_count, devices) == VK_SUCCESS){
            for_count(u32, device_index, physical_device_count){
                VkPhysicalDevice device = devices[device_index];

                // TODO: vulkan-tutorial.com sugests ranking physical devices based on their
                // specs and chooing the best one. We should probably do this.
                u32 queue_family_index;
                if(vulkan__test_physical_device(device, &properties, &queue_family_index)){
                    const char *device_name = properties.properties.deviceName;
                    fmt_msg("Target Vulkan physical device[{0}]: {1}\n", fmt_i(device_index), fmt_cstr(device_name));
                    s->physical_device = device;
                    s->queue_family_index = queue_family_index;
                }
            }
        }
    }

    // TODO: Should we store the physical device?
    if(s->physical_device == VK_NULL_HANDLE){
        fmt_msg_puts("ERROR: Unable to get physical Vulkan device.\n");
        return false;
    }

    // Now request the device feature we need.
	VkPhysicalDeviceVulkan13Features device_features_1_3 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
		.synchronization2 = VK_TRUE, // Better synchronization features
		.dynamicRendering = VK_TRUE, // Removed the need for the old render pass use
	};
    VkPhysicalDeviceVulkan12Features device_features_1_2 = {
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES,
		.pNext = &device_features_1_3,
		.timelineSemaphore = VK_TRUE
	};
    VkPhysicalDeviceFeatures2 device_features = {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
        .pNext = &device_features_1_2
    };

    float queue_priority[] = {1.0f};
	VkDeviceQueueCreateInfo queue_cinfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.queueFamilyIndex = s->queue_family_index,
		.queueCount = Array_Len(queue_priority),
		.pQueuePriorities = queue_priority
	};

    /*VkPhysicalDeviceFeatures device_features = {};*/

    VkDeviceCreateInfo device_cinfo = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pQueueCreateInfos = &queue_cinfo,
        .queueCreateInfoCount = 1,
        /*.pEnabledFeatures = &device_features,*/

        .pNext = &device_features,

        .ppEnabledExtensionNames = vulkan__device_extensions,
        .enabledExtensionCount   = Array_Len(vulkan__device_extensions)
    };

    if(vkCreateDevice(s->physical_device, &device_cinfo, NULL, &s->device) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to create Vulkan logical device.\n");
        return false;
    }

    // TODO: Can this fail? What is the value of s->device_queue on failure?
    vkGetDeviceQueue(s->device, s->queue_family_index, 0, &s->graphics_queue);

    if(!vulkan__create_swapchain(params->window_width, params->window_height)){
        return false;
    }

    //
    // Pipeline creation
    //

    // TODO: Use a temp buffer!
    String vert_source = file_read_into_memory("./bin/vulkan_test_vert.spv", s->init_params.memory);
    String frag_source = file_read_into_memory("./bin/vulkan_test_frag.spv", s->init_params.memory);

    // TODO: All shader modules need to be explicitly freed. This complicates our logic here.
    VkShaderModule vert_module;
    VkShaderModule frag_module;
    vulkan__create_shader_module(vert_source, &vert_module);
    vulkan__create_shader_module(frag_source, &frag_module);

    VkPipelineShaderStageCreateInfo shader_stages_cinfo[] = {
        {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_VERTEX_BIT,
            .module = vert_module,
            .pName = "main",
        },
        {
            .sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = VK_SHADER_STAGE_FRAGMENT_BIT,
            .module = frag_module,
            .pName = "main",
        }
    };

    VkDynamicState dynamic_state[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamic_state_cinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = Array_Len(dynamic_state),
        .pDynamicStates = dynamic_state,
    };

    // NOTE: Since we're hardcoding the vertices inside the shader, we don't really
    // have any formats to describe.
    VkPipelineVertexInputStateCreateInfo vertex_input_cinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
    };

    VkPipelineInputAssemblyStateCreateInfo input_assembly_cinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
        .primitiveRestartEnable = VK_FALSE
    };

    VkViewport viewport = {
        .x = 0,
        .y = 0,
        .width  = s->swapchain_extent.width,
        .height = s->swapchain_extent.height,
        .minDepth = 0,
        .maxDepth = 1,
    };

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = s->swapchain_extent
    };

    VkPipelineViewportStateCreateInfo viewport_cinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        .pViewports    = &viewport,
        .scissorCount = 1,
        .pScissors    = &scissor
    };

    VkPipelineRasterizationStateCreateInfo raster_cinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .polygonMode = VK_POLYGON_MODE_FILL,
        .lineWidth = 1,
        /*.cullMode = VK_CULL_MODE_BACK_BIT,*/
        /*.frontFace = VK_FRONT_FACE_CLOCKWISE,*/
    };

    VkPipelineColorBlendAttachmentState color_blend_attachment = {
        .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
                        | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
    };

    VkPipelineColorBlendStateCreateInfo color_blend_cinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOp = VK_LOGIC_OP_COPY,
        .attachmentCount = 1,
        .pAttachments = &color_blend_attachment
    };

    // NOTE: The pipeline layout determines the shader uniforms. It can also be used to define
    // "push constants"
    VkPipelineLayoutCreateInfo pipeline_layout_cinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
    };

    if(vkCreatePipelineLayout(s->device, &pipeline_layout_cinfo, NULL, &s->pipeline_layout) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to create Vulkan pipeline layout.\n");
        return false;
    }

    u32 image_format = Vulkan__Target_Surface_Format;
    VkPipelineRenderingCreateInfo rendering_cinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .colorAttachmentCount = 1,
        .pColorAttachmentFormats = &image_format,
        /*.depthAttachmentFormat = depthFormat*/
    };

    VkPipelineMultisampleStateCreateInfo multisample_cinfo = {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .sampleShadingEnable = VK_FALSE,
        .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
        /*.minSampleShading = 1.0f; // Optional*/
        /*.pSampleMask = nullptr; // Optional*/
        /*.alphaToCoverageEnable = VK_FALSE; // Optional*/
        /*.alphaToOneEnable = VK_FALSE; // Optional*/
    };

    VkGraphicsPipelineCreateInfo pipeline_cinfo = {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = &rendering_cinfo,
        .stageCount = 2,
        .pStages = shader_stages_cinfo,
        .pVertexInputState = &vertex_input_cinfo,
        .pInputAssemblyState = &input_assembly_cinfo,
        .pViewportState = &viewport_cinfo,
        .pRasterizationState = &raster_cinfo,
        .pMultisampleState = &multisample_cinfo,
        /*.pDepthStencilState = &depthStencilState,*/
        .pColorBlendState = &color_blend_cinfo,
        .pDynamicState = &dynamic_state_cinfo,
        .layout = s->pipeline_layout
    };

    if(vkCreateGraphicsPipelines(s->device, VK_NULL_HANDLE, 1, &pipeline_cinfo, NULL, &s->graphics_pipeline) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to create Vulkan graphics pipelines.\n");
        return false;
    }

    vkDestroyShaderModule(s->device, frag_module, NULL);
    vkDestroyShaderModule(s->device, vert_module, NULL);

    VkCommandPoolCreateInfo cmd_pool_cinfo ={
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
        .queueFamilyIndex = s->queue_family_index
    };

    if(vkCreateCommandPool(s->device, &cmd_pool_cinfo, NULL, &s->command_pool) != VK_SUCCESS) {
        fmt_msg_puts("ERROR: Unable to create Vulkan command pool.\n");
        return false;
    }

    VkCommandBufferAllocateInfo cmd_buffer_cinfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .commandPool = s->command_pool,
        .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
        .commandBufferCount = 1
    };

    if(vkAllocateCommandBuffers(s->device, &cmd_buffer_cinfo, &s->command_buffer) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to create Vulkan command buffer.\n");
        return false;
    }

    return true;
}

Cbed_API void vulkan_draw_test_triangle(){
    Vulkan_State *s = &vulkan__state;

    u32 image_view_index = 0;
    u32 window_w = s->init_params.window_width;
    u32 window_h = s->init_params.window_height;

    VkClearValue clear_value = {
        .color = {{0.0f, 0.0f, 0.2f, 1.0f}}
    };

    VkRenderingAttachmentInfo color_attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = s->swapchain_image_views[image_view_index],
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
        .clearValue = clear_value
    };

#if 0
    VkRenderingAttachmentInfo depth_attachment_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
        .imageView = depthImageView,
        .imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL,
        .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
        .storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
        .clearValue = {.depthStencil = {1.0f,  0}}
    };
#endif

    VkRenderingInfo rendering_info = {
        .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
        .renderArea = {
            .extent = {.width = window_w, .height = window_h } // TODO: Why not swapchain extents?
        },
        .layerCount = 1,
        .colorAttachmentCount = 1,
        .pColorAttachments = &color_attachment_info,
        /*.pDepthAttachment = &depthAttachmentInfo*/
    };

    VkCommandBuffer command_buffer = s->command_buffer;

    VkCommandBufferBeginInfo cmd_begin_cinfo = {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        /*.flags = 0, // Optional*/
        /*.pInheritanceInfo = NULL; // Optional*/
    };

    if(vkBeginCommandBuffer(command_buffer, &cmd_begin_cinfo) != VK_SUCCESS) {
        fmt_msg_puts("ERROR: Unable to begin Vulkan command buffer.\n");
        return;
    }

    vkCmdBeginRendering(command_buffer, &rendering_info);

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, s->graphics_pipeline);

    VkViewport viewport = {
        .x = 0.0f,
        .y = 0.0f,
        .width = s->swapchain_extent.width,
        .height = s->swapchain_extent.height,
        .minDepth = 0.0f,
        .maxDepth = 1.0f
    };
    vkCmdSetViewport(command_buffer, 0, 1, &viewport);

    VkRect2D scissor = {
        .offset = {0, 0},
        .extent = s->swapchain_extent,
    };
    vkCmdSetScissor(command_buffer, 0, 1, &scissor);
    vkCmdDraw(command_buffer, 3, 1, 0, 0);

    vkCmdEndRendering(command_buffer); // TODO: Can this fail?

    if(vkEndCommandBuffer(command_buffer) != VK_SUCCESS){
        fmt_msg_puts("ERROR: Unable to end Vulkan command buffer.\n");
        return;
    }
}

Cbed_API void vulkan_destroy(){
    Vulkan_State *s = &vulkan__state;

    // TODO: Are all of these null if invalid/not created?
    if(s->command_pool){
        vkDestroyCommandPool(s->device, s->command_pool, NULL);
    }

    if(s->graphics_pipeline){
        vkDestroyPipeline(s->device, s->graphics_pipeline, NULL);
    }

    if(s->pipeline_layout){
        vkDestroyPipelineLayout(s->device, s->pipeline_layout, NULL);
    }

    vulkan__destroy_swapchain();

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
