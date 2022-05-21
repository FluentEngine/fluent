// Auto generated by formatgen on Oct 22 2019
#pragma once
#if !defined( TINYIMAGEFORMAT_BASE_H_ ) &&                                     \
    !defined( TINYIMAGEFORMAT_IMAGEFORMAT_H )
#define TINYIMAGEFORMAT_BASE_H_ 1

/* TinyImageFormat is a library about the encodings of pixels typically
 * encountered in real time graphics.
 *
 * Like modern graphics API it is enumeration based but it also provides an API
 * for reasoning about that enumeration programmatically.
 *
 * Additionally it provide ways of accessing pixels encoded in the specified
 * format for most pixel formats. The hope is eventually to get decode to 100%
 * but not there yet (119 out of 193 currently).
 * This allows you to effectively read/write a large amount of image data.
 *
 * To assist with working with graphics APIs converters to and from Vulkan,
 * D3D12 and Metal. These are self contained and do not require the actual APIs.
 *
 * Available as either a single header or a version made of 6 headers split into
 * functional groups. Use one style or the other but they don't mix.
 *
 * These files are large due to unrolling and large switches. Functions are
 * inlined, allowing the compiler to eliminate and collapse when possible,
 * however particularly the Encode/Decode functions aren't near as fast as they
 * could be if heavily optimised for specific formats/layouts.
 *
 * Whilst not optimal due to number of formats it supports, it intends to be
 * fast enough that in many cases it will be fine.
 *
 * Internally every format has a descriptor packed into a 64 bit code word.
 * This code word is used to generate the header and it isn't used by the API
 * itself, as its been 'burnt' out by the code generator but it can be used at
 * runtime if desired and in future its intended to be used for exotic packed
 * formats that don't get there own enumeration value but can be expressed
 * via a descriptor.
 *
 * Where possible for C++ users functions are constexpr.
 *
 * It is MIT licensed and borrows/builds on code/specs including but not
 * limited to
 * Microsoft's Chris Walbourn DXTextureTool
 * Rygerous public domain Half to float (and vice versa) code
 * Khronos DFD specifications
 * Khronos Vulkan Specification
 * Microsoft D3D11/D3D12 Specifications
 * Apple Metal documentation
 * DDS Loaders from various sources (Humus, Confetti FX, DXTextureTool)
 *
 * TODO
 * ----
 * Test CLUT formats and add encode clause
 * Add shared component i.e. G8R8G8B8 4:2:x formats
 * Add Multi plane formats
 * Add compressed format decoders
 * Add Block copy (for updating compressed formats)
 * Add functions that work on descriptor codes directly
 * Add UFLOAT 6 and 7 bit support
 * Optional SIMD decode/encode functions
 * More tests
 *
 * Definitions
 * -----------
 *
 * Pixel
 * We define a pixel as upto 4 channels representing a single 'colour'
 * Its may not be addressable directly but as blocks of related pixels.
 * When decode/encoding pixels are presented to the API as 4 floats or doubles.
 *
 * Logical Channels (TinyImageFormat_LogicalChannel)
 * Logical channel are the usual way you would ask for a particular channel,
 * so asking about LC_Red while get you data for the red channel, however its
 * actually physically encoded in the data.
 *
 * Physical Channels (TinyImageFormat_PhysicalChannel)
 * Physical channels are the inverse of logical channels, that have no meaning
 * beyond the position in the data itself.
 *
 * Both Logical and Physical channels support returning constant 0 or 1
 *
 * Blocks
 * A block is the smallest addressable element this format refers to. Blocks
 * have up to 3 dimensions (though no format currently uses the 3rd).
 * Blocks are always at least byte aligned.
 * For single pixel formats this will be 1x1x1. For something like R1 it would
 * be 8x1x1 (8 single bits for 8 pixels).
 * For block compressed format a common value is 4x4x1.
 * A block for shared channels or very tightly packed this is how many pixels
 * are combined into one addressable unit.
 *
 * API
 * ---
 * The primary enumeration is simply TinyImageFormat, everything else supports
 * this enum.
 * All functions, enums etc. are prefixed with TinyImageFormat_, All functions
 * also take the format as first parameter. These are often removed in the api
 * docs to save space.
 *
 * Defines
 * -------
 * TinyImageFormat_Count - how many formats in total
 * TinyImageFormat_MaxPixelCountOfBlock - maximum number of pixels in a block
 * 									- for any format (for static decode buffer
 allocation)
 *
 * Enums
 * -----
 * TinyImageFormat - Count entries, one for each format supported
 * LogicalChannel - values for logical channel or constants
 * 						- LC_Red - Red channel is specified
 * 						- LC_Green - Green channel is specified
 * 						- LC_Blue - Blue channel is specified
 * 						- LC_Alpha - Alpha channel is specified
 * 						- LC_Depth - Depth channel is specified
 * 						- LC_Stencil - Stencil channel is specified
 * 						- LC_1 - constant 1 will be returned
 * 						- LC_0 - constant 0 will be return
 * PhysicalChannel - values for physical channel or constants
 * 						- PC_0 - Leftmost channel
 * 						- PC_1 - 2nd channel
 * 						- PC_2 - 3rd channel
 * 						- PC_3 - 4th channel
 * 						- PC_CONST_1 - constant 1 will be returned
 * 						- PC_CONST_0 - constant 0 will be return
 *
 * Structs
 * -------
 * TinyImageFormat_DecodeInput
 *   - pixel or pixelPlane0 - pixel data ptr or pixel data ptr for plane 0
 *   - lut or pixelPlane1 - Look Up Table ptr for CLUT formats or pixel plane 1
 *	 - pixelPlane2 to pixelPlane 9 - 7 more planes ptrs
 * TinyImageFormat_EncodeOutput
 *   - pixel or pixelPlane0 - pixel data ptr or pixel data ptr for plane 0
 *	 - pixelPlane2 to pixelPlane 9 - 8 more planes ptrs

 * Query Functions
 * -----------
 * Code - uint64_t with the internal descriptor code
 * Name  - Human C string with the name of this fmt
 * FromName - lookup the format given the name as a C String (fast)
 * IsDepthOnly - true if just a depth channel
 * IsStencilOnly - true if just a stencil channel
 * IsDepthAndStencil - if has both depth and stencil channel
 * IsCompressed - true if its a compressed format (aka block)
 * IsCLUT - true if data is index into a CLUT (Colour Look Up Table)
 * IsFloat - is the data in floating point
 * IsNormalised - return true if data will be within 0 to 1 or -1 to 1
 * IsSigned - does the data include negatives
 * IsSRGB - is the data encoded using sRGB non linear encoding
 * IsHomogenous - is the encoding the same for every channel
 * WidthOfBlock - How many pixels in the x dimension for a block
 * HeightOfBlock - How many pixels in the y dimension for a block
 * DepthOfBlock 	- How many pixels in the z dimension for a block
 * PixelCountOfBlock - How many pixels in total for a block
 * BitSizeOfBlock - How big in bits is a single block.
 * ChannelCount - How many channels are actually encoded
 *
 * Logical Channel Functions
 * -------------------------
 * ChannelBitWidth( logical channel ) - how wide in bits is the channel
 * Min( logical channel ) - minimum possible value for the channel
 * Max(  logical channel ) - maximum possible value for the channel
 * LogicalChannelToPhysical( logical channel )
 * 											- what physical channel is the
 logical channel stored in
 * 											- or constant 0 or 1 if its not
 physically stored
 *
 * Pixel Decoder Functions (X = F or D version, use F if possible as its faster)
 * -----------------------
 * CanDecodeLogicalPixelsX - Can DecodeLogicalPixelsX work with this format?
 * DecodeLogicalPixelsX( width in blocks, FetchInput, out pixels)
 * 		 - pixels should be a pointer to 4 * PixelCounfOfBlack float/doubles
 * 		 - does full decode and remapping into logical channels include
 constants.
 *     - Returned result can be used directly as RGBA floating point data
 *     - Input pointers are updated are used, so can be passed back in for
 *     - next set of pixel decoding if desired.
 *     - For CLUT formats in.pixel should be the packed pixel data and in.lut is
 *		 - the lookuptable in R8G8B8A8 format of 2^Pbits entries
 *     - For all others in.pixel should be the packed pixel data
 * Pixel Decoder Helper Functions
 * -----------------------
 * UFloat6AsUintToFloat - returns the value stored as a 6 bit UFloat
 * UFloat7AsUintToFloat - returns the value stored as a 7 bit UFloat
 * UFloat10AsUintToFloat - returns the value stored as a 10 bit UFloat
 * UFloat11AsUintToFloat - returns the value stored as a 11 bit UFloat
 * SharedE5B9G9R9UFloatToFloats - return the pixel stored in shared
 * 															- shared 5 bit
 exponent, 9 bit mantissa for RGB
 * HalfAsUintToFloat - returns the value stored as a 16 bit SFloat
 * BFloatAsUintToFloat - returns the value stored as a 16 bit BFloat
 * LookupSRGB - returns the value for an 8 bit sRGB encoded value
 *
 * Pixel Encoder Functions (X = F or D version, use F if possible as its faster)
 * -----------------------
 * CanEncodeLogicalPixelsX - Can EncodeLogicalPixelsX work with this format?
 * EncodeLogicalPixelsX( width in blocks, in pixels, PutOutput)
 * 		 - pixels should be a pointer to 4 * PixelCounfOfBlack float/doubles
 * 		 - does full encode and remapping into logical channels
 *     - Output pointers are updated are used, so can be passed back in for
 *     - next set of pixel encoding if desired.
 *     - out.pixel is where colour information should be stored
 *
 * Pixel Encoder Helper Functions
 * -----------------------
 * FloatToUFloat6AsUint - Encodes float into  a 6 bit UFloat
 * FloatToUFloat7AsUint - Encodes float into  a 7 bit UFloat
 * FloatToUFloat10AsUint - Encodes float into a 10 bit UFloat
 * FloatToUFloat11AsUint - Encodes float into  11 bit UFloat
 * FloatRGBToRGB9E5AsUint32 - Encodes a float RGB into RGB9E5
 * FloatToHalfAsUint - Encodes a float into a 16 bit SFloat
 * FloatToBFloatAsUint -  Encodes a float into a 16 bit BFloat
 * FloatToSRGB - encodes a float to sRGB assuming input is 0-1
 *
 * Physical Channel Functions (in general use the Logical Channels)
 * ------------------
 * ChannelBitWidthAtPhysical( phys channel ) - how wide in bits for this channel
 * MinAtPhysical( phys channel ) - min possible value for this channel
 * MaxAtPhysical( phys channel ) - max possible value for this channel
 * PhysicalChannelToLogical(phys channel)
 * 											- what logical channel does a
 physical channel map to.
 * 											- Or a constant 0 or 1
 *
 * Graphics API Functions
 * ------------------
 * FromVkFormat( VkFormat ) - converts from or UNDEFINED if not possible
 * ToVkFormat - converts to or VK_FORMAT_UNDEFINED if not possible
 * FromDXGI_FORMAT( DXGI_FORMAT) converts from or UNDEFINED if not possible
 * ToDXGI_FORMAT - converts to or DXGI_FORMAT_UNKNOWN if not possible
 * DXGI_FORMATToTypeless - returns the DXGI typeless format if possible
 * FromMetal( MTLPixelFormat ) - converts from or UNDEFINED if not possible
 * ToMetal - converts to or MTLPixelFormatInvalid if not possible
 *
 *
 */
