#include <algorithm>
#include <SDL_vulkan.h>
#include "tinyimageformat_apis.h"
#include "utils/utils.hpp"
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

VkFormat to_vk_format(Format format)
{
    return static_cast<VkFormat>(TinyImageFormat_ToVkFormat(static_cast<TinyImageFormat>(format)));
}

Format from_vk_format(VkFormat format)
{
    return static_cast<Format>(TinyImageFormat_FromVkFormat(static_cast<TinyImageFormat_VkFormat>(format)));
}

VkSampleCountFlagBits to_vk_sample_count(SampleCount sample_count)
{
    switch (sample_count)
    {
    case SampleCount::e1:
        return VK_SAMPLE_COUNT_1_BIT;
    case SampleCount::e2:
        return VK_SAMPLE_COUNT_2_BIT;
    case SampleCount::e4:
        return VK_SAMPLE_COUNT_4_BIT;
    case SampleCount::e8:
        return VK_SAMPLE_COUNT_8_BIT;
    case SampleCount::e16:
        return VK_SAMPLE_COUNT_16_BIT;
    case SampleCount::e32:
        return VK_SAMPLE_COUNT_32_BIT;
    default:
        FT_ASSERT(false);
        return VkSampleCountFlagBits(-1);
    }
}

VkAttachmentLoadOp to_vk_load_op(AttachmentLoadOp load_op)
{
    switch (load_op)
    {
    case AttachmentLoadOp::eClear:
        return VK_ATTACHMENT_LOAD_OP_CLEAR;
    case AttachmentLoadOp::eDontCare:
        return VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    case AttachmentLoadOp::eLoad:
        return VK_ATTACHMENT_LOAD_OP_LOAD;
    default:
        FT_ASSERT(false);
        return VkAttachmentLoadOp(-1);
    }
}

static inline VkQueueFlagBits to_vk_queue_type(QueueType type)
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
        FT_ASSERT(false);
        return VkQueueFlagBits(-1);
    }
}

static inline VkVertexInputRate to_vk_vertex_input_rate(VertexInputRate input_rate)
{
    switch (input_rate)
    {
    case VertexInputRate::eVertex:
        return VK_VERTEX_INPUT_RATE_VERTEX;
    case VertexInputRate::eInstance:
        return VK_VERTEX_INPUT_RATE_INSTANCE;
    default:
        FT_ASSERT(false);
        return VkVertexInputRate(-1);
    }
}

static inline VkCullModeFlagBits to_vk_cull_mode(CullMode cull_mode)
{
    switch (cull_mode)
    {
    case CullMode::eBack:
        return VK_CULL_MODE_BACK_BIT;
    case CullMode::eFront:
        return VK_CULL_MODE_FRONT_BIT;
    case CullMode::eNone:
        return VK_CULL_MODE_NONE;
    default:
        FT_ASSERT(false);
        return VkCullModeFlagBits(-1);
    }
}

static inline VkFrontFace to_vk_front_face(FrontFace front_face)
{
    switch (front_face)
    {
    case FrontFace::eClockwise:
        return VK_FRONT_FACE_CLOCKWISE;
    case FrontFace::eCounterClockwise:
        return VK_FRONT_FACE_COUNTER_CLOCKWISE;
    default:
        FT_ASSERT(false);
        return VkFrontFace(-1);
    }
}

static inline VkCompareOp to_vk_compare_op(CompareOp op)
{
    switch (op)
    {
    case CompareOp::eNever:
        return VK_COMPARE_OP_NEVER;
    case CompareOp::eLess:
        return VK_COMPARE_OP_LESS;
    case CompareOp::eEqual:
        return VK_COMPARE_OP_EQUAL;
    case CompareOp::eLessOrEqual:
        return VK_COMPARE_OP_LESS_OR_EQUAL;
    case CompareOp::eGreater:
        return VK_COMPARE_OP_GREATER;
    case CompareOp::eNotEqual:
        return VK_COMPARE_OP_NOT_EQUAL;
    case CompareOp::eGreaterOrEqual:
        return VK_COMPARE_OP_GREATER_OR_EQUAL;
    case CompareOp::eAlways:
        return VK_COMPARE_OP_ALWAYS;
    default:
        FT_ASSERT(false);
        return VkCompareOp(-1);
    }
}

static inline VkShaderStageFlagBits to_vk_shader_stage(ShaderStage shader_stage)
{
    switch (shader_stage)
    {
    case ShaderStage::eVertex:
        return VK_SHADER_STAGE_VERTEX_BIT;
    case ShaderStage::eTessellationControl:
        return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
    case ShaderStage::eTessellationEvaluation:
        return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
    case ShaderStage::eGeometry:
        return VK_SHADER_STAGE_GEOMETRY_BIT;
    case ShaderStage::eFragment:
        return VK_SHADER_STAGE_FRAGMENT_BIT;
    case ShaderStage::eCompute:
        return VK_SHADER_STAGE_COMPUTE_BIT;
    case ShaderStage::eAllGraphics:
        return VK_SHADER_STAGE_ALL_GRAPHICS;
    case ShaderStage::eAll:
        return VK_SHADER_STAGE_ALL;
    default:
        FT_ASSERT(false);
        return VkShaderStageFlagBits(-1);
    }
}

static inline VkPipelineBindPoint to_vk_pipeline_bind_point(PipelineType type)
{
    switch (type)
    {
    case PipelineType::eCompute:
        return VK_PIPELINE_BIND_POINT_COMPUTE;
    case PipelineType::eGraphics:
        return VK_PIPELINE_BIND_POINT_GRAPHICS;
    default:
        FT_ASSERT(false);
        return VkPipelineBindPoint(-1);
    }
}

static inline VkDescriptorType to_vk_descriptor_type(DescriptorType descriptor_type)
{
    switch (descriptor_type)
    {
    case DescriptorType::eSampler:
        return VK_DESCRIPTOR_TYPE_SAMPLER;
    case DescriptorType::eCombinedImageSampler:
        return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    case DescriptorType::eSampledImage:
        return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    case DescriptorType::eStorageImage:
        return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    case DescriptorType::eUniformTexelBuffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
    case DescriptorType::eStorageTexelBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
    case DescriptorType::eUniformBuffer:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    case DescriptorType::eStorageBuffer:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    case DescriptorType::eUniformBufferDynamic:
        return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
    case DescriptorType::eStorageBufferDynamic:
        return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC;
    case DescriptorType::eInputAttachment:
        return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    default:
        FT_ASSERT(false);
        return VkDescriptorType(-1);
    }
}

VkFilter to_vk_filter(Filter filter)
{
    switch (filter)
    {
    case Filter::eLinear:
        return VK_FILTER_LINEAR;
    case Filter::eNearest:
        return VK_FILTER_NEAREST;
    default:
        FT_ASSERT(false);
        return VkFilter(-1);
    }
}

VkSamplerMipmapMode to_vk_sampler_mipmap_mode(SamplerMipmapMode mode)
{
    switch (mode)
    {
    case SamplerMipmapMode::eNearest:
        return VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_NEAREST;
    case SamplerMipmapMode::eLinear:
        return VkSamplerMipmapMode::VK_SAMPLER_MIPMAP_MODE_LINEAR;
    default:
        FT_ASSERT(false);
        return VkSamplerMipmapMode(-1);
    }
}

VkSamplerAddressMode to_vk_sampler_address_mode(SamplerAddressMode mode)
{
    switch (mode)
    {
    case SamplerAddressMode::eRepeat:
        return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_REPEAT;
    case SamplerAddressMode::eMirroredRepeat:
        return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
    case SamplerAddressMode::eClampToEdge:
        return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    case SamplerAddressMode::eClampToBorder:
        return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
    case SamplerAddressMode::eMirrorClampToEdge:
        return VkSamplerAddressMode::VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
    default:
        FT_ASSERT(false);
        return VkSamplerAddressMode(-1);
    }
}

static inline VkAccessFlags determine_access_flags(ResourceState resource_state)
{
    VkAccessFlags access_flags = 0;

    if (resource_state & eStorage)
        access_flags |= (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

    if (resource_state & eColorAttachment)
        access_flags |= (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    if (resource_state & eDepthStencilAttachment)
        access_flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    if (resource_state & eDepthStencilReadOnly)
        access_flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    if (resource_state & eShaderReadOnly)
        access_flags |= VK_ACCESS_SHADER_READ_BIT;

    if (resource_state & eTransferSrc)
        access_flags |= VK_ACCESS_TRANSFER_READ_BIT;

    if (resource_state & eTransferDst)
        access_flags |= VK_ACCESS_TRANSFER_WRITE_BIT;

    if (resource_state & ePresent)
        access_flags |= VK_ACCESS_MEMORY_READ_BIT;

    return access_flags;
}

static inline VkPipelineStageFlags determine_pipeline_stage_flags(VkAccessFlags access_flags, QueueType queue_type)
{
    // TODO: FIX THIS
    VkPipelineStageFlags flags = 0;

    switch (queue_type)
    {
    case QueueType::eGraphics: {
        if (access_flags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT))
        {
            flags |= VK_PIPELINE_STAGE_VERTEX_INPUT_BIT;
        }

        if (access_flags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT))
        {
            flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT;
            flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        if (access_flags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT)
        {
            flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        if (access_flags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT))
        {
            flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        }

        if (access_flags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT))
        {
            flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        }
        break;
    }
    case QueueType::eCompute: {
    }
    }

    if (access_flags & (VK_ACCESS_INDEX_READ_BIT | VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT) ||
        access_flags & VK_ACCESS_INPUT_ATTACHMENT_READ_BIT ||
        access_flags & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT) ||
        access_flags & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT))
        return VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;

    if (access_flags & (VK_ACCESS_UNIFORM_READ_BIT | VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT))
        flags |= VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;

    if (access_flags & VK_ACCESS_INDIRECT_COMMAND_READ_BIT)
        flags |= VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT;

    if (access_flags & (VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT))
        flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;

    if (access_flags & (VK_ACCESS_HOST_READ_BIT | VK_ACCESS_HOST_WRITE_BIT))
        flags |= VK_PIPELINE_STAGE_HOST_BIT;

    if (flags == 0)
        flags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

    return flags;
}

