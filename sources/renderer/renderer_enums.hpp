#pragma once

#include "volk.h"
#include "tinyimageformat_base.h"

namespace fluent
{

#define BIT(x) 1 << x

enum class QueueType : u8
{
    eGraphics = 0,
    eCompute  = 1,
    eTransfer = 2,
    eLast
};

enum class Format
{
    eUndefined           = TinyImageFormat_UNDEFINED,
    eR1Unorm             = TinyImageFormat_R1_UNORM,
    eR2Unorm             = TinyImageFormat_R2_UNORM,
    eR4Unorm             = TinyImageFormat_R4_UNORM,
    eR4G4Unorm           = TinyImageFormat_R4G4_UNORM,
    eG4R4Unorm           = TinyImageFormat_G4R4_UNORM,
    eA8Unorm             = TinyImageFormat_A8_UNORM,
    eR8Unorm             = TinyImageFormat_R8_UNORM,
    eR8Snorm             = TinyImageFormat_R8_SNORM,
    eR8Uint              = TinyImageFormat_R8_UINT,
    eR8Sint              = TinyImageFormat_R8_SINT,
    eR8Srgb              = TinyImageFormat_R8_SRGB,
    eB2G3R3Unorm         = TinyImageFormat_B2G3R3_UNORM,
    eR4G4B4A4Unorm       = TinyImageFormat_R4G4B4A4_UNORM,
    eR4G4B4X4Unorm       = TinyImageFormat_R4G4B4X4_UNORM,
    eB4G4R4A4Unorm       = TinyImageFormat_B4G4R4A4_UNORM,
    eB4G4R4X4Unorm       = TinyImageFormat_B4G4R4X4_UNORM,
    eA4R4G4B4Unorm       = TinyImageFormat_A4R4G4B4_UNORM,
    eX4R4G4B4Unorm       = TinyImageFormat_X4R4G4B4_UNORM,
    eA4B4G4R4Unorm       = TinyImageFormat_A4B4G4R4_UNORM,
    eX4B4G4R4Unorm       = TinyImageFormat_X4B4G4R4_UNORM,
    eR5G6B5Unorm         = TinyImageFormat_R5G6B5_UNORM,
    eB5G6R5Unorm         = TinyImageFormat_B5G6R5_UNORM,
    eR5G5B5A1Unorm       = TinyImageFormat_R5G5B5A1_UNORM,
    eB5G5R5A1Unorm       = TinyImageFormat_B5G5R5A1_UNORM,
    eA1B5G5R5Unorm       = TinyImageFormat_A1B5G5R5_UNORM,
    eA1R5G5B5Unorm       = TinyImageFormat_A1R5G5B5_UNORM,
    eB5G5B5X1Unorm       = TinyImageFormat_R5G5B5X1_UNORM,
    eB5G5R5X1Unorm       = TinyImageFormat_B5G5R5X1_UNORM,
    eX1R5G5B5Unorm       = TinyImageFormat_X1R5G5B5_UNORM,
    eX1B5G5R5Unorm       = TinyImageFormat_X1B5G5R5_UNORM,
    eB2G3R3A8Unorm       = TinyImageFormat_B2G3R3A8_UNORM,
    eR8G8Unorm           = TinyImageFormat_R8G8_UNORM,
    eR8G8Snorm           = TinyImageFormat_R8G8_SNORM,
    eG8R8Unorm           = TinyImageFormat_G8R8_UNORM,
    eG8R8Snorm           = TinyImageFormat_G8R8_SNORM,
    eR8G8Uint            = TinyImageFormat_R8G8_UINT,
    eR8G8Sint            = TinyImageFormat_R8G8_SINT,
    eR8G8Srgb            = TinyImageFormat_R8G8_SRGB,
    eR16Unorm            = TinyImageFormat_R16_UNORM,
    eR16Snorm            = TinyImageFormat_R16_SNORM,
    eR16Uint             = TinyImageFormat_R16_UINT,
    eR16Sint             = TinyImageFormat_R16_SINT,
    eR16Sfloat           = TinyImageFormat_R16_SFLOAT,
    eR16Sbfloat          = TinyImageFormat_R16_SBFLOAT,
    eR8G8B8Unorm         = TinyImageFormat_R8G8B8_UNORM,
    eR8G8B8Snorm         = TinyImageFormat_R8G8B8_SNORM,
    eR8G8B8Uint          = TinyImageFormat_R8G8B8_UINT,
    eR8G8B8Sint          = TinyImageFormat_R8G8B8_SINT,
    eR8G8B8Srgb          = TinyImageFormat_R8G8B8_SRGB,
    eB8G8R8Unorm         = TinyImageFormat_B8G8R8_UNORM,
    eB8G8R8Snorm         = TinyImageFormat_B8G8R8_SNORM,
    eB8G8R8Uint          = TinyImageFormat_B8G8R8_UINT,
    eB8G8R8Sint          = TinyImageFormat_B8G8R8_SINT,
    eB8G8R8Srgb          = TinyImageFormat_B8G8R8_SRGB,
    eR8G8B8A8Unorm       = TinyImageFormat_R8G8B8A8_UNORM,
    eR8G8B8A8Snorm       = TinyImageFormat_R8G8B8A8_SNORM,
    eR8G8B8A8Uint        = TinyImageFormat_R8G8B8A8_UINT,
    eR8G8B8A8Sint        = TinyImageFormat_R8G8B8A8_SINT,
    eR8G8B8A8Srgb        = TinyImageFormat_R8G8B8A8_SRGB,
    eB8G8R8A8Unorm       = TinyImageFormat_B8G8R8A8_UNORM,
    eB8G8R8A8Snorm       = TinyImageFormat_B8G8R8A8_SNORM,
    eB8G8R8A8Uint        = TinyImageFormat_B8G8R8A8_UINT,
    eB8G8R8A8Sint        = TinyImageFormat_B8G8R8A8_SINT,
    eB8G8R8A8Srgb        = TinyImageFormat_B8G8R8A8_SRGB,
    eR8G8B8X8Unorm       = TinyImageFormat_R8G8B8X8_UNORM,
    eB8G8R8X8Unorm       = TinyImageFormat_B8G8R8X8_UNORM,
    eR16G16Unorm         = TinyImageFormat_R16G16_UNORM,
    eG16R16Unorm         = TinyImageFormat_G16R16_UNORM,
    eR16G16Snorm         = TinyImageFormat_R16G16_SNORM,
    eG16R16Snorm         = TinyImageFormat_G16R16_SNORM,
    eR16G16Uint          = TinyImageFormat_R16G16_UINT,
    eR16G16Sint          = TinyImageFormat_R16G16_SINT,
    eR16G16Sfloat        = TinyImageFormat_R16G16_SFLOAT,
    eR16G16Sbfloat       = TinyImageFormat_R16G16_SBFLOAT,
    eR32Uint             = TinyImageFormat_R32_UINT,
    eR32Sint             = TinyImageFormat_R32_SINT,
    eR32Sfloat           = TinyImageFormat_R32_SFLOAT,
    eA2R10G10B10Unorm    = TinyImageFormat_A2R10G10B10_UNORM,
    eA2R10G10B10Uint     = TinyImageFormat_A2R10G10B10_UINT,
    eA2R10G10B10Snorm    = TinyImageFormat_A2R10G10B10_SNORM,
    eA2R10G10B10Sint     = TinyImageFormat_A2R10G10B10_SINT,
    eA2B10G10R10Unorm    = TinyImageFormat_A2B10G10R10_UNORM,
    eA2B10G10R10Uint     = TinyImageFormat_A2B10G10R10_UINT,
    eA2B10G10R10Snorm    = TinyImageFormat_A2B10G10R10_SNORM,
    eA2B10G10R10Sint     = TinyImageFormat_A2B10G10R10_SINT,
    eR10G10B10A2Unorm    = TinyImageFormat_R10G10B10A2_UNORM,
    eR10G10B10A2Uint     = TinyImageFormat_R10G10B10A2_UINT,
    eR10G10B10A2Snorm    = TinyImageFormat_R10G10B10A2_SNORM,
    eR10G10B10A2Sint     = TinyImageFormat_R10G10B10A2_SINT,
    eB10G10R10R10A2Unorm = TinyImageFormat_B10G10R10A2_UNORM,
    eB10G10R10R10A2Uint  = TinyImageFormat_B10G10R10A2_UINT,
    eB10G10R10R10A2Snorm = TinyImageFormat_B10G10R10A2_SNORM,
    eB10G10R10R10A2Sint  = TinyImageFormat_B10G10R10A2_SINT,
    eB10G10R11Ufloat     = TinyImageFormat_B10G11R11_UFLOAT,
    eE5B5G9R9Ufloat      = TinyImageFormat_E5B9G9R9_UFLOAT,
    eR16G16B16Unorm      = TinyImageFormat_R16G16B16_UNORM,
    eR16G16B16Snorm      = TinyImageFormat_R16G16B16_SNORM,
    eR16G16B16Uint       = TinyImageFormat_R16G16B16_UINT,
    eR16G16B16Sint       = TinyImageFormat_R16G16B16_SINT,
    eR16G16B16Sfloat     = TinyImageFormat_R16G16B16_SFLOAT,
    eR16G16B16Sbfloat    = TinyImageFormat_R16G16B16_SBFLOAT,
    eR16G16B16A16Unorm   = TinyImageFormat_R16G16B16A16_UNORM,
    eR16G16B16A16Snorm   = TinyImageFormat_R16G16B16A16_SNORM,
    eR16G16B16A16Uint    = TinyImageFormat_R16G16B16A16_UINT,
    eR16G16B16A16Sint    = TinyImageFormat_R16G16B16A16_SINT,
    eR16G16B16A16Sfloat  = TinyImageFormat_R16G16B16A16_SFLOAT,
    eR16G16B16A16Sbfloat = TinyImageFormat_R16G16B16A16_SBFLOAT,
    eR32G32Uint          = TinyImageFormat_R32G32_UINT,
    eR32G32Sint          = TinyImageFormat_R32G32_SINT,
    eR32G32Sfloat        = TinyImageFormat_R32G32_SFLOAT,
    eR32G32B32Uint       = TinyImageFormat_R32G32B32_UINT,
    eR32G32B32Sint       = TinyImageFormat_R32G32B32_SINT,
    eR32G32B32Sfloat     = TinyImageFormat_R32G32B32_SFLOAT,
    eR32G32B32A32Uint    = TinyImageFormat_R32G32B32A32_UINT,
    eR32G32B32A32Sint    = TinyImageFormat_R32G32B32A32_SINT,
    eR32G32B32A32Sfloat  = TinyImageFormat_R32G32B32A32_SFLOAT,
    eR64Uint             = TinyImageFormat_R64_UINT,
    eR64Sint             = TinyImageFormat_R64_SINT,
    eR64Sfloat           = TinyImageFormat_R64_SFLOAT,
    eR64G64Uint          = TinyImageFormat_R64G64_UINT,
    eR64G64Sint          = TinyImageFormat_R64G64_SINT,
    eR64G64Sfloat        = TinyImageFormat_R64G64_SFLOAT,
    eR64G64B64Uint       = TinyImageFormat_R64G64B64_UINT,
    eR64G64B64Sint       = TinyImageFormat_R64G64B64_SINT,
    eR64G64B64Sfloat     = TinyImageFormat_R64G64B64_SFLOAT,
    eR64G64B64A64Uint    = TinyImageFormat_R64G64B64A64_UINT,
    eR64G64B64A64Sint    = TinyImageFormat_R64G64B64A64_SINT,
    eR64G64B64A64Sfloat  = TinyImageFormat_R64G64B64A64_SFLOAT,
    eD16Unorm            = TinyImageFormat_D16_UNORM,
    eX8D24Unorm          = TinyImageFormat_X8_D24_UNORM,
    eD32Sfloat           = TinyImageFormat_D32_SFLOAT,
    eS8Uint              = TinyImageFormat_S8_UINT,
    eD16UnormS8Uint      = TinyImageFormat_D16_UNORM_S8_UINT,
    eD24UnormS8Uint      = TinyImageFormat_D24_UNORM_S8_UINT,
    eD32SfloatS8Uint     = TinyImageFormat_D32_SFLOAT_S8_UINT,
    eDXBC1RgbUnorm       = TinyImageFormat_DXBC1_RGB_UNORM,
    eDXBC1RgbSrgb        = TinyImageFormat_DXBC1_RGB_SRGB,
    eDXBC1RgbaUnorm      = TinyImageFormat_DXBC1_RGBA_UNORM,
    eDXBC1RgbaSrgb       = TinyImageFormat_DXBC1_RGBA_SRGB,
    eDXBC2Unorm          = TinyImageFormat_DXBC2_UNORM,
    eDXBC2Srgb           = TinyImageFormat_DXBC2_SRGB,
    eDXBC3Unorm          = TinyImageFormat_DXBC3_UNORM,
    eDXBC3Srgb           = TinyImageFormat_DXBC3_SRGB,
    eDXBC4Unorm          = TinyImageFormat_DXBC4_UNORM,
    eDXBC4Snorm          = TinyImageFormat_DXBC4_SNORM,
    eDXBC5Unorm          = TinyImageFormat_DXBC5_UNORM,
    eDXBC5Snorm          = TinyImageFormat_DXBC5_SNORM,
    eDXBC6HUfloat        = TinyImageFormat_DXBC6H_UFLOAT,
    eDXBC6HSfloat        = TinyImageFormat_DXBC6H_SFLOAT,
    eDXBC7Unorm          = TinyImageFormat_DXBC7_UNORM,
    eDXBC7Srgb           = TinyImageFormat_DXBC7_SRGB,
    ePvrtc12BppUnorm     = TinyImageFormat_PVRTC1_2BPP_UNORM,
    ePvrtc14BppUnorm     = TinyImageFormat_PVRTC1_4BPP_UNORM,
    ePvrtc22BppUnorm     = TinyImageFormat_PVRTC2_2BPP_UNORM,
    ePvrtc24BppUnorm     = TinyImageFormat_PVRTC2_4BPP_UNORM,
    ePvrtc12BppSrgb      = TinyImageFormat_PVRTC1_2BPP_SRGB,
    ePvrtc14BppSrgb      = TinyImageFormat_PVRTC1_4BPP_SRGB,
    ePvrtc22BppSrgb      = TinyImageFormat_PVRTC2_2BPP_SRGB,
    ePvrtc24BppSrgb      = TinyImageFormat_PVRTC2_4BPP_SRGB,
    eEtc2R8G8B8Unorm     = TinyImageFormat_ETC2_R8G8B8_UNORM,
    eEtc2R8G8B8Srgb      = TinyImageFormat_ETC2_R8G8B8_SRGB,
    eEtc2R8G8B8A1Unorm   = TinyImageFormat_ETC2_R8G8B8A1_UNORM,
    eEtc2R8G8B8A1Srgb    = TinyImageFormat_ETC2_R8G8B8A1_SRGB,
    eEtc2R8G8B8A8Unorm   = TinyImageFormat_ETC2_R8G8B8A8_UNORM,
    eEtc2R8G8B8A8Srgb    = TinyImageFormat_ETC2_R8G8B8A8_SRGB,
    eEtc2EacR11Unorm     = TinyImageFormat_ETC2_EAC_R11_UNORM,
    eEtc2EacR11Snorm     = TinyImageFormat_ETC2_EAC_R11_SNORM,
    eEtc2EacR11G11Unorm  = TinyImageFormat_ETC2_EAC_R11G11_UNORM,
    eEtc2EacR11G11Snorm  = TinyImageFormat_ETC2_EAC_R11G11_SNORM,
    eAstc4x4Unorm        = TinyImageFormat_ASTC_4x4_UNORM,
    eAstc4x4Srgb         = TinyImageFormat_ASTC_4x4_SRGB,
    eAstc5x4Unorm        = TinyImageFormat_ASTC_5x4_UNORM,
    eAstc5x4Srgb         = TinyImageFormat_ASTC_5x4_SRGB,
    eAstc5x5Unorm        = TinyImageFormat_ASTC_5x5_UNORM,
    eAstc5x5Srgb         = TinyImageFormat_ASTC_5x5_SRGB,
    eAstc6x5Unorm        = TinyImageFormat_ASTC_6x5_UNORM,
    eAstc6x5Srgb         = TinyImageFormat_ASTC_6x5_SRGB,
    eAstc6x6Unorm        = TinyImageFormat_ASTC_6x6_UNORM,
    eAstc6x6Srgb         = TinyImageFormat_ASTC_6x6_SRGB,
    eAstc8x5Unorm        = TinyImageFormat_ASTC_8x5_UNORM,
    eAstc8x5Srgb         = TinyImageFormat_ASTC_8x5_SRGB,
    eAstc8x6Unorm        = TinyImageFormat_ASTC_8x6_UNORM,
    eAstc8x6Srgb         = TinyImageFormat_ASTC_8x6_SRGB,
    eAstc8x8Unorm        = TinyImageFormat_ASTC_8x8_UNORM,
    eAstc8x8Srgb         = TinyImageFormat_ASTC_8x8_SRGB,
    eAstc10x5Unorm       = TinyImageFormat_ASTC_10x5_UNORM,
    eAstc10x5Srgb        = TinyImageFormat_ASTC_10x5_SRGB,
    eAstc10x6Unorm       = TinyImageFormat_ASTC_10x6_UNORM,
    eAstc10x6Srgb        = TinyImageFormat_ASTC_10x6_SRGB,
    eAstc10x8Unorm       = TinyImageFormat_ASTC_10x8_UNORM,
    eAstc10x8Srgb        = TinyImageFormat_ASTC_10x8_SRGB,
    eAstc10x10Unorm      = TinyImageFormat_ASTC_10x10_UNORM,
    eAstc10x10Srgb       = TinyImageFormat_ASTC_10x10_SRGB,
    eAstc12x10Unorm      = TinyImageFormat_ASTC_12x10_UNORM,
    eAstc12x10Srgb       = TinyImageFormat_ASTC_12x10_SRGB,
    eAstc12x12Unorm      = TinyImageFormat_ASTC_12x12_UNORM,
    eAstc12x12Srgb       = TinyImageFormat_ASTC_12x12_SRGB,
    eClutP4              = TinyImageFormat_CLUT_P4,
    eClutP4A4            = TinyImageFormat_CLUT_P4A4,
    eClutP8              = TinyImageFormat_CLUT_P8,
    eClutP8A8            = TinyImageFormat_CLUT_P8A8,
};

enum class SampleCount
{
    e1,
    e2,
    e4,
    e8,
    e16,
    e32,
    e64
};

enum class AttachmentLoadOp
{
    eLoad,
    eClear    = 1,
    eDontCare = 2
};

enum class ResourceState : u32
{
    eUndefined            = BIT(0),
    eGeneral              = BIT(1),
    eColorAttachment      = BIT(2),
    eDepthStencilWrite    = BIT(3),
    eDepthStencilReadOnly = BIT(4),
    eShaderReadOnly       = BIT(5),
    eTransferSrc          = BIT(6),
    eTransferDst          = BIT(7),
    ePresent              = BIT(8)
};
MAKE_ENUM_FLAG(u32, ResourceState);

enum class DescriptorType : u32
{
    eUndefined              = BIT(0),
    eVertexBuffer           = BIT(1),
    eIndexBuffer            = BIT(2),
    eUniformBuffer          = BIT(3),
    eSampler                = BIT(4),
    eCombinedImageSampler   = BIT(5),
    eSampledImage           = BIT(6),
    eStorageImage           = BIT(7),
    eUniformTexelBuffer     = BIT(8),
    eStorageTexelBuffer     = BIT(9),
    eStorageBuffer          = BIT(10),
    eUniformBufferDynamic   = BIT(11),
    eStorageBufferDynamic   = BIT(12),
    eInputAttachment        = BIT(13),
    eDepthStencilAttachment = BIT(14),
    eColorAttachment        = BIT(15),
    eHostVisibleBuffer      = BIT(16),
    eDeviceLocalBuffer      = BIT(17)
};
MAKE_ENUM_FLAG(u32, DescriptorType);

enum class PipelineType
{
    eCompute,
    eGraphics
};

enum class VertexInputRate
{
    eVertex,
    eInstance
};

enum class FrontFace
{
    eClockwise,
    eCounterClockwise
};

enum class PolygonMode
{
    eFill,
    eLine
};

enum class SamplerMipmapMode
{
    eNearest,
    eLinear
};

enum class SamplerAddressMode
{
    eRepeat,
    eMirroredRepeat,
    eClampToEdge,
    eClampToBorder,
    eMirrorClampToEdge,
};

enum class CompareOp
{
    eNever,
    eLess,
    eEqual,
    eLessOrEqual,
    eGreater,
    eNotEqual,
    eGreaterOrEqual,
    eAlways
};

enum class CullMode
{
    eNone,
    eFront,
    eBack
};

enum class ShaderStage
{
    eVertex,
    eTessellationControl,
    eTessellationEvaluation,
    eGeometry,
    eFragment,
    eCompute,
    eAllGraphics,
    eAll
};

enum class Filter
{
    eNearest = 0,
    eLinear  = 1,
};

} // namespace fluent