typedef enum TinyImageFormat
{
	TinyImageFormat_UNDEFINED             = 0,
	TinyImageFormat_R1_UNORM              = 1,
	TinyImageFormat_R2_UNORM              = 2,
	TinyImageFormat_R4_UNORM              = 3,
	TinyImageFormat_R4G4_UNORM            = 4,
	TinyImageFormat_G4R4_UNORM            = 5,
	TinyImageFormat_A8_UNORM              = 6,
	TinyImageFormat_R8_UNORM              = 7,
	TinyImageFormat_R8_SNORM              = 8,
	TinyImageFormat_R8_UINT               = 9,
	TinyImageFormat_R8_SINT               = 10,
	TinyImageFormat_R8_SRGB               = 11,
	TinyImageFormat_B2G3R3_UNORM          = 12,
	TinyImageFormat_R4G4B4A4_UNORM        = 13,
	TinyImageFormat_R4G4B4X4_UNORM        = 14,
	TinyImageFormat_B4G4R4A4_UNORM        = 15,
	TinyImageFormat_B4G4R4X4_UNORM        = 16,
	TinyImageFormat_A4R4G4B4_UNORM        = 17,
	TinyImageFormat_X4R4G4B4_UNORM        = 18,
	TinyImageFormat_A4B4G4R4_UNORM        = 19,
	TinyImageFormat_X4B4G4R4_UNORM        = 20,
	TinyImageFormat_R5G6B5_UNORM          = 21,
	TinyImageFormat_B5G6R5_UNORM          = 22,
	TinyImageFormat_R5G5B5A1_UNORM        = 23,
	TinyImageFormat_B5G5R5A1_UNORM        = 24,
	TinyImageFormat_A1B5G5R5_UNORM        = 25,
	TinyImageFormat_A1R5G5B5_UNORM        = 26,
	TinyImageFormat_R5G5B5X1_UNORM        = 27,
	TinyImageFormat_B5G5R5X1_UNORM        = 28,
	TinyImageFormat_X1R5G5B5_UNORM        = 29,
	TinyImageFormat_X1B5G5R5_UNORM        = 30,
	TinyImageFormat_B2G3R3A8_UNORM        = 31,
	TinyImageFormat_R8G8_UNORM            = 32,
	TinyImageFormat_R8G8_SNORM            = 33,
	TinyImageFormat_G8R8_UNORM            = 34,
	TinyImageFormat_G8R8_SNORM            = 35,
	TinyImageFormat_R8G8_UINT             = 36,
	TinyImageFormat_R8G8_SINT             = 37,
	TinyImageFormat_R8G8_SRGB             = 38,
	TinyImageFormat_R16_UNORM             = 39,
	TinyImageFormat_R16_SNORM             = 40,
	TinyImageFormat_R16_UINT              = 41,
	TinyImageFormat_R16_SINT              = 42,
	TinyImageFormat_R16_SFLOAT            = 43,
	TinyImageFormat_R16_SBFLOAT           = 44,
	TinyImageFormat_R8G8B8_UNORM          = 45,
	TinyImageFormat_R8G8B8_SNORM          = 46,
	TinyImageFormat_R8G8B8_UINT           = 47,
	TinyImageFormat_R8G8B8_SINT           = 48,
	TinyImageFormat_R8G8B8_SRGB           = 49,
	TinyImageFormat_B8G8R8_UNORM          = 50,
	TinyImageFormat_B8G8R8_SNORM          = 51,
	TinyImageFormat_B8G8R8_UINT           = 52,
	TinyImageFormat_B8G8R8_SINT           = 53,
	TinyImageFormat_B8G8R8_SRGB           = 54,
	TinyImageFormat_R8G8B8A8_UNORM        = 55,
	TinyImageFormat_R8G8B8A8_SNORM        = 56,
	TinyImageFormat_R8G8B8A8_UINT         = 57,
	TinyImageFormat_R8G8B8A8_SINT         = 58,
	TinyImageFormat_R8G8B8A8_SRGB         = 59,
	TinyImageFormat_B8G8R8A8_UNORM        = 60,
	TinyImageFormat_B8G8R8A8_SNORM        = 61,
	TinyImageFormat_B8G8R8A8_UINT         = 62,
	TinyImageFormat_B8G8R8A8_SINT         = 63,
	TinyImageFormat_B8G8R8A8_SRGB         = 64,
	TinyImageFormat_R8G8B8X8_UNORM        = 65,
	TinyImageFormat_B8G8R8X8_UNORM        = 66,
	TinyImageFormat_R16G16_UNORM          = 67,
	TinyImageFormat_G16R16_UNORM          = 68,
	TinyImageFormat_R16G16_SNORM          = 69,
	TinyImageFormat_G16R16_SNORM          = 70,
	TinyImageFormat_R16G16_UINT           = 71,
	TinyImageFormat_R16G16_SINT           = 72,
	TinyImageFormat_R16G16_SFLOAT         = 73,
	TinyImageFormat_R16G16_SBFLOAT        = 74,
	TinyImageFormat_R32_UINT              = 75,
	TinyImageFormat_R32_SINT              = 76,
	TinyImageFormat_R32_SFLOAT            = 77,
	TinyImageFormat_A2R10G10B10_UNORM     = 78,
	TinyImageFormat_A2R10G10B10_UINT      = 79,
	TinyImageFormat_A2R10G10B10_SNORM     = 80,
	TinyImageFormat_A2R10G10B10_SINT      = 81,
	TinyImageFormat_A2B10G10R10_UNORM     = 82,
	TinyImageFormat_A2B10G10R10_UINT      = 83,
	TinyImageFormat_A2B10G10R10_SNORM     = 84,
	TinyImageFormat_A2B10G10R10_SINT      = 85,
	TinyImageFormat_R10G10B10A2_UNORM     = 86,
	TinyImageFormat_R10G10B10A2_UINT      = 87,
	TinyImageFormat_R10G10B10A2_SNORM     = 88,
	TinyImageFormat_R10G10B10A2_SINT      = 89,
	TinyImageFormat_B10G10R10A2_UNORM     = 90,
	TinyImageFormat_B10G10R10A2_UINT      = 91,
	TinyImageFormat_B10G10R10A2_SNORM     = 92,
	TinyImageFormat_B10G10R10A2_SINT      = 93,
	TinyImageFormat_B10G11R11_UFLOAT      = 94,
	TinyImageFormat_E5B9G9R9_UFLOAT       = 95,
	TinyImageFormat_R16G16B16_UNORM       = 96,
	TinyImageFormat_R16G16B16_SNORM       = 97,
	TinyImageFormat_R16G16B16_UINT        = 98,
	TinyImageFormat_R16G16B16_SINT        = 99,
	TinyImageFormat_R16G16B16_SFLOAT      = 100,
	TinyImageFormat_R16G16B16_SBFLOAT     = 101,
	TinyImageFormat_R16G16B16A16_UNORM    = 102,
	TinyImageFormat_R16G16B16A16_SNORM    = 103,
	TinyImageFormat_R16G16B16A16_UINT     = 104,
	TinyImageFormat_R16G16B16A16_SINT     = 105,
	TinyImageFormat_R16G16B16A16_SFLOAT   = 106,
	TinyImageFormat_R16G16B16A16_SBFLOAT  = 107,
	TinyImageFormat_R32G32_UINT           = 108,
	TinyImageFormat_R32G32_SINT           = 109,
	TinyImageFormat_R32G32_SFLOAT         = 110,
	TinyImageFormat_R32G32B32_UINT        = 111,
	TinyImageFormat_R32G32B32_SINT        = 112,
	TinyImageFormat_R32G32B32_SFLOAT      = 113,
	TinyImageFormat_R32G32B32A32_UINT     = 114,
	TinyImageFormat_R32G32B32A32_SINT     = 115,
	TinyImageFormat_R32G32B32A32_SFLOAT   = 116,
	TinyImageFormat_R64_UINT              = 117,
	TinyImageFormat_R64_SINT              = 118,
	TinyImageFormat_R64_SFLOAT            = 119,
	TinyImageFormat_R64G64_UINT           = 120,
	TinyImageFormat_R64G64_SINT           = 121,
	TinyImageFormat_R64G64_SFLOAT         = 122,
	TinyImageFormat_R64G64B64_UINT        = 123,
	TinyImageFormat_R64G64B64_SINT        = 124,
	TinyImageFormat_R64G64B64_SFLOAT      = 125,
	TinyImageFormat_R64G64B64A64_UINT     = 126,
	TinyImageFormat_R64G64B64A64_SINT     = 127,
	TinyImageFormat_R64G64B64A64_SFLOAT   = 128,
	TinyImageFormat_D16_UNORM             = 129,
	TinyImageFormat_X8_D24_UNORM          = 130,
	TinyImageFormat_D32_SFLOAT            = 131,
	TinyImageFormat_S8_UINT               = 132,
	TinyImageFormat_D16_UNORM_S8_UINT     = 133,
	TinyImageFormat_D24_UNORM_S8_UINT     = 134,
	TinyImageFormat_D32_SFLOAT_S8_UINT    = 135,
	TinyImageFormat_DXBC1_RGB_UNORM       = 136,
	TinyImageFormat_DXBC1_RGB_SRGB        = 137,
	TinyImageFormat_DXBC1_RGBA_UNORM      = 138,
	TinyImageFormat_DXBC1_RGBA_SRGB       = 139,
	TinyImageFormat_DXBC2_UNORM           = 140,
	TinyImageFormat_DXBC2_SRGB            = 141,
	TinyImageFormat_DXBC3_UNORM           = 142,
	TinyImageFormat_DXBC3_SRGB            = 143,
	TinyImageFormat_DXBC4_UNORM           = 144,
	TinyImageFormat_DXBC4_SNORM           = 145,
	TinyImageFormat_DXBC5_UNORM           = 146,
	TinyImageFormat_DXBC5_SNORM           = 147,
	TinyImageFormat_DXBC6H_UFLOAT         = 148,
	TinyImageFormat_DXBC6H_SFLOAT         = 149,
	TinyImageFormat_DXBC7_UNORM           = 150,
	TinyImageFormat_DXBC7_SRGB            = 151,
	TinyImageFormat_PVRTC1_2BPP_UNORM     = 152,
	TinyImageFormat_PVRTC1_4BPP_UNORM     = 153,
	TinyImageFormat_PVRTC2_2BPP_UNORM     = 154,
	TinyImageFormat_PVRTC2_4BPP_UNORM     = 155,
	TinyImageFormat_PVRTC1_2BPP_SRGB      = 156,
	TinyImageFormat_PVRTC1_4BPP_SRGB      = 157,
	TinyImageFormat_PVRTC2_2BPP_SRGB      = 158,
	TinyImageFormat_PVRTC2_4BPP_SRGB      = 159,
	TinyImageFormat_ETC2_R8G8B8_UNORM     = 160,
	TinyImageFormat_ETC2_R8G8B8_SRGB      = 161,
	TinyImageFormat_ETC2_R8G8B8A1_UNORM   = 162,
	TinyImageFormat_ETC2_R8G8B8A1_SRGB    = 163,
	TinyImageFormat_ETC2_R8G8B8A8_UNORM   = 164,
	TinyImageFormat_ETC2_R8G8B8A8_SRGB    = 165,
	TinyImageFormat_ETC2_EAC_R11_UNORM    = 166,
	TinyImageFormat_ETC2_EAC_R11_SNORM    = 167,
	TinyImageFormat_ETC2_EAC_R11G11_UNORM = 168,
	TinyImageFormat_ETC2_EAC_R11G11_SNORM = 169,
	TinyImageFormat_ASTC_4x4_UNORM        = 170,
	TinyImageFormat_ASTC_4x4_SRGB         = 171,
	TinyImageFormat_ASTC_5x4_UNORM        = 172,
	TinyImageFormat_ASTC_5x4_SRGB         = 173,
	TinyImageFormat_ASTC_5x5_UNORM        = 174,
	TinyImageFormat_ASTC_5x5_SRGB         = 175,
	TinyImageFormat_ASTC_6x5_UNORM        = 176,
	TinyImageFormat_ASTC_6x5_SRGB         = 177,
	TinyImageFormat_ASTC_6x6_UNORM        = 178,
	TinyImageFormat_ASTC_6x6_SRGB         = 179,
	TinyImageFormat_ASTC_8x5_UNORM        = 180,
	TinyImageFormat_ASTC_8x5_SRGB         = 181,
	TinyImageFormat_ASTC_8x6_UNORM        = 182,
	TinyImageFormat_ASTC_8x6_SRGB         = 183,
	TinyImageFormat_ASTC_8x8_UNORM        = 184,
	TinyImageFormat_ASTC_8x8_SRGB         = 185,
	TinyImageFormat_ASTC_10x5_UNORM       = 186,
	TinyImageFormat_ASTC_10x5_SRGB        = 187,
	TinyImageFormat_ASTC_10x6_UNORM       = 188,
	TinyImageFormat_ASTC_10x6_SRGB        = 189,
	TinyImageFormat_ASTC_10x8_UNORM       = 190,
	TinyImageFormat_ASTC_10x8_SRGB        = 191,
	TinyImageFormat_ASTC_10x10_UNORM      = 192,
	TinyImageFormat_ASTC_10x10_SRGB       = 193,
	TinyImageFormat_ASTC_12x10_UNORM      = 194,
	TinyImageFormat_ASTC_12x10_SRGB       = 195,
	TinyImageFormat_ASTC_12x12_UNORM      = 196,
	TinyImageFormat_ASTC_12x12_SRGB       = 197,
	TinyImageFormat_CLUT_P4               = 198,
	TinyImageFormat_CLUT_P4A4             = 199,
	TinyImageFormat_CLUT_P8               = 200,
	TinyImageFormat_CLUT_P8A8             = 201,
} TinyImageFormat;