static inline VkImageLayout determine_image_layout(ResourceState resource_state)
{
    if (resource_state & eStorage)
        return VK_IMAGE_LAYOUT_GENERAL;

    if (resource_state & eColorAttachment)
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    if (resource_state & eDepthStencilAttachment)
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    if (resource_state & eDepthStencilReadOnly)
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    if (resource_state & eShaderReadOnly)
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (resource_state & ePresent)
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    if (resource_state & eTransferSrc)
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    if (resource_state & eTransferDst)
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    return VK_IMAGE_LAYOUT_UNDEFINED;
}

static inline VmaMemoryUsage determine_vma_memory_usage(ResourceState resource_state)
{
    // TODO: determine memory usage
    VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;

    // can be used as source in transfer operation
    if (resource_state & ResourceState::eTransferSrc)
    {
        memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    }

    return memory_usage;
}

static inline VkBufferUsageFlags determine_vk_buffer_usage(ResourceState resource_state, DescriptorType descriptor_type)
{
    // TODO: determine buffer usage flags
    VkBufferUsageFlags buffer_usage = VkBufferUsageFlags(0);

    if (descriptor_type & DescriptorType::eVertexBuffer)
    {
        buffer_usage |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    }

    if (descriptor_type & DescriptorType::eIndexBuffer)
    {
        buffer_usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    }

    if (descriptor_type & DescriptorType::eUniformBuffer)
    {
        buffer_usage |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    }

    if (resource_state & ResourceState::eTransferSrc)
    {
        buffer_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }

    if (resource_state & ResourceState::eTransferDst)
    {
        buffer_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    return buffer_usage;
}

static inline VkImageUsageFlags determine_vk_image_usage(ResourceState resource_state, DescriptorType descriptor_type)
{
    VkImageUsageFlags image_usage = VkImageUsageFlags(0);

    if (descriptor_type & DescriptorType::eSampledImage)
    {
        image_usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
    }

    if (descriptor_type & DescriptorType::eStorageImage)
    {
        image_usage |= VK_IMAGE_USAGE_STORAGE_BIT;
    }

    if (descriptor_type & DescriptorType::eInputAttachment)
    {
        image_usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    }

    if (resource_state & ResourceState::eColorAttachment)
    {
        image_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    if (resource_state & ResourceState::eDepthStencilAttachment)
    {
        image_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }

    if (resource_state & ResourceState::eDepthStencilReadOnly)
    {
        // TODO: ?
        image_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    }

    if (resource_state & ResourceState::eTransferSrc)
    {
        image_usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
    }

    if (resource_state & ResourceState::eTransferDst)
    {
        image_usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    return image_usage;
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
        renderer->instance, &debug_messenger_create_info, renderer->vulkan_allocator, &renderer->debug_messenger);
}

static inline void get_instance_extensions(u32& extensions_count, const char** extension_names)
{
    if (extension_names == nullptr)
    {
        b32 result =
            SDL_Vulkan_GetInstanceExtensions(( SDL_Window* ) get_app_window()->handle, &extensions_count, nullptr);
#ifdef FLUENT_DEBUG
        extensions_count++;
#endif
    }
    else
    {
#ifdef FLUENT_DEBUG
        u32 sdl_extensions_count = extensions_count - 1;
        b32 result = SDL_Vulkan_GetInstanceExtensions(
            ( SDL_Window* ) get_app_window()->handle, &sdl_extensions_count, extension_names);
        FT_ASSERT(result);
        extension_names[ extensions_count - 1 ] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#else
        b32 result = SDL_Vulkan_GetInstanceExtensions(
            ( SDL_Window* ) get_app_window()->handle, &extensions_count, extension_names);
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
    VkQueueFamilyProperties* queue_families =
        ( VkQueueFamilyProperties* ) alloca(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, queue_families);

    VkQueueFlagBits target_queue_type = to_vk_queue_type(queue_type);

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

static inline b32 format_has_depth_aspect(Format format)
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

static inline b32 format_has_stencil_aspect(Format format)
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
    image_subresource_range.aspectMask = get_aspect_mask(image.format);
    image_subresource_range.baseMipLevel = 0;
    image_subresource_range.levelCount = image.mip_level_count;
    image_subresource_range.baseArrayLayer = 0;
    image_subresource_range.layerCount = image.layer_count;

    return image_subresource_range;
}

VkImageSubresourceLayers get_image_subresource_layers(const Image& image)
{
    auto subresourceRange = get_image_subresource_range(image);
    return VkImageSubresourceLayers{ subresourceRange.aspectMask, subresourceRange.baseMipLevel,
                                     subresourceRange.baseArrayLayer, subresourceRange.layerCount };
}

Renderer create_renderer(const RendererDesc& desc)
{
    Renderer renderer{};
    renderer.vulkan_allocator = desc.vulkan_allocator;

    volkInitialize();

    VkApplicationInfo app_info{};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext = nullptr;
    app_info.pApplicationName = get_app_name();
    app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.pEngineName = "Fluent";
    app_info.engineVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.apiVersion = FLUENT_VULKAN_API_VERSION;

    u32 extensions_count = 0;
    get_instance_extensions(extensions_count, nullptr);
    const char** extensions = ( const char** ) alloca(extensions_count * sizeof(const char*));
    get_instance_extensions(extensions_count, extensions);

    u32 layers_count = 0;
    get_instance_layers(layers_count, nullptr);
    const char** layers = ( const char** ) alloca(layers_count * sizeof(const char*));
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

    VK_ASSERT(vkCreateInstance(&instance_create_info, renderer.vulkan_allocator, &renderer.instance));

    volkLoadInstance(renderer.instance);

#ifdef FLUENT_DEBUG
    create_debug_messenger(&renderer);
#endif

    // pick physical device
    renderer.physical_device = VK_NULL_HANDLE;
    u32 device_count = 0;
    vkEnumeratePhysicalDevices(renderer.instance, &device_count, nullptr);
    FT_ASSERT(device_count != 0);
    VkPhysicalDevice* physical_devices = ( VkPhysicalDevice* ) alloca(device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(renderer.instance, &device_count, physical_devices);

    renderer.physical_device = physical_devices[ 0 ];

    // select best physical device
    for (u32 i = 0; i < device_count; ++i)
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceProperties(physical_devices[ i ], &deviceProperties);
        vkGetPhysicalDeviceFeatures(physical_devices[ i ], &deviceFeatures);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            renderer.physical_device = physical_devices[ i ];
            break;
        }
    }

    return renderer;
}

void destroy_renderer(Renderer& renderer)
{
#ifdef FLUENT_DEBUG
    vkDestroyDebugUtilsMessengerEXT(renderer.instance, renderer.debug_messenger, renderer.vulkan_allocator);
#endif
    vkDestroyInstance(renderer.instance, renderer.vulkan_allocator);
}

void write_to_staging_buffer(const Device& device, u32 size, const void* data)
{
    // TODO: decide what to do if no space left
    FT_ASSERT(size < (device.staging_buffer.buffer.size - device.staging_buffer.memory_used));
    u8* dst = ( u8* ) device.staging_buffer.buffer.mapped_memory + device.staging_buffer.memory_used;
    std::memcpy(dst, data, size);
}

void map_memory(const Device& device, Buffer& buffer)
{
    FT_ASSERT(buffer.mapped_memory == nullptr);
    vmaMapMemory(device.memory_allocator, buffer.allocation, &buffer.mapped_memory);
}

void unmap_memory(const Device& device, Buffer& buffer)
{
    FT_ASSERT(buffer.mapped_memory);
    vmaUnmapMemory(device.memory_allocator, buffer.allocation);
}

Device create_device(const Renderer& renderer, const DeviceDesc& desc)
{
    FT_ASSERT(desc.frame_in_use_count > 0);

    Device device{};
    device.vulkan_allocator = renderer.vulkan_allocator;
    device.instance = renderer.instance;
    device.physical_device = renderer.physical_device;

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device.physical_device, &queue_family_count, nullptr);
    VkQueueFamilyProperties* queue_families =
        ( VkQueueFamilyProperties* ) alloca(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device.physical_device, &queue_family_count, queue_families);

    VkDeviceQueueCreateInfo queue_create_infos[ static_cast<int>(QueueType::eLast) ];
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

    static const u32 device_extension_count = 1;
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

    VK_ASSERT(
        vkCreateDevice(device.physical_device, &device_create_info, device.vulkan_allocator, &device.logical_device));

    VmaAllocatorCreateInfo vma_allocator_create_info{};
    vma_allocator_create_info.instance = device.instance;
    vma_allocator_create_info.physicalDevice = device.physical_device;
    vma_allocator_create_info.device = device.logical_device;
    vma_allocator_create_info.flags = 0;
    vma_allocator_create_info.pAllocationCallbacks = device.vulkan_allocator;
    vma_allocator_create_info.frameInUseCount = desc.frame_in_use_count;
    vma_allocator_create_info.vulkanApiVersion = FLUENT_VULKAN_API_VERSION;

    VK_ASSERT(vmaCreateAllocator(&vma_allocator_create_info, &device.memory_allocator));

    // create staging buffer
    BufferDesc buffer_desc{};
    buffer_desc.size = STAGING_BUFFER_SIZE;
    buffer_desc.resource_state = ResourceState::eTransferSrc;
    device.staging_buffer.memory_used = 0;
    device.staging_buffer.buffer = create_buffer(device, buffer_desc);
    map_memory(device, device.staging_buffer.buffer);

    // TODO: move it to transfer queue
    QueueDesc queue_desc{};
    queue_desc.queue_type = QueueType::eGraphics;

    device.upload_queue = get_queue(device, queue_desc);

    CommandPoolDesc command_pool_desc{};
    command_pool_desc.queue = &device.upload_queue;
    device.command_pool = create_command_pool(device, command_pool_desc);

    allocate_command_buffers(device, device.command_pool, 1, &device.upload_command_buffer);

    static constexpr u32 pool_size_count = 11;
    VkDescriptorPoolSize pool_sizes[ pool_size_count ] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1024 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1024 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1024 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1024 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1024 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1024 },
    };

    VkDescriptorPoolCreateInfo descriptor_pool_create_info{};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext = nullptr;
    descriptor_pool_create_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_create_info.maxSets = 2048 * pool_size_count;
    descriptor_pool_create_info.poolSizeCount = pool_size_count;
    descriptor_pool_create_info.pPoolSizes = pool_sizes;

    VK_ASSERT(vkCreateDescriptorPool(
        device.logical_device, &descriptor_pool_create_info, device.vulkan_allocator, &device.descriptor_pool));

    return device;
}

