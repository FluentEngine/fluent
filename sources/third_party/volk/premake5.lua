project "volk"
    kind "StaticLib"
    language "C"

    includedirs 
    {
        ".",
    }
    
    sysincludedirs
    {
		vulkan_include_directory
    }
    
    files 
    {
        "volk.h",
        "volk.c"
    }