typedef enum TinyImageFormat_LogicalChannel
{
	TinyImageFormat_LC_Red     = 0,
	TinyImageFormat_LC_Green   = 1,
	TinyImageFormat_LC_Blue    = 2,
	TinyImageFormat_LC_Alpha   = 3,
	TinyImageFormat_LC_Depth   = 0,
	TinyImageFormat_LC_Stencil = 1,
	TinyImageFormat_LC_0       = -1,
	TinyImageFormat_LC_1       = -2,
} TinyImageFormat_LogicalChannel;

typedef enum TinyImageFormat_PhysicalChannel
{
	TinyImageFormat_PC_0       = 0,
	TinyImageFormat_PC_1       = 1,
	TinyImageFormat_PC_2       = 2,
	TinyImageFormat_PC_3       = 3,
	TinyImageFormat_PC_CONST_0 = -1,
	TinyImageFormat_PC_CONST_1 = -2,
} TinyImageFormat_PhysicalChannel;

#define TinyImageFormat_Count 202U

typedef struct TinyImageFormat_DecodeInput
{
	union
	{
		void const* pixel;
		void const* pixelPlane0;
	};
	union
	{
		void const* lut;
		void const* pixelPlane1;
	};
	void const* pixelPlane2;
	void const* pixelPlane3;
	void const* pixelPlane4;
	void const* pixelPlane5;
	void const* pixelPlane6;
	void const* pixelPlane7;
	void const* pixelPlane8;
	void const* pixelPlane9;
} TinyImageFormat_FetchInput;

typedef struct TinyImageFormat_EncodeOutput
{
	union
	{
		void* pixel;
		void* pixelPlane0;
	};
	void* pixelPlane1;
	void* pixelPlane2;
	void* pixelPlane3;
	void* pixelPlane4;
	void* pixelPlane5;
	void* pixelPlane6;
	void* pixelPlane7;
	void* pixelPlane8;
	void* pixelPlane9;
} TinyImageFormat_EncodeOutput;

#endif // TINYIMAGEFORMAT_BASE_H_
