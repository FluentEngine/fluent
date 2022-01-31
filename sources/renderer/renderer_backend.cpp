#include <algorithm>
#include <SDL_vulkan.h>
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "imgui_impl_sdl.h"
#include "tinyimageformat_apis.h"
#include "core/window.hpp"
#include "core/application.hpp"
#include "renderer/renderer_backend.hpp"

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

VkPolygonMode to_vk_polygon_mode(PolygonMode mode)
{
    switch (mode)
    {
    case PolygonMode::eFill:
        return VK_POLYGON_MODE_FILL;
    case PolygonMode::eLine:
        return VK_POLYGON_MODE_LINE;
    default:
        FT_ASSERT(false);
        return VkPolygonMode(-1);
    }
};

static inline VkAccessFlags determine_access_flags(ResourceState resource_state)
{
    VkAccessFlags access_flags = 0;

    if (b32(resource_state & ResourceState::eGeneral))
        access_flags |= (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT);

    if (b32(resource_state & ResourceState::eColorAttachment))
        access_flags |= (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

    if (b32(resource_state & ResourceState::eDepthStencilWrite))
        access_flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    if (b32(resource_state & ResourceState::eDepthStencilReadOnly))
        access_flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

    if (b32(resource_state & ResourceState::eShaderReadOnly))
        access_flags |= VK_ACCESS_SHADER_READ_BIT;

    if (b32(resource_state & ResourceState::eTransferSrc))
        access_flags |= VK_ACCESS_TRANSFER_READ_BIT;

    if (b32(resource_state & ResourceState::eTransferDst))
        access_flags |= VK_ACCESS_TRANSFER_WRITE_BIT;

    if (b32(resource_state & ResourceState::ePresent))
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
    if (b32(resource_state & ResourceState::eGeneral))
        return VK_IMAGE_LAYOUT_GENERAL;

    if (b32(resource_state & ResourceState::eColorAttachment))
        return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    if (b32(resource_state & ResourceState::eDepthStencilWrite))
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    if (b32(resource_state & ResourceState::eDepthStencilReadOnly))
        return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

    if (b32(resource_state & ResourceState::eShaderReadOnly))
        return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

    if (b32(resource_state & ResourceState::ePresent))
        return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    if (b32(resource_state & ResourceState::eTransferSrc))
        return VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    if (b32(resource_state & ResourceState::eTransferDst))
        return VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

    return VK_IMAGE_LAYOUT_UNDEFINED;
}

static inline VmaMemoryUsage determine_vma_memory_usage(DescriptorType descriptor_type)
{
    // TODO: determine memory usage
    VmaMemoryUsage memory_usage = VMA_MEMORY_USAGE_GPU_ONLY;

    // can be used as source in transfer operation
    if (b32(descriptor_type & DescriptorType::eHostVisibleBuffer))
    {
        memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    }

    if (b32(descriptor_type & DescriptorType::eUniformBuffer))
    {
        memory_usage = VMA_MEMORY_USAGE_CPU_TO_GPU;
    }

    return memory_usage;
}

static inline VkBufferUsageFlags determine_vk_buffer_usage(DescriptorType descriptor_type)
{
    // TODO: determine buffer usage flags
    VkBufferUsageFlags buffer_usage = VkBufferUsageFlags(0);

    if (b32(descriptor_type & DescriptorType::eHostVisibleBuffer))
    {
        buffer_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    }

    if (b32(descriptor_type & DescriptorType::eDeviceLocalBuffer))
    {
        buffer_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    }

    if (b32(descriptor_type & DescriptorType::eVertexBuffer))
    {
        buffer_usage |= (VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    }

    if (b32(descriptor_type & DescriptorType::eIndexBuffer))
    {
        buffer_usage |= (VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT);
    }

    if (b32(descriptor_type & DescriptorType::eUniformBuffer))
    {
        buffer_usage |=
            (VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT);
    }

    return buffer_usage;
}

static inline VkImageUsageFlags determine_vk_image_usage(DescriptorType descriptor_type)
{
    VkImageUsageFlags image_usage = VkImageUsageFlags(0);

    if (b32(descriptor_type & DescriptorType::eSampledImage))
    {
        image_usage |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    if (b32(descriptor_type & DescriptorType::eStorageImage))
    {
        image_usage |= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    }

    if (b32(descriptor_type & DescriptorType::eInputAttachment))
    {
        image_usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
    }

    if (b32(descriptor_type & DescriptorType::eColorAttachment))
    {
        image_usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    }

    if (b32(descriptor_type & DescriptorType::eDepthStencilAttachment))
    {
        image_usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
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

void create_debug_messenger(RendererBackend* renderer)
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
    debug_messenger_create_info.pUserData       = nullptr;

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
        b32 result               = SDL_Vulkan_GetInstanceExtensions(
                          ( SDL_Window* ) get_app_window()->handle, &sdl_extensions_count, extension_names);
        FT_ASSERT(result);
        extension_names[ extensions_count - 1 ] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
#else
        b32 result = SDL_Vulkan_GetInstanceExtensions(
            ( SDL_Window* ) get_app_window()->handle, *extensions_count, extension_names);
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

static inline VkImageSubresourceRange get_image_subresource_range(const BaseImage* image)
{
    VkImageSubresourceRange image_subresource_range{};
    image_subresource_range.aspectMask     = get_aspect_mask(image->format);
    image_subresource_range.baseMipLevel   = 0;
    image_subresource_range.levelCount     = image->mip_level_count;
    image_subresource_range.baseArrayLayer = 0;
    image_subresource_range.layerCount     = image->layer_count;

    return image_subresource_range;
}

VkImageSubresourceLayers get_image_subresource_layers(const BaseImage* image)
{
    auto subresourceRange = get_image_subresource_range(image);
    return VkImageSubresourceLayers{ subresourceRange.aspectMask, subresourceRange.baseMipLevel,
                                     subresourceRange.baseArrayLayer, subresourceRange.layerCount };
}

void create_renderer_backend(const RendererBackendDesc* desc, RendererBackend** p_renderer)
{
    FT_ASSERT(p_renderer);

    *p_renderer               = new (std::nothrow) RendererBackend{};
    RendererBackend* renderer = *p_renderer;

    renderer->vulkan_allocator = desc->vulkan_allocator;

    volkInitialize();

    VkApplicationInfo app_info{};
    app_info.sType              = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pNext              = nullptr;
    app_info.pApplicationName   = get_app_name();
    app_info.applicationVersion = VK_MAKE_VERSION(0, 0, 1);
    app_info.pEngineName        = "Fluent";
    app_info.engineVersion      = VK_MAKE_VERSION(0, 0, 1);
    app_info.apiVersion         = FLUENT_VULKAN_API_VERSION;

    u32 extensions_count = 0;
    get_instance_extensions(extensions_count, nullptr);
    const char** extensions = ( const char** ) alloca(extensions_count * sizeof(const char*));
    get_instance_extensions(extensions_count, extensions);

    u32 layers_count = 0;
    get_instance_layers(layers_count, nullptr);
    const char** layers = ( const char** ) alloca(layers_count * sizeof(const char*));
    get_instance_layers(layers_count, layers);

    VkInstanceCreateInfo instance_create_info{};
    instance_create_info.sType                   = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instance_create_info.pNext                   = nullptr;
    instance_create_info.pApplicationInfo        = &app_info;
    instance_create_info.enabledLayerCount       = layers_count;
    instance_create_info.ppEnabledLayerNames     = layers;
    instance_create_info.enabledExtensionCount   = extensions_count;
    instance_create_info.ppEnabledExtensionNames = extensions;
    instance_create_info.flags                   = 0;

    VK_ASSERT(vkCreateInstance(&instance_create_info, renderer->vulkan_allocator, &renderer->instance));

    volkLoadInstance(renderer->instance);

#ifdef FLUENT_DEBUG
    create_debug_messenger(renderer);
#endif

    // pick physical device
    renderer->physical_device = VK_NULL_HANDLE;
    u32 device_count          = 0;
    vkEnumeratePhysicalDevices(renderer->instance, &device_count, nullptr);
    FT_ASSERT(device_count != 0);
    VkPhysicalDevice* physical_devices = ( VkPhysicalDevice* ) alloca(device_count * sizeof(VkPhysicalDevice));
    vkEnumeratePhysicalDevices(renderer->instance, &device_count, physical_devices);

    renderer->physical_device = physical_devices[ 0 ];

    // select best physical device
    for (u32 i = 0; i < device_count; ++i)
    {
        VkPhysicalDeviceProperties deviceProperties;
        VkPhysicalDeviceFeatures   deviceFeatures;
        vkGetPhysicalDeviceProperties(physical_devices[ i ], &deviceProperties);
        vkGetPhysicalDeviceFeatures(physical_devices[ i ], &deviceFeatures);

        if (deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
        {
            renderer->physical_device = physical_devices[ i ];
            break;
        }
    }
}

void destroy_renderer_backend(RendererBackend* renderer)
{
    FT_ASSERT(renderer);
#ifdef FLUENT_DEBUG
    vkDestroyDebugUtilsMessengerEXT(renderer->instance, renderer->debug_messenger, renderer->vulkan_allocator);
#endif
    vkDestroyInstance(renderer->instance, renderer->vulkan_allocator);
    delete renderer;
}

void map_memory(const Device* device, BaseBuffer* buffer)
{
    FT_ASSERT(buffer->mapped_memory == nullptr);
    vmaMapMemory(device->memory_allocator, buffer->allocation, &buffer->mapped_memory);
}

void unmap_memory(const Device* device, BaseBuffer* buffer)
{
    FT_ASSERT(buffer->mapped_memory);
    vmaUnmapMemory(device->memory_allocator, buffer->allocation);
    buffer->mapped_memory = nullptr;
}

void create_device(const RendererBackend* renderer, const DeviceDesc* desc, Device** p_device)
{
    FT_ASSERT(p_device);
    FT_ASSERT(desc->frame_in_use_count > 0);

    *p_device      = new (std::nothrow) Device{};
    Device* device = *p_device;

    device->vulkan_allocator = renderer->vulkan_allocator;
    device->instance         = renderer->instance;
    device->physical_device  = renderer->physical_device;

    u32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &queue_family_count, nullptr);
    VkQueueFamilyProperties* queue_families =
        ( VkQueueFamilyProperties* ) alloca(queue_family_count * sizeof(VkQueueFamilyProperties));
    vkGetPhysicalDeviceQueueFamilyProperties(device->physical_device, &queue_family_count, queue_families);

    u32                     queue_create_info_count = 0;
    VkDeviceQueueCreateInfo queue_create_infos[ static_cast<int>(QueueType::eLast) ];
    f32                     queue_priority = 1.0f;

    // TODO: Select queues
    for (u32 i = 0; i < queue_family_count; ++i)
    {
        queue_create_infos[ queue_create_info_count ]                  = {};
        queue_create_infos[ queue_create_info_count ].sType            = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queue_create_infos[ queue_create_info_count ].pNext            = nullptr;
        queue_create_infos[ queue_create_info_count ].flags            = 0;
        queue_create_infos[ queue_create_info_count ].queueCount       = 1;
        queue_create_infos[ queue_create_info_count ].pQueuePriorities = &queue_priority;
        queue_create_infos[ queue_create_info_count ].queueFamilyIndex = i;
        queue_create_info_count++;

        if (queue_create_info_count == static_cast<i32>(QueueType::eLast))
            break;
    }

    static const u32 device_extension_count                      = 2;
    const char*      device_extensions[ device_extension_count ] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,
                                                                VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME };

    // TODO: check support
    VkPhysicalDeviceFeatures used_features{};
    used_features.fillModeNonSolid = true;

    VkPhysicalDeviceDescriptorIndexingFeatures descriptor_indexing_features{};
    descriptor_indexing_features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT;
    descriptor_indexing_features.descriptorBindingPartiallyBound = true;

    VkPhysicalDeviceMultiviewFeatures multiview_features{};
    multiview_features.sType     = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES;
    multiview_features.multiview = true;
    multiview_features.pNext     = &descriptor_indexing_features;

    VkDeviceCreateInfo device_create_info{};
    device_create_info.sType                   = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pNext                   = &multiview_features;
    device_create_info.flags                   = 0;
    device_create_info.queueCreateInfoCount    = queue_create_info_count;
    device_create_info.pQueueCreateInfos       = queue_create_infos;
    device_create_info.enabledLayerCount       = 0;
    device_create_info.ppEnabledLayerNames     = nullptr;
    device_create_info.enabledExtensionCount   = device_extension_count;
    device_create_info.ppEnabledExtensionNames = device_extensions;
    device_create_info.pEnabledFeatures        = &used_features;

    VK_ASSERT(vkCreateDevice(
        device->physical_device, &device_create_info, device->vulkan_allocator, &device->logical_device));

    volkLoadDevice(device->logical_device);

    VmaAllocatorCreateInfo vma_allocator_create_info{};
    vma_allocator_create_info.instance             = device->instance;
    vma_allocator_create_info.physicalDevice       = device->physical_device;
    vma_allocator_create_info.device               = device->logical_device;
    vma_allocator_create_info.flags                = 0;
    vma_allocator_create_info.pAllocationCallbacks = device->vulkan_allocator;
    vma_allocator_create_info.frameInUseCount      = desc->frame_in_use_count;
    vma_allocator_create_info.vulkanApiVersion     = FLUENT_VULKAN_API_VERSION;

    VK_ASSERT(vmaCreateAllocator(&vma_allocator_create_info, &device->memory_allocator));

    static constexpr u32 pool_size_count               = 11;
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
    descriptor_pool_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.pNext         = nullptr;
    descriptor_pool_create_info.flags         = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    descriptor_pool_create_info.maxSets       = 2048 * pool_size_count;
    descriptor_pool_create_info.poolSizeCount = pool_size_count;
    descriptor_pool_create_info.pPoolSizes    = pool_sizes;

    VK_ASSERT(vkCreateDescriptorPool(
        device->logical_device, &descriptor_pool_create_info, device->vulkan_allocator, &device->descriptor_pool));

    // TODO: move it to transfer queue
    QueueDesc queue_desc{};
    queue_desc.queue_type = QueueType::eGraphics;

    create_queue(device, &queue_desc, &device->queue);

    CommandPoolDesc command_pool_desc{};
    command_pool_desc.queue = device->queue;
    create_command_pool(device, &command_pool_desc, &device->command_pool);

    create_command_buffers(device, device->command_pool, 1, &device->cmd);
}

void destroy_device(Device* device)
{
    FT_ASSERT(device);
    FT_ASSERT(device->descriptor_pool);
    FT_ASSERT(device->memory_allocator);
    FT_ASSERT(device->logical_device);
    destroy_command_buffers(device, device->command_pool, 1, &device->cmd);
    destroy_command_pool(device, device->command_pool);
    destroy_queue(device->queue);
    vkDestroyDescriptorPool(device->logical_device, device->descriptor_pool, device->vulkan_allocator);
    vmaDestroyAllocator(device->memory_allocator);
    vkDestroyDevice(device->logical_device, device->vulkan_allocator);
    delete device;
}

void device_wait_idle(const Device* device)
{
    vkDeviceWaitIdle(device->logical_device);
}

void create_queue(const Device* device, const QueueDesc* desc, Queue** p_queue)
{
    FT_ASSERT(p_queue);
    *p_queue     = new (std::nothrow) Queue{};
    Queue* queue = *p_queue;

    u32 index = find_queue_family_index(device->physical_device, desc->queue_type);

    queue->family_index = index;
    queue->type         = desc->queue_type;
    vkGetDeviceQueue(device->logical_device, index, 0, &queue->queue);
}

void destroy_queue(Queue* queue)
{
    FT_ASSERT(queue);
    delete queue;
}

void queue_wait_idle(const Queue* queue)
{
    vkQueueWaitIdle(queue->queue);
}

void queue_submit(const Queue* queue, const QueueSubmitDesc* desc)
{
    VkPipelineStageFlags wait_dst_stage_mask = VK_PIPELINE_STAGE_TRANSFER_BIT;
    VkSemaphore*         wait_semaphores = ( VkSemaphore* ) alloca(desc->wait_semaphore_count * sizeof(VkSemaphore));
    VkCommandBuffer*     command_buffers =
        ( VkCommandBuffer* ) alloca(desc->command_buffer_count * sizeof(VkCommandBuffer));
    VkSemaphore* signal_semaphores = ( VkSemaphore* ) alloca(desc->signal_semaphore_count * sizeof(VkSemaphore));

    for (u32 i = 0; i < desc->wait_semaphore_count; ++i)
    {
        wait_semaphores[ i ] = desc->wait_semaphores[ i ].semaphore;
    }

    for (u32 i = 0; i < desc->command_buffer_count; ++i)
    {
        command_buffers[ i ] = desc->command_buffers[ i ].command_buffer;
    }

    for (u32 i = 0; i < desc->signal_semaphore_count; ++i)
    {
        signal_semaphores[ i ] = desc->signal_semaphores[ i ].semaphore;
    }

    VkSubmitInfo submit_info         = {};
    submit_info.sType                = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.pNext                = nullptr;
    submit_info.waitSemaphoreCount   = desc->wait_semaphore_count;
    submit_info.pWaitSemaphores      = wait_semaphores;
    submit_info.pWaitDstStageMask    = &wait_dst_stage_mask;
    submit_info.commandBufferCount   = desc->command_buffer_count;
    submit_info.pCommandBuffers      = command_buffers;
    submit_info.signalSemaphoreCount = desc->signal_semaphore_count;
    submit_info.pSignalSemaphores    = signal_semaphores;

    vkQueueSubmit(queue->queue, 1, &submit_info, desc->signal_fence ? desc->signal_fence->fence : VK_NULL_HANDLE);
}

void immediate_submit(const Queue* queue, const CommandBuffer* cmd)
{
    QueueSubmitDesc queue_submit_desc{};
    queue_submit_desc.command_buffer_count = 1;
    queue_submit_desc.command_buffers      = cmd;
    queue_submit(queue, &queue_submit_desc);
    queue_wait_idle(queue);
}

void queue_present(const Queue* queue, const QueuePresentDesc* desc)
{
    VkSemaphore* wait_semaphores = ( VkSemaphore* ) alloca(desc->wait_semaphore_count * sizeof(VkSemaphore));

    for (u32 i = 0; i < desc->wait_semaphore_count; ++i)
    {
        wait_semaphores[ i ] = desc->wait_semaphores[ i ].semaphore;
    }

    VkPresentInfoKHR present_info = {};

    present_info.sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.pNext              = nullptr;
    present_info.waitSemaphoreCount = desc->wait_semaphore_count;
    present_info.pWaitSemaphores    = wait_semaphores;
    present_info.swapchainCount     = 1;
    present_info.pSwapchains        = &desc->swapchain->swapchain;
    present_info.pImageIndices      = &desc->image_index;
    present_info.pResults           = nullptr;

    vkQueuePresentKHR(queue->queue, &present_info);
}

void create_semaphore(const Device* device, Semaphore** p_semaphore)
{
    FT_ASSERT(p_semaphore);
    *p_semaphore         = new (std::nothrow) Semaphore{};
    Semaphore* semaphore = *p_semaphore;

    VkSemaphoreCreateInfo semaphore_create_info{};
    semaphore_create_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    semaphore_create_info.pNext = nullptr;
    semaphore_create_info.flags = 0;

    VK_ASSERT(vkCreateSemaphore(
        device->logical_device, &semaphore_create_info, device->vulkan_allocator, &semaphore->semaphore));
}

void destroy_semaphore(const Device* device, Semaphore* semaphore)
{
    FT_ASSERT(semaphore);
    FT_ASSERT(semaphore->semaphore);
    vkDestroySemaphore(device->logical_device, semaphore->semaphore, device->vulkan_allocator);
    delete semaphore;
}

void create_fence(const Device* device, Fence** p_fence)
{
    FT_ASSERT(p_fence);
    *p_fence     = new (std::nothrow) Fence{};
    Fence* fence = *p_fence;

    VkFenceCreateInfo fence_create_info{};
    fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_create_info.pNext = nullptr;
    fence_create_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VK_ASSERT(vkCreateFence(device->logical_device, &fence_create_info, device->vulkan_allocator, &fence->fence));
}

void destroy_fence(const Device* device, Fence* fence)
{
    FT_ASSERT(fence);
    FT_ASSERT(fence->fence);
    vkDestroyFence(device->logical_device, fence->fence, device->vulkan_allocator);
    delete fence;
}

void wait_for_fences(const Device* device, u32 count, Fence* fences)
{
    VkFence* vk_fences = ( VkFence* ) alloca(count * sizeof(VkFence));
    for (u32 i = 0; i < count; ++i)
    {
        vk_fences[ i ] = fences[ i ].fence;
    }

    vkWaitForFences(device->logical_device, count, vk_fences, true, std::numeric_limits<u64>::max());
}

void reset_fences(const Device* device, u32 count, Fence* fences)
{
    VkFence* vk_fences = ( VkFence* ) alloca(count * sizeof(VkFence));
    for (u32 i = 0; i < count; ++i)
    {
        vk_fences[ i ] = fences[ i ].fence;
    }

    vkResetFences(device->logical_device, count, vk_fences);
}

void configure_swapchain(const Device* device, Swapchain* swapchain, const SwapchainDesc* desc)
{
    swapchain->queue = desc->queue;

    SDL_Vulkan_CreateSurface(( SDL_Window* ) get_app_window()->handle, device->instance, &swapchain->surface);

    VkBool32 support_surface = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(
        device->physical_device, desc->queue->family_index, swapchain->surface, &support_surface);

    FT_ASSERT(support_surface);

    // find best present mode
    uint32_t present_mode_count = 0;
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device->physical_device, swapchain->surface, &present_mode_count, nullptr);
    VkPresentModeKHR* present_modes = ( VkPresentModeKHR* ) alloca(present_mode_count * sizeof(VkPresentModeKHR));
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device->physical_device, swapchain->surface, &present_mode_count, present_modes);

    swapchain->present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR;

    for (u32 i = 0; i < present_mode_count; ++i)
    {
        if (present_modes[ i ] == VK_PRESENT_MODE_MAILBOX_KHR)
        {
            swapchain->present_mode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
    }

    // determine present image count
    VkSurfaceCapabilitiesKHR surface_capabilities{};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->physical_device, swapchain->surface, &surface_capabilities);

    // determine swapchain size
    swapchain->width =
        std::clamp(desc->width, surface_capabilities.minImageExtent.width, surface_capabilities.maxImageExtent.width);
    swapchain->height = std::clamp(
        desc->height, surface_capabilities.minImageExtent.height, surface_capabilities.maxImageExtent.height);

    swapchain->min_image_count =
        std::clamp(desc->min_image_count, surface_capabilities.minImageCount, surface_capabilities.maxImageCount);

    /// find best surface format
    u32 surface_format_count = 0;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device->physical_device, swapchain->surface, &surface_format_count, nullptr);
    VkSurfaceFormatKHR* surface_formats =
        ( VkSurfaceFormatKHR* ) alloca(surface_format_count * sizeof(VkSurfaceFormatKHR));

    vkGetPhysicalDeviceSurfaceFormatsKHR(
        device->physical_device, swapchain->surface, &surface_format_count, surface_formats);

    VkSurfaceFormatKHR surface_format = surface_formats[ 0 ];
    for (u32 i = 0; i < surface_format_count; ++i)
    {
        if (surface_formats[ i ].format == VK_FORMAT_R8G8B8A8_UNORM ||
            surface_formats[ i ].format == VK_FORMAT_B8G8R8A8_UNORM)
            surface_format = surface_formats[ i ];
    }

    swapchain->format      = from_vk_format(surface_format.format);
    swapchain->color_space = surface_format.colorSpace;

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
    swapchain->pre_transform = pre_transform;
}

void create_configured_swapchain(const Device* device, Swapchain* swapchain, b32 resize, b32 builtin_depth)
{
    // destroy old resources if it is resize
    if (resize)
    {
        if (builtin_depth)
        {
            destroy_image(device, swapchain->depth_image);
        }

        FT_ASSERT(swapchain->image_count);
        for (u32 i = 0; i < swapchain->image_count; ++i)
        {
            FT_ASSERT(swapchain->images[ i ]->image_view);
            vkDestroyImageView(device->logical_device, swapchain->images[ i ]->image_view, device->vulkan_allocator);
            delete swapchain->images[ i ];
        }
    }

    // create new
    VkSwapchainCreateInfoKHR swapchain_create_info{};
    swapchain_create_info.sType                 = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_create_info.pNext                 = nullptr;
    swapchain_create_info.flags                 = 0;
    swapchain_create_info.surface               = swapchain->surface;
    swapchain_create_info.minImageCount         = swapchain->min_image_count;
    swapchain_create_info.imageFormat           = to_vk_format(swapchain->format);
    swapchain_create_info.imageColorSpace       = swapchain->color_space;
    swapchain_create_info.imageExtent.width     = swapchain->width;
    swapchain_create_info.imageExtent.height    = swapchain->height;
    swapchain_create_info.imageArrayLayers      = 1;
    swapchain_create_info.imageUsage            = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    swapchain_create_info.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
    swapchain_create_info.queueFamilyIndexCount = 1;
    swapchain_create_info.pQueueFamilyIndices   = &swapchain->queue->family_index;
    swapchain_create_info.preTransform          = swapchain->pre_transform;
    // TODO: choose composite alpha according to caps
    swapchain_create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_create_info.presentMode    = swapchain->present_mode;
    swapchain_create_info.clipped        = true;
    swapchain_create_info.oldSwapchain   = nullptr;

    VK_ASSERT(vkCreateSwapchainKHR(
        device->logical_device, &swapchain_create_info, device->vulkan_allocator, &swapchain->swapchain));

    vkGetSwapchainImagesKHR(device->logical_device, swapchain->swapchain, &swapchain->image_count, nullptr);
    VkImage* swapchain_images = ( VkImage* ) alloca(swapchain->image_count * sizeof(VkImage));
    vkGetSwapchainImagesKHR(device->logical_device, swapchain->swapchain, &swapchain->image_count, swapchain_images);
    if (!resize)
    {
        swapchain->images = new (std::nothrow) BaseImage*[ swapchain->image_count ];
    }

    VkImageViewCreateInfo image_view_create_info{};
    image_view_create_info.sType                           = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext                           = nullptr;
    image_view_create_info.flags                           = 0;
    image_view_create_info.viewType                        = VK_IMAGE_VIEW_TYPE_2D;
    image_view_create_info.format                          = to_vk_format(swapchain->format);
    image_view_create_info.components.r                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a                    = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange.aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT;
    image_view_create_info.subresourceRange.baseMipLevel   = 0;
    image_view_create_info.subresourceRange.levelCount     = 1;
    image_view_create_info.subresourceRange.baseArrayLayer = 0;
    image_view_create_info.subresourceRange.layerCount     = 1;

    for (u32 i = 0; i < swapchain->image_count; ++i)
    {
        image_view_create_info.image = swapchain_images[ i ];

        swapchain->images[ i ]                  = new (std::nothrow) BaseImage{};
        swapchain->images[ i ]->image           = swapchain_images[ i ];
        swapchain->images[ i ]->width           = swapchain->width;
        swapchain->images[ i ]->height          = swapchain->height;
        swapchain->images[ i ]->format          = swapchain->format;
        swapchain->images[ i ]->sample_count    = SampleCount::e1;
        swapchain->images[ i ]->mip_level_count = 1;
        swapchain->images[ i ]->layer_count     = 1;
        swapchain->images[ i ]->descriptor_type = DescriptorType::eSampledImage;

        VK_ASSERT(vkCreateImageView(
            device->logical_device, &image_view_create_info, device->vulkan_allocator,
            &swapchain->images[ i ]->image_view));
    }

    if (builtin_depth)
    {
        ImageDesc image_desc{};
        image_desc.width           = swapchain->width;
        image_desc.height          = swapchain->height;
        image_desc.depth           = 1;
        image_desc.layer_count     = 1;
        image_desc.mip_levels      = 1;
        image_desc.format          = Format::eD32Sfloat;
        image_desc.descriptor_type = DescriptorType::eDepthStencilAttachment;

        create_image(device, &image_desc, &swapchain->depth_image);

        ImageBarrier image_barrier{};
        image_barrier.image     = swapchain->depth_image;
        image_barrier.src_queue = device->queue;
        image_barrier.dst_queue = device->queue;
        image_barrier.old_state = ResourceState::eUndefined;
        image_barrier.new_state = ResourceState::eDepthStencilWrite;

        begin_command_buffer(device->cmd);
        cmd_barrier(device->cmd, 0, nullptr, 1, &image_barrier);
        end_command_buffer(device->cmd);
        immediate_submit(device->queue, device->cmd);
    }

    // create default render passes
    RenderPassDesc render_pass_desc{};
    render_pass_desc.color_attachment_count         = 1;
    render_pass_desc.color_attachment_load_ops[ 0 ] = AttachmentLoadOp::eClear;
    render_pass_desc.color_image_states[ 0 ]        = ResourceState::eColorAttachment;
    render_pass_desc.depth_stencil                  = builtin_depth ? swapchain->depth_image : nullptr;
    render_pass_desc.depth_stencil_load_op          = AttachmentLoadOp::eClear;
    render_pass_desc.depth_stencil_state            = ResourceState::eDepthStencilWrite;
    render_pass_desc.width                          = swapchain->width;
    render_pass_desc.height                         = swapchain->height;

    if (!resize)
    {
        swapchain->render_passes = new (std::nothrow) RenderPass*[ swapchain->image_count ];

        for (u32 i = 0; i < swapchain->image_count; ++i)
        {
            render_pass_desc.color_attachments[ 0 ] = swapchain->images[ i ];
            create_render_pass(device, &render_pass_desc, &swapchain->render_passes[ i ]);
        }
    }
    else
    {
        for (u32 i = 0; i < swapchain->image_count; ++i)
        {
            render_pass_desc.color_attachments[ 0 ] = swapchain->images[ i ];
            update_render_pass(device, swapchain->render_passes[ i ], &render_pass_desc);
        }
    }
}

void create_swapchain(const Device* device, const SwapchainDesc* desc, Swapchain** p_swapchain)
{
    FT_ASSERT(p_swapchain);
    *p_swapchain         = new (std::nothrow) Swapchain{};
    Swapchain* swapchain = *p_swapchain;

    configure_swapchain(device, swapchain, desc);
    create_configured_swapchain(device, swapchain, false, desc->builtin_depth);
}

void resize_swapchain(const Device* device, Swapchain* swapchain, u32 width, u32 height)
{
    swapchain->width  = width;
    swapchain->height = height;
    create_configured_swapchain(device, swapchain, true, swapchain->depth_image != nullptr);
}

void destroy_swapchain(const Device* device, Swapchain* swapchain)
{
    FT_ASSERT(swapchain);
    FT_ASSERT(swapchain->render_passes);
    for (u32 i = 0; i < swapchain->image_count; ++i)
    {
        destroy_render_pass(device, swapchain->render_passes[ i ]);
    }
    delete[] swapchain->render_passes;

    if (swapchain->depth_image)
    {
        destroy_image(device, swapchain->depth_image);
    }

    FT_ASSERT(swapchain->image_count);
    for (u32 i = 0; i < swapchain->image_count; ++i)
    {
        FT_ASSERT(swapchain->images[ i ]->image_view);
        vkDestroyImageView(device->logical_device, swapchain->images[ i ]->image_view, device->vulkan_allocator);
        delete swapchain->images[ i ];
    }
    delete[] swapchain->images;
    FT_ASSERT(swapchain->swapchain);
    vkDestroySwapchainKHR(device->logical_device, swapchain->swapchain, device->vulkan_allocator);
    FT_ASSERT(swapchain->surface);
    vkDestroySurfaceKHR(device->instance, swapchain->surface, device->vulkan_allocator);

    delete swapchain;
}

const RenderPass* get_swapchain_render_pass(const Swapchain* swapchain, u32 image_index)
{
    FT_ASSERT(image_index < swapchain->image_count);
    return swapchain->render_passes[ image_index ];
}

void create_command_pool(const Device* device, const CommandPoolDesc* desc, CommandPool** p_command_pool)
{
    FT_ASSERT(p_command_pool);
    *p_command_pool           = new (std::nothrow) CommandPool{};
    CommandPool* command_pool = *p_command_pool;

    command_pool->queue = desc->queue;

    VkCommandPoolCreateInfo command_pool_create_info{};
    command_pool_create_info.sType            = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.pNext            = nullptr;
    command_pool_create_info.flags            = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = desc->queue->family_index;

    VK_ASSERT(vkCreateCommandPool(
        device->logical_device, &command_pool_create_info, device->vulkan_allocator, &command_pool->command_pool));
}

void destroy_command_pool(const Device* device, CommandPool* command_pool)
{
    FT_ASSERT(command_pool);
    FT_ASSERT(command_pool->command_pool);
    vkDestroyCommandPool(device->logical_device, command_pool->command_pool, device->vulkan_allocator);
    delete command_pool;
}

void create_command_buffers(
    const Device* device, const CommandPool* command_pool, u32 count, CommandBuffer** command_buffers)
{
    FT_ASSERT(command_buffers);

    VkCommandBuffer* buffers = ( VkCommandBuffer* ) alloca(count * sizeof(VkCommandBuffer));

    VkCommandBufferAllocateInfo command_buffer_allocate_info{};
    command_buffer_allocate_info.sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.pNext              = nullptr;
    command_buffer_allocate_info.commandPool        = command_pool->command_pool;
    command_buffer_allocate_info.level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = count;

    VK_ASSERT(vkAllocateCommandBuffers(device->logical_device, &command_buffer_allocate_info, buffers));

    for (u32 i = 0; i < count; ++i)
    {
        command_buffers[ i ] = new (std::nothrow) CommandBuffer{};
        CommandBuffer* cmd   = command_buffers[ i ];

        cmd->command_buffer = buffers[ i ];
        cmd->queue          = command_pool->queue;
    }
}

void free_command_buffers(
    const Device* device, const CommandPool* command_pool, u32 count, CommandBuffer** command_buffers)
{
    FT_ASSERT(command_buffers);

    VkCommandBuffer* buffers = ( VkCommandBuffer* ) alloca(count * sizeof(VkCommandBuffer));
    for (u32 i = 0; i < count; ++i)
    {
        buffers[ i ] = command_buffers[ i ]->command_buffer;
    }

    vkFreeCommandBuffers(device->logical_device, command_pool->command_pool, count, buffers);
}

void destroy_command_buffers(
    const Device* device, const CommandPool* command_pool, u32 count, CommandBuffer** command_buffers)
{
    FT_ASSERT(command_buffers);

    for (u32 i = 0; i < count; ++i)
    {
        delete command_buffers[ i ];
    }
}

void begin_command_buffer(const CommandBuffer* cmd)
{
    VkCommandBufferBeginInfo command_buffer_begin_info{};
    command_buffer_begin_info.sType            = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    command_buffer_begin_info.pNext            = nullptr;
    command_buffer_begin_info.flags            = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    command_buffer_begin_info.pInheritanceInfo = nullptr;

    VK_ASSERT(vkBeginCommandBuffer(cmd->command_buffer, &command_buffer_begin_info));
}

void end_command_buffer(const CommandBuffer* cmd)
{
    VK_ASSERT(vkEndCommandBuffer(cmd->command_buffer));
}

void acquire_next_image(
    const Device* device, const Swapchain* swapchain, const Semaphore* semaphore, const Fence* fence, u32* image_index)
{
    VkResult result = vkAcquireNextImageKHR(
        device->logical_device, swapchain->swapchain, std::numeric_limits<u64>::max(), semaphore->semaphore,
        fence ? fence->fence : VK_NULL_HANDLE, image_index);
}

void create_framebuffer(const Device* device, RenderPass* render_pass, const RenderPassDesc* desc)
{
    u32 attachment_count = desc->color_attachment_count;

    VkImageView image_views[ MAX_ATTACHMENTS_COUNT + 1 ];
    for (u32 i = 0; i < attachment_count; ++i)
    {
        image_views[ i ] = desc->color_attachments[ i ]->image_view;
    }

    if (desc->depth_stencil)
    {
        image_views[ attachment_count++ ] = desc->depth_stencil->image_view;
    }

    if (desc->width > 0 && desc->height > 0)
    {
        VkFramebufferCreateInfo framebuffer_create_info{};
        framebuffer_create_info.sType           = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebuffer_create_info.pNext           = nullptr;
        framebuffer_create_info.flags           = 0;
        framebuffer_create_info.renderPass      = render_pass->render_pass;
        framebuffer_create_info.attachmentCount = attachment_count;
        framebuffer_create_info.pAttachments    = image_views;
        framebuffer_create_info.width           = desc->width;
        framebuffer_create_info.height          = desc->height;
        framebuffer_create_info.layers          = 1;

        VK_ASSERT(vkCreateFramebuffer(
            device->logical_device, &framebuffer_create_info, device->vulkan_allocator, &render_pass->framebuffer));
    }
}

void create_render_pass(const Device* device, const RenderPassDesc* desc, RenderPass** p_render_pass)
{
    FT_ASSERT(p_render_pass);
    *p_render_pass          = new (std::nothrow) RenderPass{};
    RenderPass* render_pass = *p_render_pass;

    render_pass->color_attachment_count = desc->color_attachment_count;
    render_pass->width                  = desc->width;
    render_pass->height                 = desc->height;

    u32 attachments_count = desc->color_attachment_count;

    VkAttachmentDescription attachment_descriptions[ MAX_ATTACHMENTS_COUNT + 1 ];
    VkAttachmentReference   color_attachment_references[ MAX_ATTACHMENTS_COUNT ];
    VkAttachmentReference   depth_attachment_reference{};

    for (u32 i = 0; i < desc->color_attachment_count; ++i)
    {
        attachment_descriptions[ i ].flags          = 0;
        attachment_descriptions[ i ].format         = to_vk_format(desc->color_attachments[ i ]->format);
        attachment_descriptions[ i ].samples        = to_vk_sample_count(desc->color_attachments[ i ]->sample_count);
        attachment_descriptions[ i ].loadOp         = to_vk_load_op(desc->color_attachment_load_ops[ i ]);
        attachment_descriptions[ i ].storeOp        = VK_ATTACHMENT_STORE_OP_STORE;
        attachment_descriptions[ i ].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[ i ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[ i ].initialLayout  = determine_image_layout(desc->color_image_states[ i ]);
        attachment_descriptions[ i ].finalLayout    = determine_image_layout(desc->color_image_states[ i ]);

        color_attachment_references[ i ].attachment = i;
        color_attachment_references[ i ].layout     = determine_image_layout(desc->color_image_states[ i ]);
    }

    if (desc->depth_stencil)
    {
        render_pass->has_depth_stencil = true;

        u32 i                                       = attachments_count;
        attachment_descriptions[ i ].flags          = 0;
        attachment_descriptions[ i ].format         = to_vk_format(desc->depth_stencil->format);
        attachment_descriptions[ i ].samples        = to_vk_sample_count(desc->depth_stencil->sample_count);
        attachment_descriptions[ i ].loadOp         = to_vk_load_op(desc->depth_stencil_load_op);
        attachment_descriptions[ i ].storeOp        = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[ i ].stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        attachment_descriptions[ i ].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        attachment_descriptions[ i ].initialLayout  = determine_image_layout(desc->depth_stencil_state);
        attachment_descriptions[ i ].finalLayout    = determine_image_layout(desc->depth_stencil_state);

        depth_attachment_reference.attachment = i;
        depth_attachment_reference.layout     = attachment_descriptions[ i ].finalLayout;

        attachments_count++;
    }

    // TODO: subpass setup from user code
    VkSubpassDescription subpass_description{};
    subpass_description.flags                   = 0;
    subpass_description.pipelineBindPoint       = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass_description.inputAttachmentCount    = 0;
    subpass_description.pInputAttachments       = nullptr;
    subpass_description.colorAttachmentCount    = desc->color_attachment_count;
    subpass_description.pColorAttachments       = attachments_count ? color_attachment_references : nullptr;
    subpass_description.pResolveAttachments     = nullptr;
    subpass_description.pDepthStencilAttachment = desc->depth_stencil ? &depth_attachment_reference : nullptr;
    subpass_description.preserveAttachmentCount = 0;
    subpass_description.pPreserveAttachments    = nullptr;

    VkRenderPassCreateInfo render_pass_create_info{};
    render_pass_create_info.sType           = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_create_info.pNext           = nullptr;
    render_pass_create_info.flags           = 0;
    render_pass_create_info.attachmentCount = attachments_count;
    render_pass_create_info.pAttachments    = attachments_count ? attachment_descriptions : nullptr;
    render_pass_create_info.subpassCount    = 1;
    render_pass_create_info.pSubpasses      = &subpass_description;
    render_pass_create_info.dependencyCount = 0;
    render_pass_create_info.pDependencies   = nullptr;

    VK_ASSERT(vkCreateRenderPass(
        device->logical_device, &render_pass_create_info, device->vulkan_allocator, &render_pass->render_pass));

    create_framebuffer(device, render_pass, desc);
}

void update_render_pass(const Device* device, RenderPass* render_pass, const RenderPassDesc* desc)
{
    FT_ASSERT(render_pass);
    FT_ASSERT(render_pass->render_pass);
    FT_ASSERT(render_pass->framebuffer);
    FT_ASSERT(desc->width > 0 && desc->height > 0);

    render_pass->width  = desc->width;
    render_pass->height = desc->height;

    vkDestroyFramebuffer(device->logical_device, render_pass->framebuffer, device->vulkan_allocator);

    create_framebuffer(device, render_pass, desc);
}

void destroy_render_pass(const Device* device, RenderPass* render_pass)
{
    FT_ASSERT(render_pass);
    FT_ASSERT(render_pass->render_pass);
    if (render_pass->framebuffer)
    {
        vkDestroyFramebuffer(device->logical_device, render_pass->framebuffer, device->vulkan_allocator);
    }
    vkDestroyRenderPass(device->logical_device, render_pass->render_pass, device->vulkan_allocator);
    delete render_pass;
}

void create_shader(const Device* device, ShaderDesc* desc, Shader** p_shader)
{
    FT_ASSERT(p_shader);
    *p_shader      = new (std::nothrow) Shader{};
    Shader* shader = *p_shader;

    shader->stage = desc->stage;

    VkShaderModuleCreateInfo shader_create_info{};
    shader_create_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_create_info.pNext    = nullptr;
    shader_create_info.flags    = 0;
    shader_create_info.codeSize = desc->bytecode_size;
    shader_create_info.pCode    = desc->bytecode;

    VK_ASSERT(
        vkCreateShaderModule(device->logical_device, &shader_create_info, device->vulkan_allocator, &shader->shader));

    shader->reflect_data = reflect(desc->bytecode_size, desc->bytecode);
}

void destroy_shader(const Device* device, Shader* shader)
{
    FT_ASSERT(shader);
    FT_ASSERT(shader->shader);
    vkDestroyShaderModule(device->logical_device, shader->shader, device->vulkan_allocator);
    delete shader;
}

void create_descriptor_set_layout(
    const Device* device, u32 shader_count, Shader** shaders, DescriptorSetLayout** p_descriptor_set_layout)
{
    FT_ASSERT(p_descriptor_set_layout);
    FT_ASSERT(shader_count);

    *p_descriptor_set_layout                   = new (std::nothrow) DescriptorSetLayout{};
    DescriptorSetLayout* descriptor_set_layout = *p_descriptor_set_layout;

    descriptor_set_layout->shader_count = shader_count;
    descriptor_set_layout->shaders      = shaders;

    // count bindings in all shaders
    u32                      binding_counts[ MAX_SET_COUNT ]                                = { 0 };
    VkDescriptorBindingFlags binding_flags[ MAX_SET_COUNT ][ MAX_DESCRIPTOR_BINDING_COUNT ] = {};
    // collect all bindings
    VkDescriptorSetLayoutBinding bindings[ MAX_SET_COUNT ][ MAX_DESCRIPTOR_BINDING_COUNT ];

    u32 set_count = 0;

    for (u32 i = 0; i < descriptor_set_layout->shader_count; ++i)
    {
        for (u32 j = 0; j < descriptor_set_layout->shaders[ i ]->reflect_data.binding_count; ++j)
        {
            auto& binding       = descriptor_set_layout->shaders[ i ]->reflect_data.bindings[ j ];
            u32   set           = binding.set;
            u32&  binding_count = binding_counts[ set ];

            bindings[ set ][ binding_count ].binding            = binding.binding;
            bindings[ set ][ binding_count ].descriptorCount    = binding.descriptor_count;
            bindings[ set ][ binding_count ].descriptorType     = to_vk_descriptor_type(binding.descriptor_type);
            bindings[ set ][ binding_count ].pImmutableSamplers = nullptr; // ??? TODO
            bindings[ set ][ binding_count ].stageFlags =
                to_vk_shader_stage(descriptor_set_layout->shaders[ i ]->stage);

            if (binding.descriptor_count > 1)
            {
                binding_flags[ set ][ binding_count ] = VkDescriptorBindingFlags{};
                binding_flags[ set ][ binding_count ] |= VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;
            }

            binding_count++;

            if (set + 1 > set_count)
            {
                set_count = set + 1;
            }
        }
    }

    for (u32 set = 0; set < set_count; ++set)
    {
        if (binding_counts[ set ] > 0)
        {
            VkDescriptorSetLayoutBindingFlagsCreateInfo binding_flags_create_info{};
            binding_flags_create_info.sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
            binding_flags_create_info.pNext         = nullptr;
            binding_flags_create_info.bindingCount  = binding_counts[ set ];
            binding_flags_create_info.pBindingFlags = binding_flags[ set ];

            VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info{};
            descriptor_set_layout_create_info.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            descriptor_set_layout_create_info.pNext        = &binding_flags_create_info;
            descriptor_set_layout_create_info.flags        = 0;
            descriptor_set_layout_create_info.bindingCount = binding_counts[ set ];
            descriptor_set_layout_create_info.pBindings    = bindings[ set ];

            VK_ASSERT(vkCreateDescriptorSetLayout(
                device->logical_device, &descriptor_set_layout_create_info, device->vulkan_allocator,
                &descriptor_set_layout->descriptor_set_layouts[ set ]));

            descriptor_set_layout->descriptor_set_layout_count++;
        }
    }
}

void destroy_descriptor_set_layout(const Device* device, DescriptorSetLayout* layout)
{
    FT_ASSERT(layout);
    FT_ASSERT(layout->shaders);

    for (u32 i = 0; i < layout->descriptor_set_layout_count; ++i)
    {
        if (layout->descriptor_set_layouts[ i ])
        {
            vkDestroyDescriptorSetLayout(
                device->logical_device, layout->descriptor_set_layouts[ i ], device->vulkan_allocator);
        }
    }

    delete layout;
}

void create_compute_pipeline(const Device* device, const PipelineDesc* desc, Pipeline** p_pipeline)
{
    FT_ASSERT(p_pipeline);
    FT_ASSERT(desc->descriptor_set_layout);

    *p_pipeline        = new (std::nothrow) Pipeline{};
    Pipeline* pipeline = *p_pipeline;

    pipeline->type = PipelineType::eCompute;

    VkPipelineShaderStageCreateInfo shader_stage_create_info{};
    shader_stage_create_info.sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_info.pNext               = nullptr;
    shader_stage_create_info.flags               = 0;
    shader_stage_create_info.stage               = to_vk_shader_stage(desc->descriptor_set_layout->shaders[ 0 ]->stage);
    shader_stage_create_info.module              = desc->descriptor_set_layout->shaders[ 0 ]->shader;
    shader_stage_create_info.pName               = "main";
    shader_stage_create_info.pSpecializationInfo = nullptr;

    VkPushConstantRange push_constant_range{};
    push_constant_range.size = MAX_PUSH_CONSTANT_RANGE;
    push_constant_range.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount         = desc->descriptor_set_layout->descriptor_set_layout_count;
    pipeline_layout_create_info.pSetLayouts            = desc->descriptor_set_layout->descriptor_set_layouts;
    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

    VK_ASSERT(vkCreatePipelineLayout(
        device->logical_device, &pipeline_layout_create_info, nullptr, &pipeline->pipeline_layout));

    VkComputePipelineCreateInfo compute_pipeline_create_info{};
    compute_pipeline_create_info.sType  = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    compute_pipeline_create_info.stage  = shader_stage_create_info;
    compute_pipeline_create_info.layout = pipeline->pipeline_layout;

    VK_ASSERT(vkCreateComputePipelines(
        device->logical_device, {}, 1, &compute_pipeline_create_info, device->vulkan_allocator, &pipeline->pipeline));
}

void create_graphics_pipeline(const Device* device, const PipelineDesc* desc, Pipeline** p_pipeline)
{
    FT_ASSERT(p_pipeline);
    FT_ASSERT(desc->descriptor_set_layout);
    FT_ASSERT(desc->render_pass);

    *p_pipeline        = new (std::nothrow) Pipeline{};
    Pipeline* pipeline = *p_pipeline;

    pipeline->type = PipelineType::eGraphics;

    u32                             shader_stage_count = desc->descriptor_set_layout->shader_count;
    VkPipelineShaderStageCreateInfo shader_stage_create_infos[ MAX_STAGE_COUNT ];
    for (u32 i = 0; i < shader_stage_count; ++i)
    {
        shader_stage_create_infos[ i ].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        shader_stage_create_infos[ i ].pNext  = nullptr;
        shader_stage_create_infos[ i ].flags  = 0;
        shader_stage_create_infos[ i ].stage  = to_vk_shader_stage(desc->descriptor_set_layout->shaders[ i ]->stage);
        shader_stage_create_infos[ i ].module = desc->descriptor_set_layout->shaders[ i ]->shader;
        shader_stage_create_infos[ i ].pName  = "main";
        shader_stage_create_infos[ i ].pSpecializationInfo = nullptr;
    }

    const VertexLayout& vertex_layout = desc->vertex_layout;

    VkVertexInputBindingDescription binding_descriptions[ MAX_VERTEX_BINDING_COUNT ];
    for (u32 i = 0; i < vertex_layout.binding_desc_count; ++i)
    {
        binding_descriptions[ i ].binding   = vertex_layout.binding_descs[ i ].binding;
        binding_descriptions[ i ].stride    = vertex_layout.binding_descs[ i ].stride;
        binding_descriptions[ i ].inputRate = to_vk_vertex_input_rate(vertex_layout.binding_descs[ i ].input_rate);
    }

    VkVertexInputAttributeDescription attribute_descriptions[ MAX_VERTEX_ATTRIBUTE_COUNT ];
    for (u32 i = 0; i < vertex_layout.attribute_desc_count; ++i)
    {
        attribute_descriptions[ i ].location = vertex_layout.attribute_descs[ i ].location;
        attribute_descriptions[ i ].binding  = vertex_layout.attribute_descs[ i ].binding;
        attribute_descriptions[ i ].format   = to_vk_format(vertex_layout.attribute_descs[ i ].format);
        attribute_descriptions[ i ].offset   = vertex_layout.attribute_descs[ i ].offset;
    }

    VkPipelineVertexInputStateCreateInfo vertex_input_state_create_info{};
    vertex_input_state_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertex_input_state_create_info.vertexBindingDescriptionCount   = vertex_layout.binding_desc_count;
    vertex_input_state_create_info.pVertexBindingDescriptions      = binding_descriptions;
    vertex_input_state_create_info.vertexAttributeDescriptionCount = vertex_layout.attribute_desc_count;
    vertex_input_state_create_info.pVertexAttributeDescriptions    = attribute_descriptions;

    VkPipelineInputAssemblyStateCreateInfo input_assembly_state_create_info{};
    input_assembly_state_create_info.sType    = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    input_assembly_state_create_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    input_assembly_state_create_info.primitiveRestartEnable = false;

    // Dynamic states
    VkViewport viewport{};
    VkRect2D   scissor{};

    VkPipelineViewportStateCreateInfo viewport_state_create_info{};
    viewport_state_create_info.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewport_state_create_info.viewportCount = 1;
    viewport_state_create_info.pViewports    = &viewport;
    viewport_state_create_info.scissorCount  = 1;
    viewport_state_create_info.pScissors     = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterization_state_create_info{};
    rasterization_state_create_info.sType       = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterization_state_create_info.polygonMode = to_vk_polygon_mode(desc->rasterizer_desc.polygon_mode);
    rasterization_state_create_info.cullMode    = to_vk_cull_mode(desc->rasterizer_desc.cull_mode);
    rasterization_state_create_info.frontFace   = to_vk_front_face(desc->rasterizer_desc.front_face);
    rasterization_state_create_info.lineWidth   = 1.0f;

    VkPipelineMultisampleStateCreateInfo multisample_state_create_info{};
    multisample_state_create_info.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisample_state_create_info.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisample_state_create_info.minSampleShading     = 1.0f;

    VkPipelineColorBlendAttachmentState color_blend_attachment_state{};
    color_blend_attachment_state.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.colorBlendOp        = VK_BLEND_OP_ADD;
    color_blend_attachment_state.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment_state.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment_state.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo color_blend_state_create_info{};
    color_blend_state_create_info.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    color_blend_state_create_info.logicOpEnable   = false;
    color_blend_state_create_info.logicOp         = VK_LOGIC_OP_COPY;
    color_blend_state_create_info.attachmentCount = 1;
    color_blend_state_create_info.pAttachments    = &color_blend_attachment_state;

    VkPipelineDepthStencilStateCreateInfo depth_stencil_state_create_info{};
    depth_stencil_state_create_info.sType            = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depth_stencil_state_create_info.depthTestEnable  = desc->depth_state_desc.depth_test ? VK_TRUE : VK_FALSE;
    depth_stencil_state_create_info.depthWriteEnable = desc->depth_state_desc.depth_write ? VK_TRUE : VK_FALSE;
    depth_stencil_state_create_info.depthCompareOp =
        desc->depth_state_desc.depth_test ? to_vk_compare_op(desc->depth_state_desc.compare_op) : VK_COMPARE_OP_ALWAYS;
    depth_stencil_state_create_info.depthBoundsTestEnable = VK_FALSE;
    depth_stencil_state_create_info.minDepthBounds        = 0.0f; // Optional
    depth_stencil_state_create_info.maxDepthBounds        = 1.0f; // Optional
    depth_stencil_state_create_info.stencilTestEnable     = VK_FALSE;

    const uint32_t dynamic_state_count                   = 2;
    VkDynamicState dynamic_states[ dynamic_state_count ] = { VK_DYNAMIC_STATE_SCISSOR, VK_DYNAMIC_STATE_VIEWPORT };

    VkPipelineDynamicStateCreateInfo dynamic_state_create_info{};
    dynamic_state_create_info.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamic_state_create_info.dynamicStateCount = dynamic_state_count;
    dynamic_state_create_info.pDynamicStates    = dynamic_states;

    // TODO: push constant range
    VkPushConstantRange push_constant_range{};
    push_constant_range.size = MAX_PUSH_CONSTANT_RANGE;
    push_constant_range.stageFlags =
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkPipelineLayoutCreateInfo pipeline_layout_create_info{};
    pipeline_layout_create_info.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount         = desc->descriptor_set_layout->descriptor_set_layout_count;
    pipeline_layout_create_info.pSetLayouts            = desc->descriptor_set_layout->descriptor_set_layouts;
    pipeline_layout_create_info.pushConstantRangeCount = 1;
    pipeline_layout_create_info.pPushConstantRanges    = &push_constant_range;

    VK_ASSERT(vkCreatePipelineLayout(
        device->logical_device, &pipeline_layout_create_info, device->vulkan_allocator, &pipeline->pipeline_layout));

    VkGraphicsPipelineCreateInfo pipeline_create_info{};
    pipeline_create_info.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.stageCount          = shader_stage_count;
    pipeline_create_info.pStages             = shader_stage_create_infos;
    pipeline_create_info.pVertexInputState   = &vertex_input_state_create_info;
    pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;
    pipeline_create_info.pViewportState      = &viewport_state_create_info;
    pipeline_create_info.pMultisampleState   = &multisample_state_create_info;
    pipeline_create_info.pRasterizationState = &rasterization_state_create_info;
    pipeline_create_info.pColorBlendState    = &color_blend_state_create_info;
    pipeline_create_info.pDepthStencilState  = &depth_stencil_state_create_info;
    pipeline_create_info.pDynamicState       = &dynamic_state_create_info;
    pipeline_create_info.layout              = pipeline->pipeline_layout;
    pipeline_create_info.renderPass          = desc->render_pass->render_pass;

    VK_ASSERT(vkCreateGraphicsPipelines(
        device->logical_device, VK_NULL_HANDLE, 1, &pipeline_create_info, device->vulkan_allocator,
        &pipeline->pipeline));
}

void destroy_pipeline(const Device* device, Pipeline* pipeline)
{
    FT_ASSERT(pipeline);
    FT_ASSERT(pipeline->pipeline_layout);
    FT_ASSERT(pipeline->pipeline);
    vkDestroyPipelineLayout(device->logical_device, pipeline->pipeline_layout, device->vulkan_allocator);
    vkDestroyPipeline(device->logical_device, pipeline->pipeline, device->vulkan_allocator);
    delete pipeline;
}

void cmd_begin_render_pass(const CommandBuffer* cmd, const RenderPassBeginDesc* desc)
{
    FT_ASSERT(desc->render_pass);

    static VkClearValue clear_values[ MAX_ATTACHMENTS_COUNT + 1 ];
    const RenderPass*   render_pass       = desc->render_pass;
    u32                 clear_value_count = render_pass->color_attachment_count;

    for (u32 i = 0; i < render_pass->color_attachment_count; ++i)
    {
        clear_values[ i ]                    = VkClearValue{};
        clear_values[ i ].color.float32[ 0 ] = desc->clear_values[ i ].color[ 0 ];
        clear_values[ i ].color.float32[ 1 ] = desc->clear_values[ i ].color[ 1 ];
        clear_values[ i ].color.float32[ 2 ] = desc->clear_values[ i ].color[ 2 ];
        clear_values[ i ].color.float32[ 3 ] = desc->clear_values[ i ].color[ 3 ];
    }

    if (render_pass->has_depth_stencil)
    {
        clear_value_count++;
        u32 idx                                  = render_pass->color_attachment_count;
        clear_values[ idx ]                      = VkClearValue{};
        clear_values[ idx ].depthStencil.depth   = desc->clear_values[ idx ].depth;
        clear_values[ idx ].depthStencil.stencil = desc->clear_values[ idx ].stencil;
    }

    VkRenderPassBeginInfo render_pass_begin_info{};
    render_pass_begin_info.sType                    = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    render_pass_begin_info.pNext                    = nullptr;
    render_pass_begin_info.renderPass               = render_pass->render_pass;
    render_pass_begin_info.framebuffer              = render_pass->framebuffer;
    render_pass_begin_info.renderArea.extent.width  = render_pass->width;
    render_pass_begin_info.renderArea.extent.height = render_pass->height;
    render_pass_begin_info.renderArea.offset.x      = 0;
    render_pass_begin_info.renderArea.offset.y      = 0;
    render_pass_begin_info.clearValueCount          = clear_value_count;
    render_pass_begin_info.pClearValues             = clear_values;

    vkCmdBeginRenderPass(cmd->command_buffer, &render_pass_begin_info, VK_SUBPASS_CONTENTS_INLINE);
}

void cmd_end_render_pass(const CommandBuffer* cmd)
{
    vkCmdEndRenderPass(cmd->command_buffer);
}

void cmd_barrier(
    const CommandBuffer* cmd, u32 buffer_barriers_count, const BufferBarrier* buffer_barriers, u32 image_barriers_count,
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
        buffer_memory_barrier.sType               = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        buffer_memory_barrier.pNext               = nullptr;
        buffer_memory_barrier.srcAccessMask       = src_access_mask;
        buffer_memory_barrier.dstAccessMask       = dst_access_mask;
        buffer_memory_barrier.srcQueueFamilyIndex = buffer_barriers[ i ].src_queue->family_index;
        buffer_memory_barrier.dstQueueFamilyIndex = buffer_barriers[ i ].dst_queue->family_index;
        buffer_memory_barrier.buffer              = buffer_barriers[ i ].buffer->buffer;
        buffer_memory_barrier.offset              = buffer_barriers[ i ].offset;
        buffer_memory_barrier.size                = buffer_barriers[ i ].size;

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

        image_memory_barriers[ i ].sType               = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        image_memory_barriers[ i ].pNext               = nullptr;
        image_memory_barriers[ i ].srcAccessMask       = src_access_mask;
        image_memory_barriers[ i ].dstAccessMask       = dst_access_mask;
        image_memory_barriers[ i ].oldLayout           = determine_image_layout(image_barriers[ i ].old_state);
        image_memory_barriers[ i ].newLayout           = determine_image_layout(image_barriers[ i ].new_state);
        image_memory_barriers[ i ].srcQueueFamilyIndex = image_barriers[ i ].src_queue->family_index;
        image_memory_barriers[ i ].dstQueueFamilyIndex = image_barriers[ i ].dst_queue->family_index;
        image_memory_barriers[ i ].image               = image_barriers[ i ].image->image;
        image_memory_barriers[ i ].subresourceRange    = get_image_subresource_range(image_barriers[ i ].image);

        src_access |= src_access_mask;
        dst_access |= dst_access_mask;
    }

    VkPipelineStageFlags src_stage = determine_pipeline_stage_flags(src_access, cmd->queue->type);
    VkPipelineStageFlags dst_stage = determine_pipeline_stage_flags(dst_access, cmd->queue->type);

    vkCmdPipelineBarrier(
        cmd->command_buffer, src_stage, dst_stage, 0, 0, nullptr, buffer_barriers_count, buffer_memory_barriers,
        image_barriers_count, image_memory_barriers);
};

void cmd_set_scissor(const CommandBuffer* cmd, i32 x, i32 y, u32 width, u32 height)
{
    VkRect2D scissor{};
    scissor.offset.x      = x;
    scissor.offset.y      = y;
    scissor.extent.width  = width;
    scissor.extent.height = height;
    vkCmdSetScissor(cmd->command_buffer, 0, 1, &scissor);
}

void cmd_set_viewport(const CommandBuffer* cmd, f32 x, f32 y, f32 width, f32 height, f32 min_depth, f32 max_depth)
{
    VkViewport viewport{};
    viewport.x        = x;
    viewport.y        = y + height;
    viewport.width    = width;
    viewport.height   = -height;
    viewport.minDepth = min_depth;
    viewport.maxDepth = max_depth;

    vkCmdSetViewport(cmd->command_buffer, 0, 1, &viewport);
}

void cmd_bind_pipeline(const CommandBuffer* cmd, const Pipeline* pipeline)
{
    vkCmdBindPipeline(cmd->command_buffer, to_vk_pipeline_bind_point(pipeline->type), pipeline->pipeline);
}

void cmd_draw(const CommandBuffer* cmd, u32 vertex_count, u32 instance_count, u32 first_vertex, u32 first_instance)
{
    vkCmdDraw(cmd->command_buffer, vertex_count, instance_count, first_vertex, first_instance);
}

void cmd_draw_indexed(
    const CommandBuffer* cmd, u32 index_count, u32 instance_count, u32 first_index, u32 vertex_offset,
    u32 first_instance)
{
    vkCmdDrawIndexed(cmd->command_buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
}

void cmd_bind_vertex_buffer(const CommandBuffer* cmd, const BaseBuffer* buffer)
{
    static constexpr VkDeviceSize offset = 0;
    vkCmdBindVertexBuffers(cmd->command_buffer, 0, 1, &buffer->buffer, &offset);
}

void cmd_bind_index_buffer_u32(const CommandBuffer* cmd, const BaseBuffer* buffer)
{
    static constexpr VkDeviceSize offset = 0;
    vkCmdBindIndexBuffer(cmd->command_buffer, buffer->buffer, offset, VK_INDEX_TYPE_UINT32);
}

void cmd_copy_buffer(
    const CommandBuffer* cmd, const BaseBuffer* src, u32 src_offset, BaseBuffer* dst, u32 dst_offset, u32 size)
{
    VkBufferCopy buffer_copy{};
    buffer_copy.srcOffset = src_offset;
    buffer_copy.dstOffset = dst_offset;
    buffer_copy.size      = size;

    vkCmdCopyBuffer(cmd->command_buffer, src->buffer, dst->buffer, 1, &buffer_copy);
}

void cmd_copy_buffer_to_image(const CommandBuffer* cmd, const BaseBuffer* src, u32 src_offset, BaseImage* dst)
{
    auto dst_layers = get_image_subresource_layers(dst);

    VkBufferImageCopy buffer_to_image_copy_info{};
    buffer_to_image_copy_info.bufferOffset      = src_offset;
    buffer_to_image_copy_info.bufferImageHeight = 0;
    buffer_to_image_copy_info.bufferRowLength   = 0;
    buffer_to_image_copy_info.imageSubresource  = dst_layers;
    buffer_to_image_copy_info.imageOffset       = VkOffset3D{ 0, 0, 0 };
    buffer_to_image_copy_info.imageExtent       = VkExtent3D{ dst->width, dst->height, 1 };

    vkCmdCopyBufferToImage(
        cmd->command_buffer, src->buffer, dst->image, determine_image_layout(ResourceState::eTransferDst), 1,
        &buffer_to_image_copy_info);
}

void cmd_dispatch(const CommandBuffer* cmd, u32 group_count_x, u32 group_count_y, u32 group_count_z)
{
    vkCmdDispatch(cmd->command_buffer, group_count_x, group_count_y, group_count_z);
}

void cmd_push_constants(const CommandBuffer* cmd, const Pipeline* pipeline, u32 offset, u32 size, const void* data)
{
    vkCmdPushConstants(
        cmd->command_buffer, pipeline->pipeline_layout,
        VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_COMPUTE_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, offset, size, data);
}

void cmd_blit_image(
    const CommandBuffer* cmd, const BaseImage* src, ResourceState src_state, BaseImage* dst, ResourceState dst_state,
    Filter filter)
{
    auto src_range = get_image_subresource_range(src);
    auto dst_range = get_image_subresource_range(dst);

    u32 barrier_count = 0;
    u32 index         = 0;

    ImageBarrier barriers[ 2 ] = {};
    // To transfer src
    barriers[ 0 ].src_queue = cmd->queue;
    barriers[ 0 ].dst_queue = cmd->queue;
    barriers[ 0 ].image     = src;
    barriers[ 0 ].old_state = src_state;
    barriers[ 0 ].new_state = ResourceState::eTransferSrc;
    // To transfer dst
    barriers[ 1 ].src_queue = cmd->queue;
    barriers[ 1 ].dst_queue = cmd->queue;
    barriers[ 1 ].image     = dst;
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
    image_blit_info.srcOffsets[ 1 ] = VkOffset3D{ ( i32 ) src->width, ( i32 ) src->height, 1 };
    image_blit_info.dstOffsets[ 0 ] = VkOffset3D{ 0, 0, 0 };
    image_blit_info.dstOffsets[ 1 ] = VkOffset3D{ ( i32 ) dst->width, ( i32 ) dst->height, 1 };
    image_blit_info.srcSubresource  = src_layers;
    image_blit_info.dstSubresource  = dst_layers;

    vkCmdBlitImage(
        cmd->command_buffer, src->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dst->image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &image_blit_info, to_vk_filter(filter));
}

void cmd_clear_color_image(const CommandBuffer* cmd, BaseImage* image, Vector4 color)
{
    VkClearColorValue clear_color{};
    clear_color.float32[ 0 ] = color.r;
    clear_color.float32[ 1 ] = color.g;
    clear_color.float32[ 2 ] = color.b;
    clear_color.float32[ 3 ] = color.a;

    VkImageSubresourceRange range = get_image_subresource_range(image);

    vkCmdClearColorImage(
        cmd->command_buffer, image->image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &clear_color, 1, &range);
}

void create_buffer(const Device* device, const BufferDesc* desc, BaseBuffer** p_buffer)
{
    FT_ASSERT(p_buffer);

    *p_buffer          = new (std::nothrow) BaseBuffer{};
    BaseBuffer* buffer = *p_buffer;

    buffer->size            = desc->size;
    buffer->descriptor_type = desc->descriptor_type;

    VmaAllocationCreateInfo allocation_create_info{};
    allocation_create_info.usage = determine_vma_memory_usage(desc->descriptor_type);

    VkBufferCreateInfo buffer_create_info{};
    buffer_create_info.sType                 = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.pNext                 = nullptr;
    buffer_create_info.flags                 = 0;
    buffer_create_info.size                  = desc->size;
    buffer_create_info.usage                 = determine_vk_buffer_usage(desc->descriptor_type);
    buffer_create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    buffer_create_info.queueFamilyIndexCount = 0;
    buffer_create_info.pQueueFamilyIndices   = nullptr;

    VK_ASSERT(vmaCreateBuffer(
        device->memory_allocator, &buffer_create_info, &allocation_create_info, &buffer->buffer, &buffer->allocation,
        nullptr));
}

void destroy_buffer(const Device* device, BaseBuffer* buffer)
{
    FT_ASSERT(buffer);
    FT_ASSERT(buffer->allocation);
    FT_ASSERT(buffer->buffer);
    vmaDestroyBuffer(device->memory_allocator, buffer->buffer, buffer->allocation);
    delete buffer;
}

void cmd_bind_descriptor_set(
    const CommandBuffer* cmd, u32 first_set, const DescriptorSet* set, const Pipeline* pipeline)
{
    vkCmdBindDescriptorSets(
        cmd->command_buffer, to_vk_pipeline_bind_point(pipeline->type), pipeline->pipeline_layout, first_set, 1,
        &set->descriptor_set, 0, nullptr);
}

void create_sampler(const Device* device, const SamplerDesc* desc, Sampler** p_sampler)
{
    FT_ASSERT(p_sampler);

    *p_sampler       = new (std::nothrow) Sampler{};
    Sampler* sampler = *p_sampler;

    VkSamplerCreateInfo sampler_create_info{};
    sampler_create_info.sType                   = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_create_info.pNext                   = nullptr;
    sampler_create_info.flags                   = 0;
    sampler_create_info.magFilter               = to_vk_filter(desc->mag_filter);
    sampler_create_info.minFilter               = to_vk_filter(desc->min_filter);
    sampler_create_info.mipmapMode              = to_vk_sampler_mipmap_mode(desc->mipmap_mode);
    sampler_create_info.addressModeU            = to_vk_sampler_address_mode(desc->address_mode_u);
    sampler_create_info.addressModeV            = to_vk_sampler_address_mode(desc->address_mode_v);
    sampler_create_info.addressModeW            = to_vk_sampler_address_mode(desc->address_mode_w);
    sampler_create_info.mipLodBias              = desc->mip_lod_bias;
    sampler_create_info.anisotropyEnable        = desc->anisotropy_enable;
    sampler_create_info.maxAnisotropy           = desc->max_anisotropy;
    sampler_create_info.compareEnable           = desc->compare_enable;
    sampler_create_info.compareOp               = to_vk_compare_op(desc->compare_op);
    sampler_create_info.minLod                  = desc->min_lod;
    sampler_create_info.maxLod                  = desc->max_lod;
    sampler_create_info.unnormalizedCoordinates = false;

    VK_ASSERT(
        vkCreateSampler(device->logical_device, &sampler_create_info, device->vulkan_allocator, &sampler->sampler));
}

void destroy_sampler(const Device* device, Sampler* sampler)
{
    FT_ASSERT(sampler);
    FT_ASSERT(sampler->sampler);
    vkDestroySampler(device->logical_device, sampler->sampler, device->vulkan_allocator);
    delete sampler;
}

void create_image(const Device* device, const ImageDesc* desc, BaseImage** p_image)
{
    FT_ASSERT(p_image);

    *p_image         = new (std::nothrow) BaseImage{};
    BaseImage* image = *p_image;

    VmaAllocationCreateInfo allocation_create_info{};
    allocation_create_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

    VkImageCreateInfo image_create_info{};
    image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_create_info.pNext = nullptr;
    image_create_info.flags = 0;
    // TODO: determine image type properly
    image_create_info.imageType             = VK_IMAGE_TYPE_2D;
    image_create_info.format                = to_vk_format(desc->format);
    image_create_info.extent.width          = desc->width;
    image_create_info.extent.height         = desc->height;
    image_create_info.extent.depth          = desc->depth;
    image_create_info.mipLevels             = desc->mip_levels;
    image_create_info.arrayLayers           = desc->layer_count;
    image_create_info.samples               = to_vk_sample_count(desc->sample_count);
    image_create_info.tiling                = VK_IMAGE_TILING_OPTIMAL;
    image_create_info.usage                 = determine_vk_image_usage(desc->descriptor_type);
    image_create_info.sharingMode           = VK_SHARING_MODE_EXCLUSIVE;
    image_create_info.queueFamilyIndexCount = 0;
    image_create_info.pQueueFamilyIndices   = nullptr;
    image_create_info.initialLayout         = VK_IMAGE_LAYOUT_UNDEFINED;

    if (desc->layer_count == 6)
    {
        image_create_info.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
    }

    VK_ASSERT(vmaCreateImage(
        device->memory_allocator, &image_create_info, &allocation_create_info, &image->image, &image->allocation,
        nullptr));

    image->width           = desc->width;
    image->height          = desc->height;
    image->format          = desc->format;
    image->sample_count    = desc->sample_count;
    image->mip_level_count = desc->mip_levels;
    image->layer_count     = desc->layer_count;
    image->descriptor_type = desc->descriptor_type;

    // TODO: fill properly
    VkImageViewCreateInfo image_view_create_info{};
    image_view_create_info.sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    image_view_create_info.pNext    = nullptr;
    image_view_create_info.flags    = 0;
    image_view_create_info.image    = image->image;
    image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    if (image->layer_count == 6)
    {
        image_view_create_info.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
    }
    image_view_create_info.format           = image_create_info.format;
    image_view_create_info.components.r     = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.g     = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.b     = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.components.a     = VK_COMPONENT_SWIZZLE_IDENTITY;
    image_view_create_info.subresourceRange = get_image_subresource_range(image);

    VK_ASSERT(vkCreateImageView(
        device->logical_device, &image_view_create_info, device->vulkan_allocator, &image->image_view));
}

void destroy_image(const Device* device, BaseImage* image)
{
    FT_ASSERT(image);
    FT_ASSERT(image->image_view);
    FT_ASSERT(image->image);
    FT_ASSERT(image->allocation);
    vkDestroyImageView(device->logical_device, image->image_view, device->vulkan_allocator);
    vmaDestroyImage(device->memory_allocator, image->image, image->allocation);
    delete image;
}

void create_descriptor_set(const Device* device, const DescriptorSetDesc* desc, DescriptorSet** p_descriptor_set)
{
    FT_ASSERT(p_descriptor_set);
    FT_ASSERT(desc->descriptor_set_layout);

    *p_descriptor_set             = new (std::nothrow) DescriptorSet{};
    DescriptorSet* descriptor_set = *p_descriptor_set;

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info{};
    descriptor_set_allocate_info.sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    descriptor_set_allocate_info.pNext              = nullptr;
    descriptor_set_allocate_info.descriptorPool     = device->descriptor_pool;
    descriptor_set_allocate_info.descriptorSetCount = 1;
    descriptor_set_allocate_info.pSetLayouts = &desc->descriptor_set_layout->descriptor_set_layouts[ desc->index ];

    VK_ASSERT(vkAllocateDescriptorSets(
        device->logical_device, &descriptor_set_allocate_info, &descriptor_set->descriptor_set));
}

void destroy_descriptor_set(const Device* device, DescriptorSet* set)
{
    FT_ASSERT(set);
    FT_ASSERT(set->descriptor_set);
    vkFreeDescriptorSets(device->logical_device, device->descriptor_pool, 1, &set->descriptor_set);
    delete set;
}

void update_descriptor_set(const Device* device, DescriptorSet* set, u32 count, const DescriptorWriteDesc* descs)
{
    FT_ASSERT(set);
    // TODO: rewrite
    std::vector<std::vector<VkDescriptorBufferInfo>> buffer_updates(count);
    std::vector<std::vector<VkDescriptorImageInfo>>  image_updates(count);
    std::vector<VkWriteDescriptorSet>                descriptor_writes(count);

    uint32_t buffer_update_idx = 0;
    uint32_t image_update_idx  = 0;
    uint32_t write             = 0;

    for (u32 i = 0; i < count; ++i)
    {
        const auto& descriptor_write = descs[ i ];

        auto& write_descriptor_set           = descriptor_writes[ write++ ];
        write_descriptor_set                 = {};
        write_descriptor_set.sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write_descriptor_set.dstBinding      = descriptor_write.binding;
        write_descriptor_set.descriptorCount = descriptor_write.descriptor_count;
        write_descriptor_set.dstSet          = set->descriptor_set;
        write_descriptor_set.descriptorType  = to_vk_descriptor_type(descriptor_write.descriptor_type);

        if (descriptor_write.descriptor_buffer_descs)
        {
            auto& bds = buffer_updates.emplace_back();
            bds.resize(descriptor_write.descriptor_count);

            for (u32 j = 0; j < descriptor_write.descriptor_count; ++j)
            {
                bds[ j ]        = {};
                bds[ j ].buffer = descriptor_write.descriptor_buffer_descs[ j ].buffer->buffer;
                bds[ j ].offset = descriptor_write.descriptor_buffer_descs[ j ].offset;
                bds[ j ].range  = descriptor_write.descriptor_buffer_descs[ j ].range;
            }

            write_descriptor_set.pBufferInfo = bds.data();
        }
        else
        {
            auto& ids = image_updates.emplace_back();
            ids.resize(descriptor_write.descriptor_count);

            for (u32 j = 0; j < descriptor_write.descriptor_count; ++j)
            {
                auto& image_write = descriptor_write.descriptor_image_descs[ j ];
                ids[ j ]          = {};

                if (image_write.image)
                {
                    ids[ j ].imageLayout = determine_image_layout(image_write.resource_state);
                    ids[ j ].imageView   = image_write.image->image_view;
                    ids[ j ].sampler     = image_write.sampler ? image_write.sampler->sampler : nullptr;
                }
                else
                {
                    ids[ j ].sampler = image_write.sampler->sampler;
                }
            }

            write_descriptor_set.pImageInfo = ids.data();
        }
    }

    vkUpdateDescriptorSets(device->logical_device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
}

void create_ui_context(const UiDesc* desc, UiContext** p_context)
{
    FT_ASSERT(p_context);

    *p_context         = new (std::nothrow) UiContext{};
    UiContext* context = *p_context;

    VkDescriptorPoolSize pool_sizes[] = { { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
                                          { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
                                          { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
                                          { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
                                          { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
                                          { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
                                          { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
                                          { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
                                          { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
                                          { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
                                          { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 } };

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType                      = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags                      = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets                    = 1000;
    pool_info.poolSizeCount              = std::size(pool_sizes);
    pool_info.pPoolSizes                 = pool_sizes;

    VK_ASSERT(vkCreateDescriptorPool(desc->device->logical_device, &pool_info, nullptr, &context->desriptor_pool));

    ImGui::CreateContext();
    auto& io = ImGui::GetIO();
    ( void ) io;
    if (desc->docking)
    {
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    }

    if (desc->viewports)
    {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }

    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance        = desc->renderer->instance;
    init_info.PhysicalDevice  = desc->device->physical_device;
    init_info.Device          = desc->device->logical_device;
    init_info.QueueFamily     = desc->queue->family_index;
    init_info.Queue           = desc->queue->queue;
    init_info.PipelineCache   = VkPipelineCache{};
    init_info.Allocator       = desc->device->vulkan_allocator;
    init_info.MinImageCount   = desc->min_image_count;
    init_info.ImageCount      = desc->image_count;
    init_info.InFlyFrameCount = desc->in_fly_frame_count;
    init_info.CheckVkResultFn = nullptr;
    init_info.MSAASamples     = VK_SAMPLE_COUNT_1_BIT;

    ImGui_ImplSDL2_InitForVulkan(( SDL_Window* ) desc->window->handle);
    ImGui_ImplVulkan_Init(&init_info, desc->render_pass->render_pass);

    begin_command_buffer(desc->device->cmd);
    ImGui_ImplVulkan_CreateFontsTexture(desc->device->cmd->command_buffer);
    end_command_buffer(desc->device->cmd);
    QueueSubmitDesc queue_submit_desc{};
    queue_submit_desc.command_buffer_count = 1;
    queue_submit_desc.command_buffers      = desc->device->cmd;
    queue_submit(desc->device->queue, &queue_submit_desc);
    queue_wait_idle(desc->device->queue);

    ImGui_ImplVulkan_DestroyFontUploadObjects();
}

void destroy_ui_context(const Device* device, const UiContext* context)
{
    FT_ASSERT(context);
    vkDestroyDescriptorPool(device->logical_device, context->desriptor_pool, nullptr);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    delete context;
}

void ui_begin_frame()
{
    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
}

void ui_end_frame(const CommandBuffer* cmd)
{
    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd->command_buffer);

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
    }
}

} // namespace fluent