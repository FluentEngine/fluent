#include <map>
#include <utils/hash.hpp>
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

namespace std
{
template <>
struct hash<fluent::RenderPassInfo>
{
    std::size_t operator()(const fluent::RenderPassInfo& info) const
    {
        std::size_t result = 0;

        fluent::hash_combine(result, info.color_attachment_count);
        fluent::hash_combine(result, info.width);
        fluent::hash_combine(result, info.height);
        fluent::hash_combine(result, info.depth_stencil);

        for (u32 i = 0; i < info.color_attachment_count; ++i)
        {
            fluent::hash_combine(result, info.color_attachments[ i ]->m_format);
            fluent::hash_combine(result, info.color_attachments[ i ]->m_image_view);
        }

        if (info.depth_stencil)
        {
            fluent::hash_combine(result, info.depth_stencil->m_format);
            fluent::hash_combine(result, info.depth_stencil->m_image_view);
        }

        return result;
    }
};

bool operator==(const fluent::RenderPassInfo& a, const fluent::RenderPassInfo& b)
{
    if (std::tie(a.color_attachment_count, a.width, a.height, a.depth_stencil) !=
        std::tie(b.color_attachment_count, b.width, b.height, b.depth_stencil))
    {
        return false;
    }

    u32 attachments_count = 0;

    for (u32 i = 0; i < a.color_attachment_count; ++i)
    {
        fluent::Image* at_a = a.color_attachments[ 0 ];
        fluent::Image* at_b = b.color_attachments[ 0 ];

        if (std::tie(at_a->m_image_view, at_a->m_format) != std::tie(at_b->m_image_view, at_b->m_format))
        {
            return false;
        }
    }

    return true;
}
} // namespace std

namespace fluent
{
static std::unordered_map<RenderPassInfo, u32> render_pass_infos;
static std::vector<VkRenderPass> render_passes;
static std::vector<VkFramebuffer> framebuffers;

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
        bool result =
            SDL_Vulkan_GetInstanceExtensions(( SDL_Window* ) get_app_window()->m_handle, &extensions_count, nullptr);
        FT_ASSERT(result);
#ifdef FLUENT_DEBUG
        extensions_count++;
#endif
    }
    else
    {
#ifdef FLUENT_DEBUG
        u32 sdl_extensions_count = extensions_count - 1;
        bool result = SDL_Vulkan_GetInstanceExtensions(
            ( SDL_Window* ) get_app_window()->m_handle, &sdl_extensions_count, extension_names);
        FT_ASSERT(result);
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

static inline bool format_has_depth_aspect(Format format)
{
    switch (format)
    {
    case Format::eD16Unorm:
    case Format::eD16UnormS8Uint:
    case Format::eD24UnormS8Uint:
    case Format::eD32Sfloat:
    case Format::eX8D24Unorm:
    case Format::eD32SfloatS8Uint:
        return true;
    default:
        return false;
    }
}

static inline bool format_has_stencil_aspect(Format format)
{
    switch (format)
    {
    case Format::eD16UnormS8Uint:
    case Format::eD24UnormS8Uint:
    case Format::eD32SfloatS8Uint:
    case Format::eS8Uint:
        return true;
    default:
        return false;
    }
}

static inline VkImageAspectFlags get_aspect_mask(Format format)
{
    VkImageAspectFlags aspect_mask = VK_IMAGE_ASPECT_COLOR_BIT;

    if (format_has_depth_aspect(format))
    {
        aspect_mask &= ~VK_IMAGE_ASPECT_COLOR_BIT;
        aspect_mask |= VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else if (format_has_stencil_aspect(format))
    {
        aspect_mask &= ~VK_IMAGE_ASPECT_COLOR_BIT;
        aspect_mask |= VK_IMAGE_ASPECT_STENCIL_BIT;
    }

    return aspect_mask;
}

static inline VkImageSubresourceRange get_image_subresource_range(const Image& image)
{
    VkImageSubresourceRange image_subresource_range{};
    image_subresource_range.aspectMask = get_aspect_mask(image.m_format);
    image_subresource_range.baseMipLevel = 0;
    image_subresource_range.levelCount = image.m_mip_level_count;
    image_subresource_range.baseArrayLayer = 0;
    image_subresource_range.layerCount = image.m_layer_count;

    return image_subresource_range;
}

VkPipelineStageFlags determine_pipeline_stage_flags(VkAccessFlags accessFlags, QueueType queueType)
{
    VkPipelineStageFlags flags = 0;

    switch (queueType)
    {
    case QueueType::eGraphics: {
        if ((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) != 0)
            flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;

        if ((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) != 0)
        {
            flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
            flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        if ((accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) != 0)
            flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        if ((accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) != 0)
            flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

        if (accessFlags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT))
            flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        break;
    }
    case QueueType::eCompute: {
        if ((accessFlags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT)) ||
            (accessFlags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT) ||
            (accessFlags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) ||
            (accessFlags &
             (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)))
            return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

        if ((accessFlags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) != 0)
            flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

        break;
    }
    case QueueType::eTransfer:
        return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
    default:
        break;
    }

    if (accessFlags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT)
        flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

    if (accessFlags & (VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT))
        flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;

    if (accessFlags & (VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT))
        flags |= VK_PIPELINE_STAGE_HOST_BIT;

    if (flags == 0)
        flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    return flags;
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
    device.m_vulkan_allocator = renderer.m_vulkan_allocator;
    device.m_instance = renderer.m_instance;
    device.m_physical_device = renderer.m_physical_device;

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device.m_physical_device, &queue_family_count, nullptr);
    VkQueueFamilyProperties queue_families[ queue_family_count ];
    vkGetPhysicalDeviceQueueFamilyProperties(device.m_physical_device, &queue_family_count, queue_families);

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
        device.m_physical_device, &device_create_info, device.m_vulkan_allocator, &device.m_logical_device));

    return device;
}

