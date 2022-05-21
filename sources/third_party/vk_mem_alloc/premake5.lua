project "vk_mem_alloc"
    kind "StaticLib"
    language "C++"

    includedirs {
        ".",
        "../"
    }

    files {
        "vk_mem_alloc.h",
        "vk_mem_alloc.cpp"
    }
