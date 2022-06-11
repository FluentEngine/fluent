project "vk_mem_alloc"
    kind "StaticLib"
    language "C++"
	cppdialect "C++17"
	
    includedirs 
    {
        ".",
        "../",
    }
	
	sysincludedirs 
	{
		vulkan_include_directory
	}
	
    files 
    {
        "vk_mem_alloc.h",
        "vk_mem_alloc.cpp"
    }
	
	filter { "toolset:clang" }
		buildoptions { "-Wno-nullability-completeness" }
	filter { }
