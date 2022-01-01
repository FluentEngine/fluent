#include <SDL_vulkan.h>
#include "core/window.hpp"
#include "core/application.hpp"
#include "renderer/renderer.hpp"

#ifdef FLUENT_DEBUG
#define VK_ASSERT(x)                                                                                                   \
    do                                                                                                                 \
    {                                                                                                                  \
        VkResult err = x;                                                                                              \
        if (err)                                                                                                       \
        {                                                                                                              \
            FT_ERROR("Detected Vulkan error: {}", err);                                                                \
            abort();                                                                                                   \
        }                                                                                                              \
    } while (0)
#else
#define VK_ASSERT(x) x
#endif

namespace fluent
{
static inline VkQueueFlagBits util_to_vk_queue_type(QueueType type)
{
    switch (type)
    {
    case QueueType::eGraphics:
        return VK_QUEUE_GRAPHICS_BIT;
    case QueueType::eCompute:
        return VK_QUEUE_COMPUTE_BIT;
    case QueueType::eTransfer:
        return VK_QUEUE_TRANSFER_BIT;
    default:
        return VkQueueFlagBits(-1);
    }
}

static VKAPI_ATTR VkBool32 VKAPI_CALL vulkan_debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData)
{
    if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
    {
        FT_TRACE("[ vulkan validation layer ] {}", pCallbackData->pMessage);
    }
    else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT)
    {
        FT_INFO("[ vulkan validation layer ] {}", pCallbackData->pMessage);
    }
    else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
    {
        FT_WARN("[ vulkan validation layer ] {}", pCallbackData->pMessage);
    }
    else if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
    {
        FT_ERROR("[ vulkan validation layer ] {}", pCallbackData->pMessage);
    }

    return VK_FALSE;
}

void create_debug_messenger(Renderer* renderer)
{
    VkDebugUtilsMessengerCreateInfoEXT debug_messenger_create_info{};
    debug_messenger_create_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debug_messenger_create_info.messageSeverity =
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
        VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debug_messenger_create_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                              VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debug_messenger_create_info.pfnUserCallback = vulkan_debug_callback;
    debug_messenger_create_info.pUserData = nullptr;

    vkCreateDebugUtilsMessengerEXT(
        renderer->m_instance, &debug_messenger_create_info, renderer->m_vulkan_allocator, &renderer->m_debug_messenger);
}

static inline void get_instance_extensions(u32& extensions_count, const char** extension_names)
{
    if (extension_names == nullptr)
    {
        FT_ASSERT(
            SDL_Vulkan_GetInstanceExtensions(( SDL_Window* ) get_app_window()->m_handle, &extensions_count, nullptr));
#ifdef FLUENT_DEBUG
        extensions_count++;
#endif
    }
    else
    {
#ifdef FLUENT_DEBUG
        u32 sdl_extensions_count = extensions_count - 1;
        FT_ASSERT(SDL_Vulkan_GetInstanceExtensions(
            ( SDL_Window* ) get_app_window()->m_handle, &sdl_extensions_count, extension_names));
        extension_names[ extensions_count - 1 ] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#else
        FT_ASSERT(SDL_Vulkan_GetInstanceExtensions(
            ( SDL_Window* ) get_app_window()->m_handle, &extensions_count, extension_names));
#endif
    }
}

static inline void get_instance_layers(u32& layers_count, const char** layer_names)
{
    if (layer_names == nullptr)
    {
#ifdef FLUENT_DEBUG
        layers_count = 1;
#else
        layers_count = 0;
#endif
    }
    else
    {
#ifdef FLUENT_DEBUG
        layer_names[ layers_count - 1 ] = "VK_LAYER_KHRONOS_validation";
#endif
    }
}

static inline u32 find_queue_family_index(VkPhysicalDevice physical_device, QueueType queue_type)
{
    u32 index = std::numeric_limits<u32>::max();

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);
    VkQueueFamilyProperties queue_families[ queue_family_count ];
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);

    VkQueueFlagBits target_queue_type = util_to_vk_queue_type(queue_type);

    // TODO: Find dedicated queue first
    for (u32 i = 0; i < queue_family_count; ++i)
    {
        if (queue_families[ i ].queueFlags & target_queue_type)
        {
            index = i;
            break;
        }
    }

    // TODO: Rewrite more elegant way
    FT_ASSERT(index != std::numeric_limits<u32>::max());

    return index;
}

