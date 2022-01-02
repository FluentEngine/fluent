#include "tinyimageformat_apis.h"
#include "renderer/renderer_enums.hpp"

namespace fluent
{
VkFormat util_to_vk_format(Format format)
{
    return static_cast<VkFormat>(TinyImageFormat_ToVkFormat(static_cast<TinyImageFormat>(format)));
}

Format util_from_vk_format(VkFormat format)
{
    return static_cast<Format>(TinyImageFormat_FromVkFormat(static_cast<TinyImageFormat_VkFormat>(format)));
}

VkSampleCountFlagBits util_to_vk_sample_count(SampleCount sample_count)
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
        break;
    }

    return VkSampleCountFlagBits(-1);
}

VkAttachmentLoadOp util_to_vk_load_op(AttachmentLoadOp load_op)
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
        break;
    }
    return VkAttachmentLoadOp(-1);
}
} // namespace fluent