void destroy_device(Device& device)
{
    FT_ASSERT(device.m_logical_device);
    vkDestroyDevice(device.m_logical_device, device.m_vulkan_allocator);
}

void device_wait_idle(const Device& device)
{
    vkDeviceWaitIdle(device.m_logical_device);
}

Queue get_queue(const Device& device, const QueueDescription& description)
{
    Queue queue{};
    u32 index = find_queue_family_index(device.m_physical_device, description.queue_type);

    queue.m_family_index = index;
    queue.m_type = description.queue_type;
    vkGetDeviceQueue(device.m_logical_device, index, 0, &queue.m_queue);

    return queue;
}

void queue_wait_idle(const Queue& queue)
{
    vkQueueWaitIdle(queue.m_queue);
}

void queue_submit(const Queue& queue, const QueueSubmitDescription& description)
{
    VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSemaphore wait_semaphores[ description.wait_semaphore_count ];
    VkCommandBuffer command_buffers[ description.command_buffer_count ];
    VkSemaphore signal_semaphores[ description.signal_semaphore_count ];

    for (u32 i = 0; i < description.wait_semaphore_count; ++i)
    {
        wait_semaphores[ i ] = description.wait_semaphores[ i ].m_semaphore;
    }

    for (u32 i = 0; i < description.command_buffer_count; ++i)
    {
        command_buffers[ i ] = description.command_buffers[ i ].m_command_buffer;
    }

    for (u32 i = 0; i < description.signal_semaphore_count; ++i)
    {
        signal_semaphores[ i ] = description.signal_semaphores[ i ].m_semaphore;
    }

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = description.wait_semaphore_count;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
    submit_info.commandBufferCount = description.command_buffer_count;
    submit_info.pCommandBuffers = command_buffers;
    submit_info.signalSemaphoreCount = description.signal_semaphore_count;
    submit_info.pSignalSemaphores = signal_semaphores;

    vkQueueSubmit(
        queue.m_queue, 1, &submit_info, description.signal_fence ? description.signal_fence->m_fence : VK_NULL_HANDLE);
}

void queue_present(const Queue& queue, const QueuePresentDescription& description)
{
    VkSemaphore wait_semaphores[ description.wait_semaphore_count ];
    for (u32 i = 0; i < description.wait_semaphore_count; ++i)
    {
        wait_semaphores[ i ] = description.wait_semaphores[ i ].m_semaphore;
    }

    VkPresentInfoKHR present_info = {};

    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = description.wait_semaphore_count;
    present_info.pWaitSemaphores = wait_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &description.swapchain->m_swapchain;
    present_info.pImageIndices = &description.image_index;
    present_info.pResults = nullptr;

    vkQueuePresentKHR(queue.m_queue, &present_info);
}

Semaphore create_semaphore(const Device& device)
{
    Semaphore semaphore{};

    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;

    VK_ASSERT(vkCreateSemaphore(
        device.m_logical_device, &semaphore_create_info, device.m_vulkan_allocator, &semaphore.m_semaphore));

    return semaphore;
}