Renderer create_renderer(const RendererDescription& description)
{
    Renderer renderer{};
    renderer.m_vulkan_allocator = description.vulkan_allocator;

    volkInitialize();

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = get_app_name();
    app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.pEngineName = "Fluent";
    app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.apiVersion = VK_API_VERSION_1_2;

    u32 extensions_count = 0;
    get_instance_extensions(extensions_count, nullptr);
    const char* extensions[ extensions_count ];
    get_instance_extensions(extensions_count, extensions);

    u32 layers_count = 0;
    get_instance_layers(layers_count, nullptr);
    const char* layers[ layers_count ];
    get_instance_layers(layers_count, layers);

    VkInstanceCreateInfo instance_create_info{};
    instance_create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext = nullptr;
    instance_create_info.pApplicationInfo = &app_info;
    instance_create_info.enabledLayerCount = layers_count;
    instance_create_info.ppEnabledLayerNames = layers;
    instance_create_info.enabledExtensionCount = extensions_count;
    instance_create_info.ppEnabledExtensionNames = extensions;
    instance_create_info.flags = 0;

    VK_ASSERT(vkCreateInstance(&instance_create_info, renderer.m_vulkan_allocator, &renderer.m_instance));

    volkLoadInstance(renderer.m_instance);

#ifdef FLUENT_DEBUG
    create_debug_messenger(&renderer);
#endif

    // pick physical device
    renderer.m_physical_device = VK_NULL_HANDLE;
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(renderer.m_instance, &device_count, nullptr);
    FT_ASSERT(device_count != 0);
    VkPhysicalDevice physical_devices[ device_count ];
    vkEnumeratePhysicalDevices(renderer.m_instance, &device_count, physical_devices);

    renderer.m_physical_device = physical_devices[ 0 ];

    // select best physical device
    for (u32 i = 0; i < device_count; ++i)
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(physical_devices[ i ], &deviceProperties);
        vkGetPhysicalDeviceFeatures(physical_devices[ i ], &deviceFeatures);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            renderer.m_physical_device = physical_devices[ i ];
            break;
        }
    }

    return renderer;
}

void destroy_renderer(Renderer& renderer)
{
#ifdef FLUENT_DEBUG
    vkDestroyDebugUtilsMessengerEXT(renderer.m_instance, renderer.m_debug_messenger, renderer.m_vulkan_allocator);
#endif
    vkDestroyInstance(renderer.m_instance, renderer.m_vulkan_allocator);
}

Device create_device(const Renderer& renderer, const DeviceDescription& description)
{
    Device device{};

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(renderer.m_physical_device, &queue_family_count, nullptr);
    VkQueueFamilyProperties queue_families[ queue_family_count ];
    vkGetPhysicalDeviceQueueFamilyProperties(renderer.m_physical_device, &queue_family_count, queue_families);

    VkDeviceQueueCreateInfo queue_create_infos[ queue_family_count ];
    f32 queue_priority = 1.0f;

    for (u32 i = 0; i < queue_family_count; ++i)
    {
        queue_create_infos[ i ] = {};
        queue_create_infos[ i ].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[ i ].pNext = nullptr;
        queue_create_infos[ i ].flags = 0;
        queue_create_infos[ i ].queueCount = 1;
        queue_create_infos[ i ].pQueuePriorities = &queue_priority;
        queue_create_infos[ i ].queueFamilyIndex = i;
    }

    u32 device_extension_count = 1;
    const char* device_extensions[ device_extension_count ] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext = nullptr;
    device_create_info.flags = 0;
    device_create_info.queueCreateInfoCount = queue_family_count;
    device_create_info.pQueueCreateInfos = queue_create_infos;
    device_create_info.enabledLayerCount = 0;
    device_create_info.ppEnabledLayerNames = nullptr;
    device_create_info.enabledExtensionCount = device_extension_count;
    device_create_info.ppEnabledExtensionNames = device_extensions;
    device_create_info.pEnabledFeatures = nullptr;

    VK_ASSERT(vkCreateDevice(
        renderer.m_physical_device, &device_create_info, renderer.m_vulkan_allocator, &device.m_logical_device));

    return device;
}