void destroy_device(Device& device)
{
    FT_ASSERT(device.descriptor_pool);
    FT_ASSERT(device.memory_allocator);
    FT_ASSERT(device.logical_device);
    vkDestroyDescriptorPool(device.logical_device, device.descriptor_pool, device.vulkan_allocator);
    free_command_buffers(device, device.command_pool, 1, &device.upload_command_buffer);
    destroy_command_pool(device, device.command_pool);
    unmap_memory(device, device.staging_buffer.buffer);
    destroy_buffer(device, device.staging_buffer.buffer);
    vmaDestroyAllocator(device.memory_allocator);
    vkDestroyDevice(device.logical_device, device.vulkan_allocator);
}

void device_wait_idle(const Device& device)
{
    vkDeviceWaitIdle(device.logical_device);
}

Queue get_queue(const Device& device, const QueueDesc& desc)
{
    Queue queue{};
    u32 index = find_queue_family_index(device.physical_device, desc.queue_type);

    queue.family_index = index;
    queue.type = desc.queue_type;
    vkGetDeviceQueue(device.logical_device, index, 0, &queue.queue);

    return queue;
}

void queue_wait_idle(const Queue& queue)
{
    vkQueueWaitIdle(queue.queue);
}

void queue_submit(const Queue& queue, const QueueSubmitDesc& desc)
{
    VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSemaphore* wait_semaphores = ( VkSemaphore* ) alloca(desc.wait_semaphore_count * sizeof(VkSemaphore));
    VkCommandBuffer* command_buffers = ( VkCommandBuffer* ) alloca(desc.command_buffer_count * sizeof(VkCommandBuffer));
    VkSemaphore* signal_semaphores = ( VkSemaphore* ) alloca(desc.signal_semaphore_count * sizeof(VkSemaphore));

    for (u32 i = 0; i < desc.wait_semaphore_count; ++i)
    {
        wait_semaphores[ i ] = desc.wait_semaphores[ i ].semaphore;
    }

    for (u32 i = 0; i < desc.command_buffer_count; ++i)
    {
        command_buffers[ i ] = desc.command_buffers[ i ].command_buffer;
    }

    for (u32 i = 0; i < desc.signal_semaphore_count; ++i)
    {
        signal_semaphores[ i ] = desc.signal_semaphores[ i ].semaphore;
    }

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext = nullptr;
    submit_info.waitSemaphoreCount = desc.wait_semaphore_count;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
    submit_info.commandBufferCount = desc.command_buffer_count;
    submit_info.pCommandBuffers = command_buffers;
    submit_info.signalSemaphoreCount = desc.signal_semaphore_count;
    submit_info.pSignalSemaphores = signal_semaphores;

    vkQueueSubmit(queue.queue, 1, &submit_info, desc.signal_fence ? desc.signal_fence->fence : VK_NULL_HANDLE);
}

void queue_present(const Queue& queue, const QueuePresentDesc& desc)
{
    VkSemaphore* wait_semaphores = ( VkSemaphore* ) alloca(desc.wait_semaphore_count * sizeof(VkSemaphore));

    for (u32 i = 0; i < desc.wait_semaphore_count; ++i)
    {
        wait_semaphores[ i ] = desc.wait_semaphores[ i ].semaphore;
    }

    VkPresentInfoKHR present_info = {};

    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext = nullptr;
    present_info.waitSemaphoreCount = desc.wait_semaphore_count;
    present_info.pWaitSemaphores = wait_semaphores;
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &desc.swapchain->swapchain;
    present_info.pImageIndices = &desc.image_index;
    present_info.pResults = nullptr;

    vkQueuePresentKHR(queue.queue, &present_info);
}

Semaphore create_semaphore(const Device& device)
{
    Semaphore semaphore{};

    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;

    VK_ASSERT(vkCreateSemaphore(
        device.logical_device, &semaphore_create_info, device.vulkan_allocator, &semaphore.semaphore));

    return semaphore;
}

void destroy_semaphore(const Device& device, Semaphore& semaphore)
{
    FT_ASSERT(semaphore.semaphore);
    vkDestroySemaphore(device.logical_device, semaphore.semaphore, device.vulkan_allocator);
}

Fence create_fence(const Device& device)
{
    Fence fence{};

    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_ASSERT(vkCreateFence(device.logical_device, &fence_create_info, device.vulkan_allocator, &fence.fence));

    return fence;
}

void destroy_fence(const Device& device, Fence& fence)
{
    FT_ASSERT(fence.fence);
    vkDestroyFence(device.logical_device, fence.fence, device.vulkan_allocator);
}

void wait_for_fences(const Device& device, u32 count, Fence* fences)
{
    VkFence* vk_fences = ( VkFence* ) alloca(count * sizeof(VkFence));
    for (u32 i = 0; i < count; ++i)
    {
        vk_fences[ i ] = fences[ i ].fence;
    }

    vkWaitForFences(device.logical_device, count, vk_fences, true, std::numeric_limits<u64>::max());
}

void reset_fences(const Device& device, u32 count, Fence* fences)
{
    VkFence* vk_fences = ( VkFence* ) alloca(count * sizeof(VkFence));
    for (u32 i = 0; i < count; ++i)
    {
        vk_fences[ i ] = fences[ i ].fence;
    }

    vkResetFences(device.logical_device, count, vk_fences);
}

Swapchain create_swapchain(const Renderer& renderer, const Device& device, const SwapchainDesc& desc)
{
    Swapchain swapchain{};

    SDL_Vulkan_CreateSurface(( SDL_Window* ) get_app_window()->handle, device.instance, &swapchain.surface);

    VkBool32 support_surface = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        device.physical_device, desc.queue->family_index, swapchain.surface, &support_surface);

    FT_ASSERT(support_surface);

    // find best present mode
    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device.physical_device, swapchain.surface, &present_mode_count, nullptr);
    VkPresentModeKHR* present_modes = ( VkPresentModeKHR* ) alloca(present_mode_count * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device.physical_device, swapchain.surface, &present_mode_count, present_modes);

    swapchain.present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

    for (u32 i = 0; i < present_mode_count; ++i)
    {
        if (present_modes[ i ] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchain.present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    // determine present image count
    VkSurfaceCapabilitiesKHR surface_capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physical_device, swapchain.surface, &surface_capabilities);

    // determine swapchain size
    swapchain.width =
        std::clamp(desc.width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
    swapchain.height =
        std::clamp(desc.height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);

    swapchain.image_count =
        std::clamp(desc.image_count, surface_capabilities.minImageCount, surface_capabilities.maxImageCount);

    /// find best surface format
    u32 surface_format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device.physical_device, swapchain.surface, &surface_format_count, nullptr);
    VkSurfaceFormatKHR* surface_formats =
        ( VkSurfaceFormatKHR* ) alloca(surface_format_count * sizeof(VkSurfaceFormatKHR));

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device.physical_device, swapchain.surface, &surface_format_count, surface_formats);

    VkSurfaceFormatKHR surface_format = surface_formats[ 0 ];
    for (u32 i = 0; i < surface_format_count; ++i)
    {
        if (surface_formats[ i ].format == VK_FORMAT_R8G8B8A8_UNORM ||
            surface_formats[ i ].format == VK_FORMAT_B8G8R8A8_UNORM)
            surface_format = surface_formats[ i ];
    }

    swapchain.format = from_vk_format(surface_format.format);

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
    swapchain_create_info.surface = swapchain.surface;
    swapchain_create_info.minImageCount = swapchain.image_count;
    swapchain_create_info.imageFormat = surface_format.format;
    swapchain_create_info.imageColorSpace = surface_format.colorSpace;
    swapchain_create_info.imageExtent.width = swapchain.width;
    swapchain_create_info.imageExtent.height = swapchain.height;
    swapchain_create_info.imageArrayLayers = 1;
    swapchain_create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchain_create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 1;
    swapchain_create_info.pQueueFamilyIndices = &desc.queue->family_index;
    swapchain_create_info.preTransform = pre_transform;
    // TODO: choose composite alpha according to caps
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode = swapchain.present_mode;
    swapchain_create_info.clipped = true;
    swapchain_create_info.oldSwapchain = nullptr;

    VK_ASSERT(vkCreateSwapchainKHR(
        device.logical_device, &swapchain_create_info, device.vulkan_allocator, &swapchain.swapchain));

    vkGetSwapchainImagesKHR(device.logical_device, swapchain.swapchain, &swapchain.image_count, nullptr);
    VkImage* swapchain_images = ( VkImage* ) alloca(swapchain.image_count * sizeof(VkImage));
    vkGetSwapchainImagesKHR(device.logical_device, swapchain.swapchain, &swapchain.image_count, swapchain_images);

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

    swapchain.images = new Image[ swapchain.image_count ];
    for (u32 i = 0; i < swapchain.image_count; ++i)
    {
        image_view_create_info.image = swapchain_images[ i ];

        swapchain.images[ i ] = {};
        swapchain.images[ i ].image = swapchain_images[ i ];
        swapchain.images[ i ].width = swapchain.width;
        swapchain.images[ i ].height = swapchain.height;
        swapchain.images[ i ].format = swapchain.format;
        swapchain.images[ i ].sample_count = SampleCount::e1;
        swapchain.images[ i ].mip_level_count = 1;
        swapchain.images[ i ].layer_count = 1;

        VK_ASSERT(vkCreateImageView(
            device.logical_device, &image_view_create_info, device.vulkan_allocator,
            &swapchain.images[ i ].image_view));
    }

    // create default render passes
    RenderPassDesc render_pass_desc{};
    render_pass_desc.color_attachment_count = 1;
    render_pass_desc.color_attachment_load_ops[ 0 ] = AttachmentLoadOp::eClear;
    render_pass_desc.color_image_states[ 0 ] = ResourceState::eColorAttachment;
    render_pass_desc.depth_stencil = nullptr;
    render_pass_desc.width = swapchain.width;
    render_pass_desc.height = swapchain.height;

    swapchain.render_passes = new RenderPass[ swapchain.image_count ];

    for (u32 i = 0; i < swapchain.image_count; ++i)
    {
        render_pass_desc.color_attachments[ 0 ] = &swapchain.images[ i ];
        swapchain.render_passes[ i ] = create_render_pass(device, render_pass_desc);
    }

    return swapchain;
}

