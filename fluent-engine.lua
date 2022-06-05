if (not os.isdir(root_directory .. "/deps/SDL"))
then
    sdl_repo = "https://github.com/FluentEngine/SDL.git " .. root_directory .. "/deps/SDL"
    os.execute("git clone " .. sdl_repo )
end

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
sdl_include_dir = root_directory .. "/deps/SDL/include"
vulkan_include_directory = os.findheader("vulkan/vulkan.h")

if (os.host() == "windows") then
	local vk_sdk = os.getenv("VULKAN_SDK")

	if (vk_sdk ~= nil) then
		vulkan_include_directory = vk_sdk .. "/Include"
	end
end

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

include(root_directory .. "/deps/SDL/sdl2.lua")
include("sources/third_party/hashmap_c/premake5.lua")
include("sources/third_party/spirv_reflect/premake5.lua")
include("sources/third_party/cgltf/premake5.lua")
include("sources/third_party/tiny_image_format/premake5.lua")
include("sources/third_party/vk_mem_alloc/premake5.lua")
include("sources/third_party/volk/premake5.lua")
include("sources/third_party/stb/premake5.lua")

project "ft_log"
	kind "StaticLib"
	language "C"
	
	commons.opts()
	
	includedirs 
	{
		"sources"
	}

	files 
	{
		"sources/log/log.h",
		"sources/log/log.c"
	}

project "ft_os"
    kind "StaticLib"
    language "C"
	
	commons.opts()

	declare_backend_defines()
	
    includedirs 
    {
		sdl_include_dir,
        "sources",
    }

    files 
    {
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
	
    includedirs 
    {
		sdl_include_dir,
		vulkan_include_directory,
        "sources",
        "sources/third_party"
    }
	
	backend_dir = "sources/renderer/backend/"
	rg_dir = "sources/renderer/render_graph/"
	nuklear_dir = "sources/renderer/nuklear/"
	shader_reflection_dir = "sources/renderer/shader_reflection/"
	scene_dir = "sources/renderer/scene/"
	
    files 
    {
		path.join(backend_dir, "renderer_backend.c"),
		path.join(backend_dir, "renderer_backend.h"),
		path.join(backend_dir, "renderer_enums.h"),
		path.join(backend_dir, "renderer_private.h"),
		path.join(backend_dir, "resource_loader.c"),
		path.join(backend_dir, "vulkan/vulkan_backend.c"),
		path.join(backend_dir, "vulkan/vulkan_backend.h"),
		path.join(backend_dir, "d3d12/d3d12_backend.c"),
		path.join(backend_dir, "d3d12/d3d12_backend.h"),
		path.join(rg_dir, "render_graph.h"),
		path.join(rg_dir, "render_graph.c"),
		path.join(rg_dir, "render_graph_private.h"),
		path.join(rg_dir, "vulkan_graph.c"),
		path.join(rg_dir, "vulkan_graph.h"),
		path.join(nuklear_dir, "ft_nuklear.h"),
		path.join(nuklear_dir, "ft_nuklear.c"),
		path.join(shader_reflection_dir, "shader_reflection.h"),
		path.join(shader_reflection_dir, "vulkan_reflection.c"),
		path.join(scene_dir, "model_loader.h"),
		path.join(scene_dir, "model_loader.c")
    }

	fluent_engine = {}

	fluent_engine.link = function()
		links {
			"hashmap_c",
			"cgltf",
			"spirv_reflect",
			"tiny_image_format",
			"sdl2",
			"stb"
		}

		if (renderer_backend_vulkan) then
			links
			{
				"vk_mem_alloc",
				"volk",
			}
		end

		if (renderer_backend_d3d12) then
			links
			{
				"d3d12", 
				"dxgi",
				"dxguid",
			}
		end
	end
