project "volk"
    kind "StaticLib"
    language "C"

    includedirs {
        ".",
        vulkan_include_directory
    }

    print(vulkan_include_directory)
    
    files {
        "volk.h",
        "volk.c"
    }
