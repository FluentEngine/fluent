include("sources/third_party/hashmap.c/premake5.lua")
include("sources/third_party/spirv_reflect/premake5.lua")
include("sources/third_party/tiny_image_format/premake5.lua")
include("sources/third_party/vk_mem_alloc/premake5.lua")
include("sources/third_party/volk/premake5.lua")

local commons = {}
commons.opts = function()
	filter { "configurations:debug" }
		defines( "FLUENT_DEBUG" )
		symbols "On"
		optimize "Off"
	filter { "configurations:release" }
		optimize "Speed"
	filter {}
	
	filter "system:windows"
		defines("NOMINMAX")
	filter {}
end

-- renderer
-- todo: option
vulkan_include_directory = "/usr/include/vulkan/"

renderer_backend_vulkan = true
renderer_backend_d3d12 = false
renderer_backend_metal = false

function declare_backend_defines()
	if (renderer_backend_vulkan) 
	then 
		defines { "VULKAN_BACKEND" }
	end
	
	if (renderer_backend_d3d12) 
	then 
		defines { "D3D12_BACKEND" }
	end
	
	if (renderer_backend_metal) 
	then 
		defines { "METAL_BACKEND" }
		buildoptions( "-fobjc-arc", "-fobjc-weak", "-fmodules")
	end
end

project "ft_log"
	kind "StaticLib"
	language "C"
	
	commons.opts()
	
	includedirs {
		"sources"
	}

	files {
		"sources/log/log.h",
		"sources/log/log.c"
	}

project "ft_os"
    kind "StaticLib"
    language "C"
	
	commons.opts()

	declare_backend_defines()
	
    includedirs {
        "sources"
    }

    files {
        "sources/os/application.c", 
        "sources/os/application.h",
        "sources/os/camera.h",
        "sources/os/camera.c",
        "sources/os/input.c",
        "sources/os/input.h",
        "sources/os/key_codes.h",
        "sources/os/mouse_codes.h",
        "sources/os/window.c", 
        "sources/os/window.h", 
    }

project "ft_renderer"
    kind "StaticLib"
    language "C"
	
	commons.opts()
	
	declare_backend_defines()
	
    includedirs {
        "sources",
        "sources/third_party"
    }

    files {
        "sources/renderer/shader_reflection.h", 
        "sources/renderer/renderer_enums.h",
        "sources/renderer/renderer_backend.h",
        "sources/renderer/renderer_backend_functions.h",
        "sources/renderer/renderer_backend.c",
        "sources/renderer/nuklear/ft_nuklear.h",
        "sources/renderer/nuklear/ft_nuklear.c",
        "sources/renderer/vulkan/vulkan_backend.h", 
        "sources/renderer/vulkan/vulkan_backend.c",
        "sources/renderer/vulkan/vulkan_reflection.c",
        "sources/renderer/d3d12/d3d12_backend.h", 
        "sources/renderer/d3d12/d3d12_backend.c",
        "sources/renderer/d3d12/d3d12_reflection.c",
        "sources/renderer/resource_loader.h", 
        "sources/renderer/resource_loader.c"
    }
