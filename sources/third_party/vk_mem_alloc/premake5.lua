project "vk_mem_alloc"
    kind "StaticLib"
    language "C++"

    includedirs {
        ".",
        "../",
        vulkan_include_directory
    }

    files {
        "vk_mem_alloc.h",
        "vk_mem_alloc.cpp"
    }
