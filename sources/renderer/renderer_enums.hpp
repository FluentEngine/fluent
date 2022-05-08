#pragma once

#include <tinyimageformat_base.h>
#include "core/base.hpp"

namespace fluent
{
#define BIT( x ) 1 << x

enum class RendererAPI : u8
{
	VULKAN = 0,
	D3D12  = 1,
	METAL  = 2,
};

enum class QueueType : u8
{
	GRAPHICS = 0,
	COMPUTE  = 1,
	TRANSFER = 2,
	COUNT
};

enum class Format
{
	UNDEFINED             = TinyImageFormat_UNDEFINED,
	R1_UNORM              = TinyImageFormat_R1_UNORM,
	R2_UNORM              = TinyImageFormat_R2_UNORM,
	R4_UNORM              = TinyImageFormat_R4_UNORM,
	R4G4_UNORM            = TinyImageFormat_R4G4_UNORM,
	G4R4_UNORM            = TinyImageFormat_G4R4_UNORM,
	A8_UNORM              = TinyImageFormat_A8_UNORM,
	R8_UNORM              = TinyImageFormat_R8_UNORM,
	eR8_SNORM             = TinyImageFormat_R8_SNORM,
	R8_UINT               = TinyImageFormat_R8_UINT,
	R8_SINT               = TinyImageFormat_R8_SINT,
	R8_SRGB               = TinyImageFormat_R8_SRGB,
	B2G3R3_UNORM          = TinyImageFormat_B2G3R3_UNORM,
	R4G4B4A4_UNORM        = TinyImageFormat_R4G4B4A4_UNORM,
	R4G4B4X4_UNORM        = TinyImageFormat_R4G4B4X4_UNORM,
	B4G4R4A4_UNORM        = TinyImageFormat_B4G4R4A4_UNORM,
	B4G4R4X4_UNORM        = TinyImageFormat_B4G4R4X4_UNORM,
	A4R4G4B4_UNORM        = TinyImageFormat_A4R4G4B4_UNORM,
	X4R4G4B4_UNORM        = TinyImageFormat_X4R4G4B4_UNORM,
	A4B4G4R4_UNORM        = TinyImageFormat_A4B4G4R4_UNORM,
	X4B4G4R4_UNORM        = TinyImageFormat_X4B4G4R4_UNORM,
	R5G6B5_UNORM          = TinyImageFormat_R5G6B5_UNORM,
	B5G6R5_UNORM          = TinyImageFormat_B5G6R5_UNORM,
	R5G5B5A1_UNORM        = TinyImageFormat_R5G5B5A1_UNORM,
	B5G5R5A1_UNORM        = TinyImageFormat_B5G5R5A1_UNORM,
	A1B5G5R5_UNORM        = TinyImageFormat_A1B5G5R5_UNORM,
	A1R5G5B5_UNORM        = TinyImageFormat_A1R5G5B5_UNORM,
	B5G5B5X1_UNORM        = TinyImageFormat_R5G5B5X1_UNORM,
	B5G5R5X1_UNORM        = TinyImageFormat_B5G5R5X1_UNORM,
	X1R5G5B5_UNORM        = TinyImageFormat_X1R5G5B5_UNORM,
	X1B5G5R5_UNORM        = TinyImageFormat_X1B5G5R5_UNORM,
	B2G3R3A8_UNORM        = TinyImageFormat_B2G3R3A8_UNORM,
	R8G8_UNORM            = TinyImageFormat_R8G8_UNORM,
	R8G8_SNORM            = TinyImageFormat_R8G8_SNORM,
	G8R8_UNORM            = TinyImageFormat_G8R8_UNORM,
	G8R8_SNORM            = TinyImageFormat_G8R8_SNORM,
	R8G8_UINT             = TinyImageFormat_R8G8_UINT,
	R8G8_SINT             = TinyImageFormat_R8G8_SINT,
	R8G8_SRGB             = TinyImageFormat_R8G8_SRGB,
	R16_UNORM             = TinyImageFormat_R16_UNORM,
	R16_SNORM             = TinyImageFormat_R16_SNORM,
	R16_UINT              = TinyImageFormat_R16_UINT,
	R16_SINT              = TinyImageFormat_R16_SINT,
	R16_SFLOAT            = TinyImageFormat_R16_SFLOAT,
	R16_SBFLOAT           = TinyImageFormat_R16_SBFLOAT,
	R8G8B8_UNORM          = TinyImageFormat_R8G8B8_UNORM,
	R8G8B8_SNORM          = TinyImageFormat_R8G8B8_SNORM,
	R8G8B8_UINT           = TinyImageFormat_R8G8B8_UINT,
	R8G8B8_SINT           = TinyImageFormat_R8G8B8_SINT,
	R8G8B8_SRGB           = TinyImageFormat_R8G8B8_SRGB,
	B8G8R8_UNORM          = TinyImageFormat_B8G8R8_UNORM,
	B8G8R8_SNORM          = TinyImageFormat_B8G8R8_SNORM,
	B8G8R8_UINT           = TinyImageFormat_B8G8R8_UINT,
	B8G8R8_SINT           = TinyImageFormat_B8G8R8_SINT,
	B8G8R8_SRGB           = TinyImageFormat_B8G8R8_SRGB,
	R8G8B8A8_UNORM        = TinyImageFormat_R8G8B8A8_UNORM,
	R8G8B8A8_SNORM        = TinyImageFormat_R8G8B8A8_SNORM,
	R8G8B8A8_UINT         = TinyImageFormat_R8G8B8A8_UINT,
	R8G8B8A8_SINT         = TinyImageFormat_R8G8B8A8_SINT,
	R8G8B8A8_SRGB         = TinyImageFormat_R8G8B8A8_SRGB,
	B8G8R8A8_UNORM        = TinyImageFormat_B8G8R8A8_UNORM,
	B8G8R8A8_SNORM        = TinyImageFormat_B8G8R8A8_SNORM,
	B8G8R8A8_UINT         = TinyImageFormat_B8G8R8A8_UINT,
	B8G8R8A8_SINT         = TinyImageFormat_B8G8R8A8_SINT,
	B8G8R8A8_SRGB         = TinyImageFormat_B8G8R8A8_SRGB,
	R8G8B8X8_UNORM        = TinyImageFormat_R8G8B8X8_UNORM,
	B8G8R8X8_UNORM        = TinyImageFormat_B8G8R8X8_UNORM,
	R16G16_UNORM          = TinyImageFormat_R16G16_UNORM,
	G16R16_UNORM          = TinyImageFormat_G16R16_UNORM,
	R16G16_SNORM          = TinyImageFormat_R16G16_SNORM,
	G16R16_SNORM          = TinyImageFormat_G16R16_SNORM,
	R16G16_UINT           = TinyImageFormat_R16G16_UINT,
	R16G16_SINT           = TinyImageFormat_R16G16_SINT,
	R16G16_SFLOAT         = TinyImageFormat_R16G16_SFLOAT,
	R16G16_SBFLOAT        = TinyImageFormat_R16G16_SBFLOAT,
	R32_UINT              = TinyImageFormat_R32_UINT,
	R32_SINT              = TinyImageFormat_R32_SINT,
	R32_SFLOAT            = TinyImageFormat_R32_SFLOAT,
	A2R10G10B10_UNORM     = TinyImageFormat_A2R10G10B10_UNORM,
	A2R10G10B10_UINT      = TinyImageFormat_A2R10G10B10_UINT,
	A2R10G10B10_SNORM     = TinyImageFormat_A2R10G10B10_SNORM,
	A2R10G10B10_SINT      = TinyImageFormat_A2R10G10B10_SINT,
	A2B10G10R10_UNORM     = TinyImageFormat_A2B10G10R10_UNORM,
	A2B10G10R10_UINT      = TinyImageFormat_A2B10G10R10_UINT,
	A2B10G10R10_SNORM     = TinyImageFormat_A2B10G10R10_SNORM,
	A2B10G10R10_SINT      = TinyImageFormat_A2B10G10R10_SINT,
	R10G10B10A2_UNORM     = TinyImageFormat_R10G10B10A2_UNORM,
	R10G10B10A2_UINT      = TinyImageFormat_R10G10B10A2_UINT,
	R10G10B10A2_SNORM     = TinyImageFormat_R10G10B10A2_SNORM,
	R10G10B10A2_SINT      = TinyImageFormat_R10G10B10A2_SINT,
	B10G10R10R10A2_UNORM  = TinyImageFormat_B10G10R10A2_UNORM,
	B10G10R10R10A2_UINT   = TinyImageFormat_B10G10R10A2_UINT,
	B10G10R10R10A2_SNORM  = TinyImageFormat_B10G10R10A2_SNORM,
	B10G10R10R10A2_SINT   = TinyImageFormat_B10G10R10A2_SINT,
	B10G10R11_UFLOAT      = TinyImageFormat_B10G11R11_UFLOAT,
	E5B5G9R9_UFLOAT       = TinyImageFormat_E5B9G9R9_UFLOAT,
	R16G16B16_UNORM       = TinyImageFormat_R16G16B16_UNORM,
	R16G16B16_SNORM       = TinyImageFormat_R16G16B16_SNORM,
	R16G16B16_UINT        = TinyImageFormat_R16G16B16_UINT,
	R16G16B16_SINT        = TinyImageFormat_R16G16B16_SINT,
	R16G16B16_SFLOAT      = TinyImageFormat_R16G16B16_SFLOAT,
	R16G16B16_SBFLOAT     = TinyImageFormat_R16G16B16_SBFLOAT,
	R16G16B16A16_UNORM    = TinyImageFormat_R16G16B16A16_UNORM,
	R16G16B16A16_SNORM    = TinyImageFormat_R16G16B16A16_SNORM,
	R16G16B16A16_UINT     = TinyImageFormat_R16G16B16A16_UINT,
	R16G16B16A16_SINT     = TinyImageFormat_R16G16B16A16_SINT,
	R16G16B16A16_SFLOAT   = TinyImageFormat_R16G16B16A16_SFLOAT,
	R16G16B16A16_SBFLOAT  = TinyImageFormat_R16G16B16A16_SBFLOAT,
	R32G32_UINT           = TinyImageFormat_R32G32_UINT,
	R32G32_SINT           = TinyImageFormat_R32G32_SINT,
	R32G32_SFLOAT         = TinyImageFormat_R32G32_SFLOAT,
	R32G32B32_UINT        = TinyImageFormat_R32G32B32_UINT,
	R32G32B32_SINT        = TinyImageFormat_R32G32B32_SINT,
	R32G32B32_SFLOAT      = TinyImageFormat_R32G32B32_SFLOAT,
	R32G32B32A32_UINT     = TinyImageFormat_R32G32B32A32_UINT,
	R32G32B32A32_SINT     = TinyImageFormat_R32G32B32A32_SINT,
	R32G32B32A32_SFLOAT   = TinyImageFormat_R32G32B32A32_SFLOAT,
	R64_UINT              = TinyImageFormat_R64_UINT,
	R64_SINT              = TinyImageFormat_R64_SINT,
	R64_SFLOAT            = TinyImageFormat_R64_SFLOAT,
	R64G64_UINT           = TinyImageFormat_R64G64_UINT,
	R64G64_SINT           = TinyImageFormat_R64G64_SINT,
	R64G64_SFLOAT         = TinyImageFormat_R64G64_SFLOAT,
	R64G64B64_UINT        = TinyImageFormat_R64G64B64_UINT,
	R64G64B64_SINT        = TinyImageFormat_R64G64B64_SINT,
	R64G64B64_SFLOAT      = TinyImageFormat_R64G64B64_SFLOAT,
	R64G64B64A64_UINT     = TinyImageFormat_R64G64B64A64_UINT,
	R64G64B64A64_SINT     = TinyImageFormat_R64G64B64A64_SINT,
	R64G64B64A64_SFLOAT   = TinyImageFormat_R64G64B64A64_SFLOAT,
	D16_UNORM             = TinyImageFormat_D16_UNORM,
	X8D24_UNORM           = TinyImageFormat_X8_D24_UNORM,
	D32_SFLOAT            = TinyImageFormat_D32_SFLOAT,
	S8_UINT               = TinyImageFormat_S8_UINT,
	D16_UNORMS8_UINT      = TinyImageFormat_D16_UNORM_S8_UINT,
	D24_UNORMS8_UINT      = TinyImageFormat_D24_UNORM_S8_UINT,
	D32_SFLOATS8_UINT     = TinyImageFormat_D32_SFLOAT_S8_UINT,
	DXBC1_RGB_UNORM       = TinyImageFormat_DXBC1_RGB_UNORM,
	DXBC1_RGB_SRGB        = TinyImageFormat_DXBC1_RGB_SRGB,
	DXBC1_RGBA_UNORM      = TinyImageFormat_DXBC1_RGBA_UNORM,
	DXBC1_RGBA_SRGB       = TinyImageFormat_DXBC1_RGBA_SRGB,
	DXBC2_UNORM           = TinyImageFormat_DXBC2_UNORM,
	DXBC2_SRGB            = TinyImageFormat_DXBC2_SRGB,
	DXBC3_UNORM           = TinyImageFormat_DXBC3_UNORM,
	DXBC3_SRGB            = TinyImageFormat_DXBC3_SRGB,
	DXBC4_UNORM           = TinyImageFormat_DXBC4_UNORM,
	DXBC4_SNORM           = TinyImageFormat_DXBC4_SNORM,
	DXBC5_UNORM           = TinyImageFormat_DXBC5_UNORM,
	DXBC5_SNORM           = TinyImageFormat_DXBC5_SNORM,
	DXBC6H_UFLOAT         = TinyImageFormat_DXBC6H_UFLOAT,
	DXBC6H_SFLOAT         = TinyImageFormat_DXBC6H_SFLOAT,
	DXBC7_UNORM           = TinyImageFormat_DXBC7_UNORM,
	DXBC7_SRGB            = TinyImageFormat_DXBC7_SRGB,
	PVRTC1_2BPP_UNORM     = TinyImageFormat_PVRTC1_2BPP_UNORM,
	PVRTC1_4BPP_UNORM     = TinyImageFormat_PVRTC1_4BPP_UNORM,
	PVRTC2_2BPP_UNORM     = TinyImageFormat_PVRTC2_2BPP_UNORM,
	PVRTC2_4BPP_UNORM     = TinyImageFormat_PVRTC2_4BPP_UNORM,
	PVRTC1_2BPP_SRGB      = TinyImageFormat_PVRTC1_2BPP_SRGB,
	PVRTC1_4BPP_SRGB      = TinyImageFormat_PVRTC1_4BPP_SRGB,
	PVRTC2_2BPP_SRGB      = TinyImageFormat_PVRTC2_2BPP_SRGB,
	PVRTC2_4BPP_SRGB      = TinyImageFormat_PVRTC2_4BPP_SRGB,
	ETC2_R8G8B8_UNORM     = TinyImageFormat_ETC2_R8G8B8_UNORM,
	ETC2_R8G8B8_SRGB      = TinyImageFormat_ETC2_R8G8B8_SRGB,
	ETC2_R8G8B8A1_UNORM   = TinyImageFormat_ETC2_R8G8B8A1_UNORM,
	ETC2_R8G8B8A1_SRGB    = TinyImageFormat_ETC2_R8G8B8A1_SRGB,
	ETC2_R8G8B8A8_UNORM   = TinyImageFormat_ETC2_R8G8B8A8_UNORM,
	ETC2_R8G8B8A8_SRGB    = TinyImageFormat_ETC2_R8G8B8A8_SRGB,
	ETC2_EAC_R11_UNORM    = TinyImageFormat_ETC2_EAC_R11_UNORM,
	ETC2_EAC_R11_SNORM    = TinyImageFormat_ETC2_EAC_R11_SNORM,
	ETC2_EAC_R11G11_UNORM = TinyImageFormat_ETC2_EAC_R11G11_UNORM,
	ETC2_EAC_R11G11_SNORM = TinyImageFormat_ETC2_EAC_R11G11_SNORM,
	ASTC_4x4_UNORM        = TinyImageFormat_ASTC_4x4_UNORM,
	ASTC_4x4_SRGB         = TinyImageFormat_ASTC_4x4_SRGB,
	ASTC_5x4_UNORM        = TinyImageFormat_ASTC_5x4_UNORM,
	ASTC_5x4_SRGB         = TinyImageFormat_ASTC_5x4_SRGB,
	ASTC_5x5_UNORM        = TinyImageFormat_ASTC_5x5_UNORM,
	ASTC_5x5_SRGB         = TinyImageFormat_ASTC_5x5_SRGB,
	ASTC_6x5_UNORM        = TinyImageFormat_ASTC_6x5_UNORM,
	ASTC_6x5_SRGB         = TinyImageFormat_ASTC_6x5_SRGB,
	ASTC_6x6_UNORM        = TinyImageFormat_ASTC_6x6_UNORM,
	ASTC_6x6_SRGB         = TinyImageFormat_ASTC_6x6_SRGB,
	ASTC_8x5_UNORM        = TinyImageFormat_ASTC_8x5_UNORM,
	ASTC_8x5_SRGB         = TinyImageFormat_ASTC_8x5_SRGB,
	ASTC_8x6_UNORM        = TinyImageFormat_ASTC_8x6_UNORM,
	ASTC_8x6_SRGB         = TinyImageFormat_ASTC_8x6_SRGB,
	ASTC_8x8_UNORM        = TinyImageFormat_ASTC_8x8_UNORM,
	ASTC_8x8_SRGB         = TinyImageFormat_ASTC_8x8_SRGB,
	ASTC_10x5_UNORM       = TinyImageFormat_ASTC_10x5_UNORM,
	ASTC_10x5_SRGB        = TinyImageFormat_ASTC_10x5_SRGB,
	ASTC_10x6_UNORM       = TinyImageFormat_ASTC_10x6_UNORM,
	ASTC_10x6_SRGB        = TinyImageFormat_ASTC_10x6_SRGB,
	ASTC_10x8_UNORM       = TinyImageFormat_ASTC_10x8_UNORM,
	ASTC_10x8_SRGB        = TinyImageFormat_ASTC_10x8_SRGB,
	ASTC_10x10_UNORM      = TinyImageFormat_ASTC_10x10_UNORM,
	ASTC_10x10_SRGB       = TinyImageFormat_ASTC_10x10_SRGB,
	ASTC_12x10_UNORM      = TinyImageFormat_ASTC_12x10_UNORM,
	ASTC_12x10_SRGB       = TinyImageFormat_ASTC_12x10_SRGB,
	ASTC_12x12_UNORM      = TinyImageFormat_ASTC_12x12_UNORM,
	ASTC_12x12_SRGB       = TinyImageFormat_ASTC_12x12_SRGB,
	CLUT_P4               = TinyImageFormat_CLUT_P4,
	CLUT_P4A4             = TinyImageFormat_CLUT_P4A4,
	CLUT_P8               = TinyImageFormat_CLUT_P8,
	CLUT_P8A8             = TinyImageFormat_CLUT_P8A8,
};

enum class SampleCount
{
	E1,
	E2,
	E4,
	E8,
	E16,
	E32,
	E64
};

enum class AttachmentLoadOp
{
	LOAD,
	CLEAR     = 1,
	DONT_CARE = 2
};

enum class MemoryUsage
{
	GPU_ONLY   = 0,
	CPU_ONLY   = 1,
	CPU_TO_GPU = 2,
	GPU_TO_CPU = 3,
	CPU_COPY   = 4,
};

enum class ResourceState : u32
{
	UNDEFINED               = BIT( 0 ),
	GENERAL                 = BIT( 1 ),
	COLOR_ATTACHMENT        = BIT( 2 ),
	DEPTH_STENCIL_WRITE     = BIT( 3 ),
	DEPTH_STENCIL_READ_ONLY = BIT( 4 ),
	SHADER_READ_ONLY        = BIT( 5 ),
	TRANSFER_SRC            = BIT( 6 ),
	TRANSFER_DST            = BIT( 7 ),
	PRESENT                 = BIT( 8 )
};
MAKE_ENUM_FLAG( u32, ResourceState );

enum class DescriptorType : u32
{
	UNDEFINED                = BIT( 0 ),
	VERTEX_BUFFER            = BIT( 1 ),
	INDEX_BUFFER             = BIT( 2 ),
	UNIFORM_BUFFER           = BIT( 3 ),
	SAMPLER                  = BIT( 4 ),
	SAMPLED_IMAGE            = BIT( 5 ),
	STORAGE_IMAGE            = BIT( 6 ),
	UNIFORM_TEXEL_BUFFER     = BIT( 7 ),
	STORAGE_TEXEL_BUFFER     = BIT( 8 ),
	STORAGE_BUFFER           = BIT( 9 ),
	INDIRECT_BUFFER          = BIT( 10 ),
	UNIFORM_BUFFER_DYNAMIC   = BIT( 11 ),
	STORAGE_BUFFER_DYNAMIC   = BIT( 12 ),
	INPUT_ATTACHMENT         = BIT( 13 ),
	DEPTH_STENCIL_ATTACHMENT = BIT( 14 ),
	COLOR_ATTACHMENT         = BIT( 15 ),
	TRANSIENT_ATTACHMENT     = BIT( 16 )
};
MAKE_ENUM_FLAG( u32, DescriptorType );

enum class PipelineType
{
	COMPUTE,
	GRAPHICS
};

enum class VertexInputRate
{
	VERTEX,
	INSTANCE
};

enum class FrontFace
{
	CLOCKWISE,
	COUNTER_CLOCKWISE
};

enum class PolygonMode
{
	FILL,
	LINE
};

enum class SamplerMipmapMode
{
	NEAREST,
	LINEAR
};

enum class SamplerAddressMode
{
	REPEAT,
	MIRRORED_REPEAT,
	CLAMP_TO_EDGE,
	CLAMP_TO_BORDER
};

enum class CompareOp
{
	NEVER,
	LESS,
	EQUAL,
	LESS_OR_EQUAL,
	GREATER,
	NOT_EQUAL,
	GREATER_OR_EQUAL,
	ALWAYS
};

enum class CullMode
{
	NONE,
	FRONT,
	BACK
};

enum class ShaderStage
{
	VERTEX,
	TESSELLATION_CONTROL,
	TESSELLATION_EVALUATION,
	GEOMETRY,
	FRAGMENT,
	COMPUTE,
	COUNT
};

enum class Filter
{
	NEAREST = 0,
	LINEAR  = 1,
};

enum class PrimitiveTopology
{
	POINT_LIST,
	LINE_LIST,
	LINE_STRIP,
	TRIANGLE_LIST,
	TRIANGLE_STRIP
};

} // namespace fluent
