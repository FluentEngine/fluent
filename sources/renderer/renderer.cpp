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

} // namespace fluent