void destroy_semaphore(const Device& device, Semaphore& semaphore)
{
    FT_ASSERT(semaphore.m_semaphore);
    vkDestroySemaphore(device.m_logical_device, semaphore.m_semaphore, device.m_vulkan_allocator);
}

Fence create_fence(const Device& device)
{
    Fence fence{};

    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_ASSERT(vkCreateFence(device.m_logical_device, &fence_create_info, device.m_vulkan_allocator, &fence.m_fence));

    return fence;
}

void destroy_fence(const Device& device, Fence& fence)
{
    FT_ASSERT(fence.m_fence);
    vkDestroyFence(device.m_logical_device, fence.m_fence, device.m_vulkan_allocator);
}

void wait_for_fences(const Device& device, u32 count, Fence* fences)
{
    VkFence vk_fences[ count ];
    for (u32 i = 0; i < count; ++i)
    {
        vk_fences[ i ] = fences[ i ].m_fence;
    }

    vkWaitForFences(device.m_logical_device, count, vk_fences, true, std::numeric_limits<u64>::max());
}

void reset_fences(const Device& device, u32 count, Fence* fences)
{
    VkFence vk_fences[ count ];
    for (u32 i = 0; i < count; ++i)
    {
        vk_fences[ i ] = fences[ i ].m_fence;
    }

    vkResetFences(device.m_logical_device, count, vk_fences);
}

Swapchain create_swapchain(const Renderer& renderer, const Device& device, const SwapchainDescription& description)
{
    Swapchain swapchain{};

    SDL_Vulkan_CreateSurface(( SDL_Window* ) get_app_window()->m_handle, device.m_instance, &swapchain.m_surface);

    // find best present mode
    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device.m_physical_device, swapchain.m_surface, &present_mode_count, nullptr);
    VkPresentModeKHR present_modes[ present_mode_count ];
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device.m_physical_device, swapchain.m_surface, &present_mode_count, present_modes);

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
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.m_physical_device, swapchain.m_surface, &surface_capabilities);

    // determine swapchain size
    swapchain.m_width = std::clamp(
        description.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
    swapchain.m_height = std::clamp(
        description.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);

    swapchain.m_image_count =
        std::clamp(description.image_count, surface_capabilities.minImageCount, surface_capabilities.maxImageCount);

    /// find best surface format
    u32 surface_format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.m_physical_device, swapchain.m_surface, &surface_format_count, nullptr);
    VkSurfaceFormatKHR surface_formats[ surface_format_count ];
    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device.m_physical_device, swapchain.m_surface, &surface_format_count, surface_formats);

    VkSurfaceFormatKHR surface_format = surface_formats[ 0 ];
    for (u32 i = 0; i < surface_format_count; ++i)
    {
        if (surface_formats[ i ].format == VK_FORMAT_R8G8B8A8_UNORM ||
            surface_formats[ i ].format == VK_FORMAT_B8G8R8A8_UNORM)
            surface_format = surface_formats[ i ];
    }

    swapchain.m_format = util_from_vk_format(surface_format.format);

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
        device.m_logical_device, &swapchain_create_info, device.m_vulkan_allocator, &swapchain.m_swapchain));

    vkGetSwapchainImagesKHR(device.m_logical_device, swapchain.m_swapchain, &swapchain.m_image_count, nullptr);
    VkImage swapchain_images[ swapchain.m_image_count ];
    vkGetSwapchainImagesKHR(device.m_logical_device, swapchain.m_swapchain, &swapchain.m_image_count, swapchain_images);

    VkImageViewCreateInfo image_view_create_info{};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = surface_format.format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel = 0;
    image_view_create_info.subresourceRange.levelCount = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount = 1;

    swapchain.m_images = new Image[ swapchain.m_image_count ];
    for (u32 i = 0; i < swapchain.m_image_count; ++i)
    {
        image_view_create_info.image = swapchain_images[ i ];

        swapchain.m_images[ i ] = {};
        swapchain.m_images[ i ].m_image = swapchain_images[ i ];
        swapchain.m_images[ i ].m_width = swapchain.m_width;
        swapchain.m_images[ i ].m_height = swapchain.m_height;
        swapchain.m_images[ i ].m_format = swapchain.m_format;
        swapchain.m_images[ i ].m_sample_count = SampleCount::e1;
        swapchain.m_images[ i ].m_mip_level_count = 1;
        swapchain.m_images[ i ].m_layer_count = 1;

        VK_ASSERT(vkCreateImageView(
            device.m_logical_device, &image_view_create_info, device.m_vulkan_allocator,
            &swapchain.m_images[ i ].m_image_view));
    }

    return swapchain;
}