void destroy_swapchain(const Device& device, Swapchain& swapchain)
{
    FT_ASSERT(swapchain.render_passes);
    for (u32 i = 0; i < swapchain.image_count; ++i)
    {
        destroy_render_pass(device, swapchain.render_passes[ i ]);
    }
    delete[] swapchain.render_passes;

    FT_ASSERT(swapchain.image_count);
    for (u32 i = 0; i < swapchain.image_count; ++i)
    {
        FT_ASSERT(swapchain.images[ i ].image_view);
        vkDestroyImageView(device.logical_device, swapchain.images[ i ].image_view, device.vulkan_allocator);
    }

    delete[] swapchain.images;
    FT_ASSERT(swapchain.swapchain);
    vkDestroySwapchainKHR(device.logical_device, swapchain.swapchain, device.vulkan_allocator);
    FT_ASSERT(swapchain.surface);
    vkDestroySurfaceKHR(device.instance, swapchain.surface, device.vulkan_allocator);
}

const RenderPass* get_swapchain_render_pass(const Swapchain& swapchain, u32 image_index)
{
    FT_ASSERT(image_index < swapchain.image_count);
    return &swapchain.render_passes[ image_index ];
}

CommandPool create_command_pool(const Device& device, const CommandPoolDesc& desc)
{
    CommandPool command_pool{};

    command_pool.queue = desc.queue;

    VkCommandPoolCreateInfo command_pool_create_info{};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext = nullptr;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = desc.queue->family_index;

    VK_ASSERT(vkCreateCommandPool(
        device.logical_device, &command_pool_create_info, device.vulkan_allocator, &command_pool.command_pool));

    return command_pool;
}

void destroy_command_pool(const Device& device, CommandPool& command_pool)
{
    FT_ASSERT(command_pool.command_pool);
    vkDestroyCommandPool(device.logical_device, command_pool.command_pool, device.vulkan_allocator);
}

void allocate_command_buffers(
    const Device& device, const CommandPool& command_pool, u32 count, CommandBuffer* command_buffers)
{
    FT_ASSERT(command_buffers);

    VkCommandBuffer* buffers = ( VkCommandBuffer* ) alloca(count * sizeof(VkCommandBuffer));

    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext = nullptr;
    command_buffer_allocate_info.commandPool = command_pool.command_pool;
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = count;

    VK_ASSERT(vkAllocateCommandBuffers(device.logical_device, &command_buffer_allocate_info, buffers));

    for (u32 i = 0; i < count; ++i)
    {
        command_buffers[ i ].command_buffer = buffers[ i ];
        command_buffers[ i ].queue = command_pool.queue;
    }
}

void free_command_buffers(
    const Device& device, const CommandPool& command_pool, u32 count, CommandBuffer* command_buffers)
{
    FT_ASSERT(command_buffers);

    VkCommandBuffer* buffers = ( VkCommandBuffer* ) alloca(count * sizeof(VkCommandBuffer));
    for (u32 i = 0; i < count; ++i)
    {
        buffers[ i ] = command_buffers[ i ].command_buffer;
    }

    vkFreeCommandBuffers(device.logical_device, command_pool.command_pool, count, buffers);
}

void begin_command_buffer(const CommandBuffer& cmd)
{
    VkCommandBufferBeginInfo command_buffer_begin_info{};
    command_buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext = nullptr;
    command_buffer_begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;

    VK_ASSERT(vkBeginCommandBuffer(cmd.command_buffer, &command_buffer_begin_info));
}

void end_command_buffer(const CommandBuffer& cmd)
{
    VK_ASSERT(vkEndCommandBuffer(cmd.command_buffer));
}

void acquire_next_image(
    const Device& device, const Swapchain& swapchain, const Semaphore& semaphore, const Fence& fence, u32& image_index)
{
    VkResult result = vkAcquireNextImageKHR(
        device.logical_device, swapchain.swapchain, std::numeric_limits<u64>::max(), semaphore.semaphore, fence.fence,
        &image_index);
}

