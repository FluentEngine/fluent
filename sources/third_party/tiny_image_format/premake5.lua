project "tiny_image_format"
    kind "StaticLib"
    language "C"

    includedirs {
        "../"
    }

    files {
        "tinyimageformat_apis.h",
        "tinyimageformat_base.h",
        "tinyimageformat_bits.h",
        "tinyimageformat_decode.h",
        "tinyimageformat_encode.h",
        "tinyimageformat_query.h",
        "tinyimageformat.h",
        "tinyimageformat.c",
    }