void destroy_swapchain(const Device& device, Swapchain& swapchain)
{
    // TODO: decide when to destroy all framebuffers, now will do it on swapchain recreate
    for (u32 i = 0; i < render_passes.size(); ++i)
    {
        vkDestroyFramebuffer(device.m_logical_device, framebuffers[ i ], device.m_vulkan_allocator);
        vkDestroyRenderPass(device.m_logical_device, render_passes[ i ], device.m_vulkan_allocator);
    }
    render_pass_infos.clear();
    render_passes.clear();
    framebuffers.clear();

    for (u32 i = 0; i < swapchain.m_image_count; ++i)
    {
        FT_ASSERT(swapchain.m_images[ i ].m_image_view);
        vkDestroyImageView(device.m_logical_device, swapchain.m_images[ i ].m_image_view, device.m_vulkan_allocator);
    }

    delete[] swapchain.m_images;
    FT_ASSERT(swapchain.m_swapchain);
    vkDestroySwapchainKHR(device.m_logical_device, swapchain.m_swapchain, device.m_vulkan_allocator);
    FT_ASSERT(swapchain.m_surface);
    vkDestroySurfaceKHR(device.m_instance, swapchain.m_surface, device.m_vulkan_allocator);
}

CommandPool create_command_pool(const Device& device, const CommandPoolDescription& description)
{
    CommandPool command_pool{};

    command_pool.m_queue_type = description.queue->m_type;

    VkCommandPoolCreateInfo command_pool_create_info{};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = nullptr;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = description.queue->m_family_index;

    VK_ASSERT(vkCreateCommandPool(
        device.m_logical_device, &command_pool_create_info, device.m_vulkan_allocator, &command_pool.m_command_pool));

    return command_pool;
}

void destroy_command_pool(const Device& device, CommandPool& command_pool)
{
    FT_ASSERT(command_pool.m_command_pool);
    vkDestroyCommandPool(device.m_logical_device, command_pool.m_command_pool, device.m_vulkan_allocator);
}

void allocate_command_buffers(
    const Device& device, const CommandPool& command_pool, u32 count, CommandBuffer* command_buffers)
{
    FT_ASSERT(command_buffers);

    VkCommandBuffer buffers[ count ] = {};

    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = nullptr;
    command_buffer_allocate_info.commandPool = command_pool.m_command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = count;

    VK_ASSERT(vkAllocateCommandBuffers(device.m_logical_device, &command_buffer_allocate_info, buffers));

    for (u32 i = 0; i < count; ++i)
    {
        command_buffers[ i ].m_command_buffer = buffers[ i ];
        command_buffers[ i ].m_queue_type = command_pool.m_queue_type;
    }
}

void free_command_buffers(
    const Device& device, const CommandPool& command_pool, u32 count, CommandBuffer* command_buffers)
{
    FT_ASSERT(command_buffers);

    VkCommandBuffer buffers[ count ];
    for (u32 i = 0; i < count; ++i)
    {
        buffers[ i ] = command_buffers[ i ].m_command_buffer;
    }

    vkFreeCommandBuffers(device.m_logical_device, command_pool.m_command_pool, count, buffers);
}

void begin_command_buffer(const CommandBuffer& command_buffer)
{
    VkCommandBufferBeginInfo command_buffer_begin_info{};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;

    VK_ASSERT(vkBeginCommandBuffer(command_buffer.m_command_buffer, &command_buffer_begin_info));
}

void end_command_buffer(const CommandBuffer& command_buffer)
{
    VK_ASSERT(vkEndCommandBuffer(command_buffer.m_command_buffer));
}

void acquire_next_image(
    const Device& device, const Swapchain& swapchain, const Semaphore& semaphore, const Fence& fence, u32& image_index)
{
    VkResult result = vkAcquireNextImageKHR(
        device.m_logical_device, swapchain.m_swapchain, std::numeric_limits<u64>::max(), semaphore.m_semaphore,
        fence.m_fence, &image_index);
}

