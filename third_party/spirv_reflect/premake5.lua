project "spirv_reflect"
    kind "StaticLib"
    language "C"

    includedirs {
        "../"
    }

    files {
        "spirv_reflect.h",
        "spirv_reflect.c"
    }