void destroy_device(const Renderer& renderer, Device& device)
{
    vkDestroyDevice(device.m_logical_device, renderer.m_vulkan_allocator);
}

Queue get_queue(const Renderer& renderer, const Device& device, const QueueDescription& description)
{
    Queue queue{};
    u32 index = find_queue_family_index(renderer.m_physical_device, description.queue_type);

    queue.m_family_index = index;
    vkGetDeviceQueue(device.m_logical_device, index, 0, &queue.m_queue);

    return queue;
}

Swapchain create_swapchain(const Renderer& renderer, const Device& device, const SwapchainDescription& description)
{
    Swapchain swapchain{};
    swapchain.m_width = description.width;
    swapchain.m_height = description.height;

    SDL_Vulkan_CreateSurface(( SDL_Window* ) get_app_window()->m_handle, renderer.m_instance, &swapchain.m_surface);

    // find best present mode
    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        renderer.m_physical_device, swapchain.m_surface, &present_mode_count, nullptr);
    VkPresentModeKHR present_modes[ present_mode_count ];
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        renderer.m_physical_device, swapchain.m_surface, &present_mode_count, present_modes);

    swapchain.m_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

    for (u32 i = 0; i < present_mode_count; ++i)
    {
        if (present_modes[ i ] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchain.m_present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    // determine present image count
    VkSurfaceCapabilitiesKHR surface_capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(renderer.m_physical_device, swapchain.m_surface, &surface_capabilities);

    swapchain.m_image_count =
        std::clamp(description.image_count, surface_capabilities.minImageCount, surface_capabilities.maxImageCount);

    /// find best surface format
    u32 surface_format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        renderer.m_physical_device, swapchain.m_surface, &surface_format_count, nullptr);
    VkSurfaceFormatKHR surface_formats[ surface_format_count ];
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        renderer.m_physical_device, swapchain.m_surface, &surface_format_count, surface_formats);

    VkSurfaceFormatKHR surface_format = surface_formats[ 0 ];
    for (u32 i = 0; i < surface_format_count; ++i)
    {
        if (surface_formats[ i ].format == VK_FORMAT_R8G8B8A8_UNORM ||
            surface_formats[ i ].format == VK_FORMAT_B8G8R8A8_UNORM)
            surface_format = surface_formats[ i ];
    }

    swapchain.m_format = surface_format.format;

    /// fins swapchain pretransform
    VkSurfaceTransformFlagBitsKHR pre_transform;
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
    {
        pre_transform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
    }
    else
    {
        pre_transform = surface_capabilities.currentTransform;
    }

    VkSwapchainCreateInfoKHR swapchain_create_info{};
    swapchain_create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext = nullptr;
    swapchain_create_info.flags = 0;
    swapchain_create_info.surface = swapchain.m_surface;
    swapchain_create_info.minImageCount = swapchain.m_image_count;
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent.width = swapchain.m_width;
    swapchain_create_info.imageExtent.height = swapchain.m_height;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 1;
    swapchain_create_info.pQueueFamilyIndices = &description.queue->m_family_index;
    swapchain_create_info.preTransform = pre_transform;
    // TODO: choose composite alpha according to caps
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = swapchain.m_present_mode;
    swapchain_create_info.clipped = true;
    swapchain_create_info.oldSwapchain = nullptr;

    VK_ASSERT(vkCreateSwapchainKHR(
        device.m_logical_device, &swapchain_create_info, renderer.m_vulkan_allocator, &swapchain.m_swapchain));

    return swapchain;
}

void destroy_swapchain(const Renderer& renderer, const Device& device, Swapchain& swapchain)
{
    vkDestroySwapchainKHR(device.m_logical_device, swapchain.m_swapchain, renderer.m_vulkan_allocator);
    vkDestroySurfaceKHR(renderer.m_instance, swapchain.m_surface, renderer.m_vulkan_allocator);
}

} // namespace fluent