void cmd_begin_render_pass(const Device& device, const CommandBuffer& command_buffer, const RenderPassInfo& info)
{
    VkRenderPass render_pass = VK_NULL_HANDLE;
    VkFramebuffer framebuffer = VK_NULL_HANDLE;

    auto it = render_pass_infos.find(info);

    if (it == render_pass_infos.end())
    {
        u32 attachments_count = info.color_attachment_count;

        VkAttachmentDescription attachment_descriptions[ info.color_attachment_count + 1 ];
        VkAttachmentReference color_attachment_references[ info.color_attachment_count ];
        VkAttachmentReference depth_attachment_reference{};

        for (u32 i = 0; i < info.color_attachment_count; ++i)
        {
            attachment_descriptions[ i ].flags = 0;
            attachment_descriptions[ i ].format = util_to_vk_format(info.color_attachments[ i ]->m_format);
            attachment_descriptions[ i ].samples = util_to_vk_sample_count(info.color_attachments[ i ]->m_sample_count);
            attachment_descriptions[ i ].loadOp = util_to_vk_load_op(info.color_attachment_load_ops[ i ]);
            attachment_descriptions[ i ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment_descriptions[ i ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment_descriptions[ i ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment_descriptions[ i ].initialLayout = info.color_image_layouts[ i ];
            attachment_descriptions[ i ].finalLayout = info.color_image_layouts[ i ];

            color_attachment_references[ i ].attachment = i;
            color_attachment_references[ i ].layout = info.color_image_layouts[ i ];
        }

        if (info.depth_stencil)
        {
            u32 i = info.color_attachment_count;
            attachment_descriptions[ i ].flags = 0;
            attachment_descriptions[ i ].format = util_to_vk_format(info.depth_stencil->m_format);
            attachment_descriptions[ i ].samples = util_to_vk_sample_count(info.depth_stencil->m_sample_count);
            attachment_descriptions[ i ].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            attachment_descriptions[ i ].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            attachment_descriptions[ i ].stencilLoadOp = util_to_vk_load_op(info.depth_stencil_load_op);
            attachment_descriptions[ i ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
            attachment_descriptions[ i ].initialLayout = info.depth_stencil_layout;
            attachment_descriptions[ i ].finalLayout = info.depth_stencil_layout;

            depth_attachment_reference.attachment = i;
            depth_attachment_reference.layout = info.depth_stencil_layout;
            // TODO: support for depth attachment
            attachments_count++;
        }

        // TODO: subpass setup from user code
        VkSubpassDescription subpass_description{};
        subpass_description.flags = 0;
        subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass_description.inputAttachmentCount = 0;
        subpass_description.pInputAttachments = nullptr;
        subpass_description.colorAttachmentCount = info.color_attachment_count;
        subpass_description.pColorAttachments = color_attachment_references;
        subpass_description.pResolveAttachments = nullptr;
        subpass_description.pDepthStencilAttachment = info.depth_stencil ? &depth_attachment_reference : nullptr;
        subpass_description.preserveAttachmentCount = 0;
        subpass_description.pPreserveAttachments = nullptr;

        VkRenderPassCreateInfo render_pass_create_info{};
        render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        render_pass_create_info.pNext = nullptr;
        render_pass_create_info.flags = 0;
        render_pass_create_info.attachmentCount = attachments_count;
        render_pass_create_info.pAttachments = attachment_descriptions;
        render_pass_create_info.subpassCount = 1;
        render_pass_create_info.pSubpasses = &subpass_description;
        render_pass_create_info.dependencyCount = 0;
        render_pass_create_info.pDependencies = nullptr;

        VK_ASSERT(vkCreateRenderPass(
            device.m_logical_device, &render_pass_create_info, device.m_vulkan_allocator, &render_pass));

        VkImageView image_views[ attachments_count ];
        for (u32 i = 0; i < info.color_attachment_count; ++i)
        {
            image_views[ i ] = info.color_attachments[ i ]->m_image_view;
        }

        if (info.depth_stencil)
        {
            image_views[ info.color_attachment_count ] = info.depth_stencil->m_image_view;
        }

        VkFramebufferCreateInfo framebuffer_create_info{};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = nullptr;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = render_pass;
        framebuffer_create_info.attachmentCount = attachments_count;
        framebuffer_create_info.pAttachments = image_views;
        framebuffer_create_info.width = info.width;
        framebuffer_create_info.height = info.height;
        framebuffer_create_info.layers = 1;

        VK_ASSERT(vkCreateFramebuffer(
            device.m_logical_device, &framebuffer_create_info, device.m_vulkan_allocator, &framebuffer));

        render_pass_infos[ info ] = render_passes.size();
        render_passes.push_back(render_pass);
        framebuffers.push_back(framebuffer);
    }
    else
    {
        render_pass = render_passes[ it->second ];
        framebuffer = framebuffers[ it->second ];
    }

    // TODO: clear values
    static VkClearValue clear_values[ MAX_ATTACHMENTS_COUNT + 1 ];
    for (u32 i = 0; i < info.color_attachment_count; ++i)
    {
        clear_values[ i ] = VkClearValue{};
        clear_values[ i ].color.float32[ 0 ] = info.color_clear_values[ i ].color[ 0 ];
        clear_values[ i ].color.float32[ 1 ] = info.color_clear_values[ i ].color[ 1 ];
        clear_values[ i ].color.float32[ 2 ] = info.color_clear_values[ i ].color[ 2 ];
        clear_values[ i ].color.float32[ 3 ] = info.color_clear_values[ i ].color[ 3 ];
    }

    if (info.depth_stencil)
    {
        clear_values[ info.color_attachment_count ] = VkClearValue{};
        clear_values[ info.color_attachment_count ].depthStencil.depth = info.depth_clear_value;
        clear_values[ info.color_attachment_count ].depthStencil.stencil = info.stencil_clear_value;
    }

    VkRenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = render_pass;
    render_pass_begin_info.framebuffer = framebuffer;
    render_pass_begin_info.renderArea.extent.width = info.width;
    render_pass_begin_info.renderArea.extent.height = info.height;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.clearValueCount = info.color_attachment_count + info.depth_stencil ? 1 : 0;
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(command_buffer.m_command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void cmd_end_render_pass(const CommandBuffer& command_buffer)
{
    vkCmdEndRenderPass(command_buffer.m_command_buffer);
}

void cmd_barrier(
    const CommandBuffer& command_buffer, u32 buffer_barriers_count, const BufferBarrier* buffer_barriers,
    u32 image_barriers_count, const ImageBarrier* image_barriers)
{
    // TODO: rewrite without heap allocation
    VkBufferMemoryBarrier* buffer_memory_barriers =
        buffer_barriers_count ? new VkBufferMemoryBarrier[ buffer_barriers_count ] : nullptr;
    VkImageMemoryBarrier* image_memory_barriers =
        image_barriers_count ? new VkImageMemoryBarrier[ image_barriers_count ] : nullptr;

    for (u32 i = 0; i < buffer_barriers_count; ++i)
    {
        // TODO: create buffer barriers
    }

    VkAccessFlags src_access = VkAccessFlags(0);
    VkAccessFlags dst_access = VkAccessFlags(0);

    // TODO: queues

    for (u32 i = 0; i < image_barriers_count; ++i)
    {
        FT_ASSERT(image_barriers[ i ].image);
        FT_ASSERT(image_barriers[ i ].src_queue);
        FT_ASSERT(image_barriers[ i ].dst_queue);

        image_memory_barriers[ i ].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barriers[ i ].pNext = nullptr;
        image_memory_barriers[ i ].srcAccessMask = image_barriers[ i ].src_access_mask;
        image_memory_barriers[ i ].dstAccessMask = image_barriers[ i ].dst_access_mask;
        image_memory_barriers[ i ].oldLayout = image_barriers[ i ].old_layout;
        image_memory_barriers[ i ].newLayout = image_barriers[ i ].new_layout;
        image_memory_barriers[ i ].srcQueueFamilyIndex = image_barriers[ i ].src_queue->m_family_index;
        image_memory_barriers[ i ].dstQueueFamilyIndex = image_barriers[ i ].dst_queue->m_family_index;
        image_memory_barriers[ i ].image = image_barriers[ i ].image->m_image;
        image_memory_barriers[ i ].subresourceRange = get_image_subresource_range(*image_barriers[ i ].image);

        src_access |= image_barriers[ i ].src_access_mask;
        dst_access |= image_barriers[ i ].dst_access_mask;
    }

    VkPipelineStageFlags src_stage = determine_pipeline_stage_flags(src_access, command_buffer.m_queue_type);
    VkPipelineStageFlags dst_stage = determine_pipeline_stage_flags(dst_access, command_buffer.m_queue_type);

    vkCmdPipelineBarrier(
        command_buffer.m_command_buffer, src_stage, dst_stage, 0, 0, nullptr, buffer_barriers_count,
        buffer_memory_barriers, image_barriers_count, image_memory_barriers);
};

} // namespace fluent