RenderPass create_render_pass(const Device& device, const RenderPassDesc& desc)
{
    RenderPass render_pass{};
    render_pass.color_attachment_count = desc.color_attachment_count;
    render_pass.width = desc.width;
    render_pass.height = desc.height;

    u32 attachments_count = desc.color_attachment_count;

    VkAttachmentDescription attachment_descriptions[ MAX_ATTACHMENTS_COUNT + 1 ];
    VkAttachmentReference color_attachment_references[ MAX_ATTACHMENTS_COUNT ];
    VkAttachmentReference depth_attachment_reference{};

    for (u32 i = 0; i < desc.color_attachment_count; ++i)
    {
        attachment_descriptions[ i ].flags = 0;
        attachment_descriptions[ i ].format = to_vk_format(desc.color_attachments[ i ]->format);
        attachment_descriptions[ i ].samples = to_vk_sample_count(desc.color_attachments[ i ]->sample_count);
        attachment_descriptions[ i ].loadOp = to_vk_load_op(desc.color_attachment_load_ops[ i ]);
        attachment_descriptions[ i ].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_descriptions[ i ].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[ i ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[ i ].initialLayout = determine_image_layout(desc.color_image_states[ i ]);
        attachment_descriptions[ i ].finalLayout = determine_image_layout(desc.color_image_states[ i ]);

        color_attachment_references[ i ].attachment = i;
        color_attachment_references[ i ].layout = determine_image_layout(desc.color_image_states[ i ]);
    }

    if (desc.depth_stencil)
    {
        render_pass.has_depth_stencil = true;

        u32 i = attachments_count;
        attachment_descriptions[ i ].flags = 0;
        attachment_descriptions[ i ].format = to_vk_format(desc.depth_stencil->format);
        attachment_descriptions[ i ].samples = to_vk_sample_count(desc.depth_stencil->sample_count);
        attachment_descriptions[ i ].loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[ i ].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[ i ].stencilLoadOp = to_vk_load_op(desc.depth_stencil_load_op);
        attachment_descriptions[ i ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_descriptions[ i ].initialLayout = determine_image_layout(desc.depth_stencil_state);
        attachment_descriptions[ i ].finalLayout = determine_image_layout(desc.depth_stencil_state);

        depth_attachment_reference.attachment = i;
        depth_attachment_reference.layout = attachment_descriptions[ i ].finalLayout;

        attachments_count++;
    }

    // TODO: subpass setup from user code
    VkSubpassDescription subpass_description{};
    subpass_description.flags = 0;
    subpass_description.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount = 0;
    subpass_description.pInputAttachments = nullptr;
    subpass_description.colorAttachmentCount = desc.color_attachment_count;
    subpass_description.pColorAttachments = attachments_count ? color_attachment_references : nullptr;
    subpass_description.pResolveAttachments = nullptr;
    subpass_description.pDepthStencilAttachment = desc.depth_stencil ? &depth_attachment_reference : nullptr;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments = nullptr;

    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext = nullptr;
    render_pass_create_info.flags = 0;
    render_pass_create_info.attachmentCount = attachments_count;
    render_pass_create_info.pAttachments = attachments_count ? attachment_descriptions : nullptr;
    render_pass_create_info.subpassCount = 1;
    render_pass_create_info.pSubpasses = &subpass_description;
    render_pass_create_info.dependencyCount = 0;
    render_pass_create_info.pDependencies = nullptr;

    VK_ASSERT(vkCreateRenderPass(
        device.logical_device, &render_pass_create_info, device.vulkan_allocator, &render_pass.render_pass));

    VkImageView image_views[ MAX_ATTACHMENTS_COUNT + 1 ];
    for (u32 i = 0; i < desc.color_attachment_count; ++i)
    {
        image_views[ i ] = desc.color_attachments[ i ]->image_view;
    }

    if (desc.depth_stencil)
    {
        image_views[ desc.color_attachment_count ] = desc.depth_stencil->image_view;
    }

    if (desc.width > 0 && desc.height > 0)
    {
        VkFramebufferCreateInfo framebuffer_create_info{};
        framebuffer_create_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext = nullptr;
        framebuffer_create_info.flags = 0;
        framebuffer_create_info.renderPass = render_pass.render_pass;
        framebuffer_create_info.attachmentCount = attachments_count;
        framebuffer_create_info.pAttachments = image_views;
        framebuffer_create_info.width = desc.width;
        framebuffer_create_info.height = desc.height;
        framebuffer_create_info.layers = 1;

        VK_ASSERT(vkCreateFramebuffer(
            device.logical_device, &framebuffer_create_info, device.vulkan_allocator, &render_pass.framebuffer));
    }

    return render_pass;
}

void destroy_render_pass(const Device& device, RenderPass& render_pass)
{
    FT_ASSERT(render_pass.render_pass);
    if (render_pass.framebuffer)
    {
        vkDestroyFramebuffer(device.logical_device, render_pass.framebuffer, device.vulkan_allocator);
    }
    vkDestroyRenderPass(device.logical_device, render_pass.render_pass, device.vulkan_allocator);
}

Shader create_shader(const Device& device, const ShaderDesc& desc)
{
    Shader shader{};
    shader.stage = desc.stage;

    auto byte_code = read_file_binary(get_app_shaders_directory() + desc.filename);

    VkShaderModuleCreateInfo shader_create_info{};
    shader_create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.pNext = nullptr;
    shader_create_info.flags = 0;
    shader_create_info.codeSize = byte_code.size() * sizeof(uint32_t);
    shader_create_info.pCode = byte_code.data();

    VK_ASSERT(
        vkCreateShaderModule(device.logical_device, &shader_create_info, device.vulkan_allocator, &shader.shader));

    shader.reflect_data = reflect(byte_code.size() * sizeof(u32), byte_code.data());

    return shader;
}

void destroy_shader(const Device& device, Shader& shader)
{
    FT_ASSERT(shader.shader);
    vkDestroyShaderModule(device.logical_device, shader.shader, device.vulkan_allocator);
}

DescriptorSetLayout create_descriptor_set_layout(const Device& device, const DescriptorSetLayoutDesc& desc)
{
    FT_ASSERT(desc.shader_count);

    DescriptorSetLayout descriptor_set_layout{};
    descriptor_set_layout.shader_count = desc.shader_count;
    descriptor_set_layout.shaders = desc.shaders;

    // count bindings in all shaders
    u32 binding_count = 0;
    for (u32 i = 0; i < descriptor_set_layout.shader_count; ++i)
    {
        binding_count += descriptor_set_layout.shaders[ i ].reflect_data.binding_count;
    }

    if (binding_count > 0)
    {
        // collect all bindings
        VkDescriptorSetLayoutBinding* bindings = new VkDescriptorSetLayoutBinding[ binding_count ];
        u32 binding_index = 0;
        for (u32 i = 0; i < descriptor_set_layout.shader_count; ++i)
        {
            for (u32 j = 0; j < descriptor_set_layout.shaders[ i ].reflect_data.binding_count; ++j)
            {
                auto& reflected_binding = descriptor_set_layout.shaders[ i ].reflect_data.bindings[ j ];
                bindings[ binding_index ].binding = reflected_binding.binding;
                bindings[ binding_index ].descriptorCount = reflected_binding.descriptor_count;
                bindings[ binding_index ].descriptorType = to_vk_descriptor_type(reflected_binding.descriptor_type);
                bindings[ binding_index ].pImmutableSamplers = nullptr; // ??? TODO
                bindings[ binding_index ].stageFlags = to_vk_shader_stage(descriptor_set_layout.shaders[ i ].stage);
                binding_index++;
            }
        }

        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
        descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_create_info.pNext = nullptr;
        descriptor_set_layout_create_info.flags = 0;
        descriptor_set_layout_create_info.bindingCount = binding_count;
        descriptor_set_layout_create_info.pBindings = bindings;

        VK_ASSERT(vkCreateDescriptorSetLayout(
            device.logical_device, &descriptor_set_layout_create_info, device.vulkan_allocator,
            &descriptor_set_layout.descriptor_set_layout));

        delete[] bindings;
    }

    return descriptor_set_layout;
}

void destroy_descriptor_set_layout(const Device& device, DescriptorSetLayout& layout)
{
    FT_ASSERT(layout.shaders);
    if (layout.descriptor_set_layout)
    {
        vkDestroyDescriptorSetLayout(device.logical_device, layout.descriptor_set_layout, device.vulkan_allocator);
    }
}

Pipeline create_compute_pipeline(const Device& device, const PipelineDesc& desc)
{
    FT_ASSERT(desc.descriptor_set_layout);

    Pipeline pipeline{};
    pipeline.type = PipelineType::eCompute;

    VkPipelineShaderStageCreateInfo shader_stage_create_info{};
    shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_info.pNext = nullptr;
    shader_stage_create_info.flags = 0;
    shader_stage_create_info.stage = to_vk_shader_stage(desc.descriptor_set_layout->shaders[ 0 ].stage);
    shader_stage_create_info.module = desc.descriptor_set_layout->shaders[ 0 ].shader;
    shader_stage_create_info.pName = "main";
    shader_stage_create_info.pSpecializationInfo = nullptr;

    VkPushConstantRange push_constant_range{};
    push_constant_range.size = MAX_PUSH_CONSTANT_RANGE;
    push_constant_range.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &desc.descriptor_set_layout->descriptor_set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;

    VK_ASSERT(vkCreatePipelineLayout(
        device.logical_device, &pipeline_layout_create_info, nullptr, &pipeline.pipeline_layout));

    VkComputePipelineCreateInfo compute_pipeline_create_info{};
    compute_pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_create_info.stage = shader_stage_create_info;
    compute_pipeline_create_info.layout = pipeline.pipeline_layout;

    VK_ASSERT(vkCreateComputePipelines(
        device.logical_device, {}, 1, &compute_pipeline_create_info, device.vulkan_allocator, &pipeline.pipeline));

    return pipeline;
}

Pipeline create_graphics_pipeline(const Device& device, const PipelineDesc& desc)
{
    FT_ASSERT(desc.descriptor_set_layout);
    FT_ASSERT(desc.render_pass);

    Pipeline pipeline{};
    pipeline.type = PipelineType::eGraphics;

    u32 shader_stage_count = desc.descriptor_set_layout->shader_count;
    VkPipelineShaderStageCreateInfo shader_stage_create_infos[ MAX_STAGE_COUNT ];
    for (u32 i = 0; i < shader_stage_count; ++i)
    {
        shader_stage_create_infos[ i ].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_create_infos[ i ].pNext = nullptr;
        shader_stage_create_infos[ i ].flags = 0;
        shader_stage_create_infos[ i ].stage = to_vk_shader_stage(desc.descriptor_set_layout->shaders[ i ].stage);
        shader_stage_create_infos[ i ].module = desc.descriptor_set_layout->shaders[ i ].shader;
        shader_stage_create_infos[ i ].pName = "main";
        shader_stage_create_infos[ i ].pSpecializationInfo = nullptr;
    }

    VkVertexInputBindingDescription binding_descriptions[ MAX_VERTEX_BINGINGS_COUNT ];
    for (u32 i = 0; i < desc.binding_desc_count; ++i)
    {
        binding_descriptions[ i ].binding = desc.binding_descs[ i ].binding;
        binding_descriptions[ i ].stride = desc.binding_descs[ i ].stride;
        binding_descriptions[ i ].inputRate = to_vk_vertex_input_rate(desc.binding_descs[ i ].input_rate);
    }

    VkVertexInputAttributeDescription attribute_descriptions[ MAX_VERTEX_ATTRIBUTE_COUNT ];
    for (u32 i = 0; i < desc.attribute_desc_count; ++i)
    {
        attribute_descriptions[ i ].location = desc.attribute_descs[ i ].location;
        attribute_descriptions[ i ].binding = desc.attribute_descs[ i ].binding;
        attribute_descriptions[ i ].format = to_vk_format(desc.attribute_descs[ i ].format);
        attribute_descriptions[ i ].offset = desc.attribute_descs[ i ].offset;
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount = desc.binding_desc_count;
    vertex_input_state_create_info.pVertexBindingDescriptions = binding_descriptions;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = desc.attribute_desc_count;
    vertex_input_state_create_info.pVertexAttributeDescriptions = attribute_descriptions;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
    input_assembly_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = false;

    // Dynamic states
    VkViewport viewport{};
    VkRect2D scissor{};

    VkPipelineViewportStateCreateInfo viewport_state_create_info{};
    viewport_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports = &viewport;
    viewport_state_create_info.scissorCount = 1;
    viewport_state_create_info.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
    rasterization_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.polygonMode = VK_POLYGON_MODE_FILL;
    rasterization_state_create_info.cullMode = to_vk_cull_mode(desc.rasterizer_desc.cull_mode);
    rasterization_state_create_info.frontFace = to_vk_front_face(desc.rasterizer_desc.front_face);
    rasterization_state_create_info.lineWidth = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
    multisample_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.minSampleShading = 1.0f;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state{};
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{};
    color_blend_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable = false;
    color_blend_state_create_info.logicOp = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments = &color_blend_attachment_state;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{};
    depth_stencil_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable = desc.depth_state_desc.depth_test ? VK_TRUE : VK_FALSE;
    depth_stencil_state_create_info.depthWriteEnable = desc.depth_state_desc.depth_write ? VK_TRUE : VK_FALSE;
    depth_stencil_state_create_info.depthCompareOp =
        desc.depth_state_desc.depth_test ? to_vk_compare_op(desc.depth_state_desc.compare_op) : VK_COMPARE_OP_ALWAYS;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.minDepthBounds = 0.0f; // Optional
    depth_stencil_state_create_info.maxDepthBounds = 1.0f; // Optional
    depth_stencil_state_create_info.stencilTestEnable = VK_FALSE;

    const uint32_t dynamic_state_count = 2;
    VkDynamicState dynamic_states[ dynamic_state_count ] = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT };

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
    dynamic_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = dynamic_state_count;
    dynamic_state_create_info.pDynamicStates = dynamic_states;

    // TODO: push constant range
    VkPushConstantRange push_constant_range{};
    push_constant_range.size = MAX_PUSH_CONSTANT_RANGE;
    push_constant_range.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    u32 set_layout_count = desc.descriptor_set_layout->descriptor_set_layout ? 1 : 0;
    VkDescriptorSetLayout* set_layout = set_layout_count ? &desc.descriptor_set_layout->descriptor_set_layout : nullptr;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = set_layout_count;
    pipeline_layout_create_info.pSetLayouts = set_layout;
    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges = &push_constant_range;

    VK_ASSERT(vkCreatePipelineLayout(
        device.logical_device, &pipeline_layout_create_info, device.vulkan_allocator, &pipeline.pipeline_layout));

    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount = shader_stage_count;
    pipeline_create_info.pStages = shader_stage_create_infos;
    pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    pipeline_create_info.pViewportState = &viewport_state_create_info;
    pipeline_create_info.pMultisampleState = &multisample_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    pipeline_create_info.pColorBlendState = &color_blend_state_create_info;
    pipeline_create_info.pDepthStencilState = &depth_stencil_state_create_info;
    pipeline_create_info.pDynamicState = &dynamic_state_create_info;
    pipeline_create_info.layout = pipeline.pipeline_layout;
    pipeline_create_info.renderPass = desc.render_pass->render_pass;

    VK_ASSERT(vkCreateGraphicsPipelines(
        device.logical_device, VK_NULL_HANDLE, 1, &pipeline_create_info, device.vulkan_allocator, &pipeline.pipeline));

    return pipeline;
}

void destroy_pipeline(const Device& device, Pipeline& pipeline)
{
    FT_ASSERT(pipeline.pipeline_layout);
    FT_ASSERT(pipeline.pipeline);
    vkDestroyPipelineLayout(device.logical_device, pipeline.pipeline_layout, device.vulkan_allocator);
    vkDestroyPipeline(device.logical_device, pipeline.pipeline, device.vulkan_allocator);
}

void cmd_begin_render_pass(const CommandBuffer& cmd, const RenderPassBeginDesc& desc)
{
    FT_ASSERT(desc.render_pass);

    static VkClearValue clear_values[ MAX_ATTACHMENTS_COUNT + 1 ];
    const RenderPass* render_pass = desc.render_pass;
    u32 clear_value_count = render_pass->color_attachment_count;

    for (u32 i = 0; i < render_pass->color_attachment_count; ++i)
    {
        clear_values[ i ] = VkClearValue{};
        clear_values[ i ].color.float32[ 0 ] = desc.clear_values[ i ].color[ 0 ];
        clear_values[ i ].color.float32[ 1 ] = desc.clear_values[ i ].color[ 1 ];
        clear_values[ i ].color.float32[ 2 ] = desc.clear_values[ i ].color[ 2 ];
        clear_values[ i ].color.float32[ 3 ] = desc.clear_values[ i ].color[ 3 ];
    }

    if (render_pass->has_depth_stencil)
    {
        clear_value_count++;
        u32 idx = render_pass->color_attachment_count;
        clear_values[ idx ] = VkClearValue{};
        clear_values[ idx ].depthStencil.depth = desc.clear_values[ idx ].depth;
        clear_values[ idx ].depthStencil.stencil = desc.clear_values[ idx ].stencil;
    }

    VkRenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext = nullptr;
    render_pass_begin_info.renderPass = render_pass->render_pass;
    render_pass_begin_info.framebuffer = render_pass->framebuffer;
    render_pass_begin_info.renderArea.extent.width = render_pass->width;
    render_pass_begin_info.renderArea.extent.height = render_pass->height;
    render_pass_begin_info.renderArea.offset.x = 0;
    render_pass_begin_info.renderArea.offset.y = 0;
    render_pass_begin_info.clearValueCount = clear_value_count;
    render_pass_begin_info.pClearValues = clear_values;

    vkCmdBeginRenderPass(cmd.command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void cmd_end_render_pass(const CommandBuffer& cmd)
{
    vkCmdEndRenderPass(cmd.command_buffer);
}

void cmd_barrier(
    const CommandBuffer& cmd, u32 buffer_barriers_count, const BufferBarrier* buffer_barriers, u32 image_barriers_count,
    const ImageBarrier* image_barriers)
{
    VkBufferMemoryBarrier* buffer_memory_barriers =
        buffer_barriers_count ? ( VkBufferMemoryBarrier* ) alloca(buffer_barriers_count * sizeof(VkBufferMemoryBarrier))
                              : nullptr;
    VkImageMemoryBarrier* image_memory_barriers =
        image_barriers_count ? ( VkImageMemoryBarrier* ) alloca(image_barriers_count * sizeof(VkImageMemoryBarrier))
                             : nullptr;

    // TODO: Queues
    VkAccessFlags src_access = VkAccessFlags(0);
    VkAccessFlags dst_access = VkAccessFlags(0);

    for (u32 i = 0; i < buffer_barriers_count; ++i)
    {
        FT_ASSERT(buffer_barriers[ i ].buffer);
        FT_ASSERT(buffer_barriers[ i ].src_queue);
        FT_ASSERT(buffer_barriers[ i ].dst_queue);

        VkAccessFlags src_access_mask = determine_access_flags(buffer_barriers[ i ].old_state);
        VkAccessFlags dst_access_mask = determine_access_flags(buffer_barriers[ i ].new_state);

        VkBufferMemoryBarrier buffer_memory_barrier{};
        buffer_memory_barrier.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        buffer_memory_barrier.pNext = nullptr;
        buffer_memory_barrier.srcAccessMask = src_access_mask;
        buffer_memory_barrier.dstAccessMask = dst_access_mask;
        buffer_memory_barrier.srcQueueFamilyIndex = buffer_barriers[ i ].src_queue->family_index;
        buffer_memory_barrier.dstQueueFamilyIndex = buffer_barriers[ i ].dst_queue->family_index;
        buffer_memory_barrier.buffer = buffer_barriers[ i ].buffer->buffer;
        buffer_memory_barrier.offset = buffer_barriers[ i ].offset;
        buffer_memory_barrier.size = buffer_barriers[ i ].size;

        src_access |= src_access_mask;
        dst_access |= dst_access_mask;
    }

    for (u32 i = 0; i < image_barriers_count; ++i)
    {
        FT_ASSERT(image_barriers[ i ].image);
        FT_ASSERT(image_barriers[ i ].src_queue);
        FT_ASSERT(image_barriers[ i ].dst_queue);

        VkAccessFlags src_access_mask = determine_access_flags(image_barriers[ i ].old_state);
        VkAccessFlags dst_access_mask = determine_access_flags(image_barriers[ i ].new_state);

        image_memory_barriers[ i ].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barriers[ i ].pNext = nullptr;
        image_memory_barriers[ i ].srcAccessMask = src_access_mask;
        image_memory_barriers[ i ].dstAccessMask = dst_access_mask;
        image_memory_barriers[ i ].oldLayout = determine_image_layout(image_barriers[ i ].old_state);
        image_memory_barriers[ i ].newLayout = determine_image_layout(image_barriers[ i ].new_state);
        image_memory_barriers[ i ].srcQueueFamilyIndex = image_barriers[ i ].src_queue->family_index;
        image_memory_barriers[ i ].dstQueueFamilyIndex = image_barriers[ i ].dst_queue->family_index;
        image_memory_barriers[ i ].image = image_barriers[ i ].image->image;
        image_memory_barriers[ i ].subresourceRange = get_image_subresource_range(*image_barriers[ i ].image);

        src_access |= src_access_mask;
        dst_access |= dst_access_mask;
    }

    VkPipelineStageFlags src_stage = determine_pipeline_stage_flags(src_access, cmd.queue->type);
    VkPipelineStageFlags dst_stage = determine_pipeline_stage_flags(dst_access, cmd.queue->type);

    vkCmdPipelineBarrier(
        cmd.command_buffer, src_stage, dst_stage, 0, 0, nullptr, buffer_barriers_count, buffer_memory_barriers,
        image_barriers_count, image_memory_barriers);
};

void cmd_set_scissor(const CommandBuffer& cmd, i32 x, i32 y, u32 width, u32 height)
{
    VkRect2D scissor{};
    scissor.offset.x = x;
    scissor.offset.y = y;
    scissor.extent.width = width;
    scissor.extent.height = height;
    vkCmdSetScissor(cmd.command_buffer, 0, 1, &scissor);
}

void cmd_set_viewport(const CommandBuffer& cmd, f32 x, f32 y, f32 width, f32 height, f32 min_depth, f32 max_depth)
{
    VkViewport viewport{};
    viewport.x = x;
    viewport.y = y + height;
    viewport.width = width;
    viewport.height = -height;
    viewport.minDepth = min_depth;
    viewport.maxDepth = max_depth;

    vkCmdSetViewport(cmd.command_buffer, 0, 1, &viewport);
}

void cmd_bind_pipeline(const CommandBuffer& cmd, const Pipeline& pipeline)
{
    vkCmdBindPipeline(cmd.command_buffer, to_vk_pipeline_bind_point(pipeline.type), pipeline.pipeline);
}

void cmd_draw(
    const CommandBuffer& cmd, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex,
    uint32_t first_instance)
{
    vkCmdDraw(cmd.command_buffer, vertex_count, instance_count, first_vertex, first_instance);
}

void cmd_bind_vertex_buffer(const CommandBuffer& cmd, const Buffer& buffer)
{
    static constexpr VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd.command_buffer, 0, 1, &buffer.buffer, &offset);
}

void cmd_copy_buffer(const CommandBuffer& cmd, const Buffer& src, u32 src_offset, Buffer& dst, u32 dst_offset, u32 size)
{
    VkBufferCopy buffer_copy{};
    buffer_copy.srcOffset = src_offset;
    buffer_copy.dstOffset = dst_offset;
    buffer_copy.size = size;

    vkCmdCopyBuffer(cmd.command_buffer, src.buffer, dst.buffer, 1, &buffer_copy);
}

void cmd_copy_buffer_to_image(const CommandBuffer& cmd, const Buffer& src, u32 src_offset, Image& dst)
{
    auto dst_layers = get_image_subresource_layers(dst);

    VkBufferImageCopy buffer_to_image_copy_info{};
    buffer_to_image_copy_info.bufferOffset = src_offset;
    buffer_to_image_copy_info.bufferImageHeight = 0;
    buffer_to_image_copy_info.bufferRowLength = 0;
    buffer_to_image_copy_info.imageSubresource = dst_layers;
    buffer_to_image_copy_info.imageOffset = VkOffset3D{ 0, 0, 0 };
    buffer_to_image_copy_info.imageExtent = VkExtent3D{ dst.width, dst.height, 1 };

    vkCmdCopyBufferToImage(
        cmd.command_buffer, src.buffer, dst.image, determine_image_layout(ResourceState::eTransferDst), 1,
        &buffer_to_image_copy_info);
}

void cmd_dispatch(const CommandBuffer& cmd, u32 group_count_x, u32 group_count_y, u32 group_count_z)
{
    vkCmdDispatch(cmd.command_buffer, group_count_x, group_count_y, group_count_z);
}

void cmd_push_constants(const CommandBuffer& cmd, const Pipeline& pipeline, u32 offset, u32 size, const void* data)
{
    vkCmdPushConstants(
        cmd.command_buffer, pipeline.pipeline_layout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, offset, size, data);
}

void cmd_blit_image(
    const CommandBuffer& cmd, const Image& src, ResourceState src_state, Image& dst, ResourceState dst_state,
    Filter filter)
{
    auto src_range = get_image_subresource_range(src);
    auto dst_range = get_image_subresource_range(dst);

    u32 barrier_count = 0;
    u32 index = 0;

    ImageBarrier barriers[ 2 ] = {};
    // To transfer src
    barriers[ 0 ].src_queue = cmd.queue;
    barriers[ 0 ].dst_queue = cmd.queue;
    barriers[ 0 ].image = &src;
    barriers[ 0 ].old_state = src_state;
    barriers[ 0 ].new_state = ResourceState::eTransferSrc;
    // To transfer dst
    barriers[ 1 ].src_queue = cmd.queue;
    barriers[ 1 ].dst_queue = cmd.queue;
    barriers[ 1 ].image = &dst;
    barriers[ 1 ].old_state = dst_state;
    barriers[ 1 ].new_state = ResourceState::eTransferDst;

    if (src_state != ResourceState::eTransferSrc)
    {
        barrier_count++;
    }

    if (dst_state != ResourceState::eTransferDst)
    {
        if (barrier_count == 0)
        {
            index = 1;
        }

        barrier_count++;
    }

    if (barrier_count > 0)
    {
        cmd_barrier(cmd, 0, nullptr, barrier_count, &barriers[ index ]);
    }

    auto src_layers = get_image_subresource_layers(src);
    auto dst_layers = get_image_subresource_layers(dst);

    VkImageBlit image_blit_info{};
    image_blit_info.srcOffsets[ 0 ] = VkOffset3D{ 0, 0, 0 };
    image_blit_info.srcOffsets[ 1 ] = VkOffset3D{ ( i32 ) src.width, ( i32 ) src.height, 1 };
    image_blit_info.dstOffsets[ 0 ] = VkOffset3D{ 0, 0, 0 };
    image_blit_info.dstOffsets[ 1 ] = VkOffset3D{ ( i32 ) dst.width, ( i32 ) dst.height, 1 };
    image_blit_info.srcSubresource = src_layers;
    image_blit_info.dstSubresource = dst_layers;

    vkCmdBlitImage(
        cmd.command_buffer, src.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_blit_info, to_vk_filter(filter));
}

void immediate_submit(const Queue& queue, const CommandBuffer& cmd)
{
    QueueSubmitDesc submit_desc{};
    submit_desc.wait_semaphore_count = 0;
    submit_desc.wait_semaphores = nullptr;
    submit_desc.command_buffer_count = 1;
    submit_desc.command_buffers = &cmd;
    submit_desc.signal_semaphore_count = 0;
    submit_desc.signal_semaphores = 0;
    submit_desc.signal_fence = nullptr;

    queue_submit(queue, submit_desc);
    queue_wait_idle(queue);
}

Buffer create_buffer(const Device& device, const BufferDesc& desc)
{
    Buffer buffer{};
    buffer.size = desc.size;
    buffer.resource_state = desc.resource_state;
    buffer.descriptor_type = desc.descriptor_type;

    VmaAllocationCreateInfo allocation_create_info{};
    allocation_create_info.usage = determine_vma_memory_usage(desc.resource_state);

    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext = nullptr;
    buffer_create_info.flags = 0;
    buffer_create_info.size = desc.size;
    buffer_create_info.usage = determine_vk_buffer_usage(desc.resource_state, desc.descriptor_type);
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices = nullptr;

    VK_ASSERT(vmaCreateBuffer(
        device.memory_allocator, &buffer_create_info, &allocation_create_info, &buffer.buffer, &buffer.allocation,
        nullptr));

    if (desc.data)
    {
        BufferUpdateDesc buffer_update_desc{};
        buffer_update_desc.data = desc.data;
        buffer_update_desc.buffer = &buffer;
        buffer_update_desc.offset = 0;
        buffer_update_desc.size = desc.size;

        update_buffer(device, buffer_update_desc);
    }

    return buffer;
}

void destroy_buffer(const Device& device, Buffer& buffer)
{
    FT_ASSERT(buffer.allocation);
    FT_ASSERT(buffer.buffer);
    vmaDestroyBuffer(device.memory_allocator, buffer.buffer, buffer.allocation);
}

void update_buffer(const Device& device, BufferUpdateDesc& desc)
{
    FT_ASSERT(desc.buffer);
    FT_ASSERT(desc.offset + desc.size <= desc.buffer->size);

    Buffer* buffer = desc.buffer;

    if (buffer->resource_state & ResourceState::eTransferSrc)
    {
        bool was_mapped = buffer->mapped_memory;
        if (!was_mapped)
        {
            map_memory(device, *buffer);
        }

        std::memcpy(( u8* ) buffer->mapped_memory + desc.offset, desc.data, desc.size);

        if (!was_mapped)
        {
            unmap_memory(device, *buffer);
        }
    }
    else
    {
        write_to_staging_buffer(device, desc.size, desc.data);

        begin_command_buffer(device.upload_command_buffer);

        cmd_copy_buffer(
            device.upload_command_buffer, device.staging_buffer.buffer, device.staging_buffer.memory_used, *desc.buffer,
            desc.offset, desc.size);

        end_command_buffer(device.upload_command_buffer);

        immediate_submit(device.upload_queue, device.upload_command_buffer);
    }
}

void cmd_bind_descriptor_set(const CommandBuffer& cmd, const DescriptorSet& set, const Pipeline& pipeline)
{
    vkCmdBindDescriptorSets(
        cmd.command_buffer, to_vk_pipeline_bind_point(pipeline.type), pipeline.pipeline_layout, 0, 1,
        &set.descriptor_set, 0, nullptr);
}

Sampler create_sampler(const Device& device, const SamplerDesc& desc)
{
    Sampler sampler{};

    VkSamplerCreateInfo sampler_create_info{};
    sampler_create_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext = nullptr;
    sampler_create_info.flags = 0;
    sampler_create_info.magFilter = to_vk_filter(desc.mag_filter);
    sampler_create_info.minFilter = to_vk_filter(desc.min_filter);
    sampler_create_info.mipmapMode = to_vk_sampler_mipmap_mode(desc.mipmap_mode);
    sampler_create_info.addressModeU = to_vk_sampler_address_mode(desc.address_mode_u);
    sampler_create_info.addressModeV = to_vk_sampler_address_mode(desc.address_mode_v);
    sampler_create_info.addressModeW = to_vk_sampler_address_mode(desc.address_mode_w);
    sampler_create_info.mipLodBias = desc.mip_lod_bias;
    sampler_create_info.anisotropyEnable = desc.anisotropy_enable;
    sampler_create_info.maxAnisotropy = desc.max_anisotropy;
    sampler_create_info.compareEnable = desc.compare_enable;
    sampler_create_info.compareOp = to_vk_compare_op(desc.compare_op);
    sampler_create_info.minLod = desc.min_lod;
    sampler_create_info.maxLod = desc.max_lod;
    sampler_create_info.unnormalizedCoordinates = false;

    VK_ASSERT(vkCreateSampler(device.logical_device, &sampler_create_info, device.vulkan_allocator, &sampler.sampler));

    return sampler;
}

void destroy_sampler(const Device& device, Sampler& sampler)
{
    FT_ASSERT(sampler.sampler);
    vkDestroySampler(device.logical_device, sampler.sampler, device.vulkan_allocator);
}

Image create_image(const Device& device, const ImageDesc& desc)
{
    Image image{};

    VmaAllocationCreateInfo allocation_create_info{};
    allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = nullptr;
    image_create_info.flags = 0;
    // TODO: determine image type properly
    image_create_info.imageType = VK_IMAGE_TYPE_2D;
    image_create_info.format = to_vk_format(desc.format);
    image_create_info.extent.width = desc.width;
    image_create_info.extent.height = desc.height;
    image_create_info.extent.depth = desc.depth;
    image_create_info.mipLevels = desc.mip_levels;
    image_create_info.arrayLayers = desc.layer_count;
    image_create_info.samples = to_vk_sample_count(desc.sample_count);
    image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage = determine_vk_image_usage(desc.resource_state, desc.descriptor_type);
    image_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices = nullptr;
    image_create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

    VK_ASSERT(vmaCreateImage(
        device.memory_allocator, &image_create_info, &allocation_create_info, &image.image, &image.allocation,
        nullptr));

    image.width = desc.width;
    image.height = desc.height;
    image.format = desc.format;
    image.sample_count = desc.sample_count;
    image.mip_level_count = desc.mip_levels;
    image.layer_count = desc.layer_count;
    image.resource_state = desc.resource_state;
    image.descriptor_type = desc.descriptor_type;

    if (desc.data)
    {
        ImageUpdateDesc image_update_desc{};
        image_update_desc.image = &image;
        image_update_desc.size = desc.data_size;
        image_update_desc.data = desc.data;
        image_update_desc.resource_state = ResourceState::eUndefined;

        update_image(device, image_update_desc);
    }

    if (desc.resource_state != ResourceState::eUndefined)
    {
        ImageBarrier image_barrier{};
        image_barrier.image = &image;
        image_barrier.old_state = ResourceState::eUndefined;
        image_barrier.new_state = desc.resource_state;
        image_barrier.src_queue = &device.upload_queue;
        image_barrier.dst_queue = &device.upload_queue;

        begin_command_buffer(device.upload_command_buffer);
        cmd_barrier(device.upload_command_buffer, 0, nullptr, 1, &image_barrier);
        end_command_buffer(device.upload_command_buffer);
        immediate_submit(device.upload_queue, device.upload_command_buffer);
    }

    // TODO: fill properly
    VkImageViewCreateInfo image_view_create_info{};
    image_view_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext = nullptr;
    image_view_create_info.flags = 0;
    image_view_create_info.image = image.image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format = image_create_info.format;
    image_view_create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange = get_image_subresource_range(image);

    VK_ASSERT(
        vkCreateImageView(device.logical_device, &image_view_create_info, device.vulkan_allocator, &image.image_view));

    return image;
}

void destroy_image(const Device& device, Image& image)
{
    FT_ASSERT(image.image_view);
    FT_ASSERT(image.image);
    FT_ASSERT(image.allocation);
    vkDestroyImageView(device.logical_device, image.image_view, device.vulkan_allocator);
    vmaDestroyImage(device.memory_allocator, image.image, image.allocation);
}

Image load_image_from_dds_file(const Device& device, const char* filename, ResourceState resource_state)
{
    std::string filepath = std::string(get_app_textures_directory()) + std::string(filename);
    ImageDesc image_desc = read_image_desc_from_dds(filepath.c_str());
    image_desc.descriptor_type = DescriptorType::eSampledImage;
    image_desc.resource_state = ResourceState(ResourceState::eTransferDst | resource_state);
    Image image = create_image(device, image_desc);
    release_image_desc_dds(image_desc);

    return image;
}

Image load_image_from_file(const Device& device, const char* filename, ResourceState resource_state)
{
    std::string filepath = std::string(get_app_textures_directory()) + std::string(filename);
    ImageDesc image_desc = read_image_desc(filepath.c_str());
    image_desc.descriptor_type = DescriptorType::eSampledImage;
    image_desc.resource_state = ResourceState(ResourceState::eTransferDst | resource_state);
    Image image = create_image(device, image_desc);
    release_image_desc(image_desc);

    return image;
}

void update_image(const Device& device, const ImageUpdateDesc& desc)
{
    write_to_staging_buffer(device, desc.size, desc.data);

    begin_command_buffer(device.upload_command_buffer);

    ImageBarrier image_barrier{};
    image_barrier.image = desc.image;
    image_barrier.old_state = desc.resource_state;
    image_barrier.new_state = ResourceState::eTransferDst;
    image_barrier.src_queue = &device.upload_queue;
    image_barrier.dst_queue = &device.upload_queue;

    if (desc.resource_state != ResourceState::eTransferDst)
    {
        cmd_barrier(device.upload_command_buffer, 0, nullptr, 1, &image_barrier);
    }

    cmd_copy_buffer_to_image(
        device.upload_command_buffer, device.staging_buffer.buffer, device.staging_buffer.memory_used, *desc.image);

    image_barrier.old_state = ResourceState::eTransferDst;
    image_barrier.new_state = desc.resource_state;

    if (desc.resource_state != ResourceState::eTransferDst && desc.resource_state != ResourceState::eUndefined)
    {
        cmd_barrier(device.upload_command_buffer, 0, nullptr, 1, &image_barrier);
    }

    end_command_buffer(device.upload_command_buffer);

    immediate_submit(device.upload_queue, device.upload_command_buffer);
}

DescriptorSet create_descriptor_set(const Device& device, const DescriptorSetDesc& desc)
{
    FT_ASSERT(desc.descriptor_set_layout);
    FT_ASSERT(desc.descriptor_set_layout->descriptor_set_layout != VK_NULL_HANDLE);

    DescriptorSet descriptor_set{};

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext = nullptr;
    descriptor_set_allocate_info.descriptorPool = device.descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &desc.descriptor_set_layout->descriptor_set_layout;

    VK_ASSERT(
        vkAllocateDescriptorSets(device.logical_device, &descriptor_set_allocate_info, &descriptor_set.descriptor_set));

    return descriptor_set;
}

void destroy_descriptor_set(const Device& device, DescriptorSet& set)
{
    FT_ASSERT(set.descriptor_set);
    vkFreeDescriptorSets(device.logical_device, device.descriptor_pool, 1, &set.descriptor_set);
}

void update_descriptor_set(const Device& device, DescriptorSet& set, u32 count, const DescriptorSetUpdateDesc* descs)
{
    // TODO: rewrite
    std::vector<std::vector<VkDescriptorBufferInfo>> buffer_updates(count);
    std::vector<std::vector<VkDescriptorImageInfo>> image_updates(count);
    std::vector<VkWriteDescriptorSet> descriptor_writes(count);

    uint32_t buffer_update_idx = 0;
    uint32_t image_update_idx = 0;
    uint32_t write = 0;

    for (u32 i = 0; i < count; ++i)
    {
        auto& update = descs[ i ];

        auto& write_descriptor_set = descriptor_writes[ write++ ];
        write_descriptor_set.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.dstBinding = update.binding;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.dstSet = set.descriptor_set;
        write_descriptor_set.descriptorType = to_vk_descriptor_type(update.descriptor_type);

        if (update.buffer_set_write_count > 0)
        {
            auto& buffer_update_infos = buffer_updates[ buffer_update_idx++ ];
            buffer_update_infos.resize(update.buffer_set_write_count, {});

            for (u32 j = 0; j < buffer_update_infos.size(); ++j)
            {
                buffer_update_infos[ j ].buffer = update.buffer_set_writes[ j ].buffer->buffer;
                buffer_update_infos[ j ].offset = update.buffer_set_writes[ j ].offset;
                buffer_update_infos[ j ].range = update.buffer_set_writes[ j ].range;
            }

            write_descriptor_set.descriptorCount = buffer_update_infos.size();
            write_descriptor_set.pBufferInfo = buffer_update_infos.data();
        }

        if (update.image_set_write_count > 0)
        {
            auto& image_update_infos = image_updates[ image_update_idx++ ];
            image_update_infos.resize(update.image_set_write_count, {});

            for (u32 j = 0; j < image_update_infos.size(); ++j)
            {
                if (update.image_set_writes[ j ].sampler != nullptr)
                {
                    image_update_infos[ j ].sampler = update.image_set_writes[ j ].sampler->sampler;
                }

                if (update.image_set_writes[ j ].image != nullptr)
                {
                    image_update_infos[ j ].imageLayout =
                        determine_image_layout(update.image_set_writes[ j ].image->resource_state);
                    image_update_infos[ j ].imageView = update.image_set_writes[ j ].image->image_view;
                }
            }

            write_descriptor_set.descriptorCount = image_update_infos.size();
            write_descriptor_set.pImageInfo = image_update_infos.data();
        }
    }

    vkUpdateDescriptorSets(device.logical_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
}

} // namespace fluent
