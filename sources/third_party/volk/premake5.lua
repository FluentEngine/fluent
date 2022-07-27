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
	
	filter { "system:windows" }
		defines { "VK_USE_PLATFORM_WIN32_KHR" }
	filter { "system:linux" }
		defines { "VK_USE_PLATFORM_XLIB_KHR" }
	filter { "system:macosx" } 
		defines { "VK_USE_PLATFORM_MACOS_MVK" }
	filter {}
	
	files 
	{
		"volk.h",
		"volk.c"
	}
