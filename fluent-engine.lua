function toboolean(str)
	local bool = false
	if str == "true" or str == "1" then
		bool = true
	end
	return bool
end

local use_sdl2 = true
if (use_sdl2) then
	newoption
	{
	trigger			= "sdl2_include_directory",
	description		= "SDL2 include directory"
	}

	newoption
	{
		trigger			= "sdl2_library_directory",
		description		= "SDL2 library directory"
	}
end

newoption
{
	trigger			= "vulkan_include_directory",
	description		= "Vulkan include directory",
}

newoption
{
	trigger			= "vulkan_backend",
	description		= "enable vulkan backend",
	default			= "true"
}

newoption
{
	trigger			= "d3d12_backend",
	description		= "enable directx12 backend",
	default			= "true"
}

newoption
{
	trigger			= "metal_backend",
	description		= "enable metal backend",
	default			= "true"
}

local commons = {}
commons.opts = function()
	filter { "configurations:debug" }
		defines( "FT_DEBUG=1" )
		symbols "On"
		optimize "Off"
	filter { "configurations:release" }
		defines( "FT_DEBUG=0" )
		symbols "Off"
		optimize "Speed"
	filter {}

	filter "system:windows"
		defines("NOMINMAX")
	filter {}
end

-- try to find deps first
if (use_sdl2) then
	sdl2_include_directory = os.findheader("SDL.h")
	sdl2_library_directory = os.findlib("SDL2")
end

vulkan_include_directory = os.findheader("vulkan/vulkan.h")

if (os.host() == "windows") then
	local vk_sdk = os.getenv("VULKAN_SDK")

	if (vk_sdk ~= nil) then
		vulkan_include_directory = vk_sdk .. "/Include"
	end
end

if (use_sdl2) then
	if sdl2_include_directory == nil then
		if _OPTIONS["sdl2_include_directory"] ~= nil then
			sdl2_include_directory = _OPTIONS["sdl2_include_directory"]
		else
			error("SDL2 headers not found you should manually specify directories")
		end
	end

	if sdl2_library_directory == nil then
		if _OPTIONS["sdl2_library_directory"] ~= nil then
			sdl2_library_directory = _OPTIONS["sdl2_library_directory"]
		else
			error("SDL2 library not found you should manually specify directories")
		end
	end
end

renderer_backend_vulkan = toboolean(_OPTIONS["vulkan_backend"])
renderer_backend_d3d12 = toboolean(_OPTIONS["d3d12_backend"])
renderer_backend_metal = toboolean(_OPTIONS["metal_backend"])

if os.host() ~= "macosx" then
	renderer_backend_metal = false
end

if os.host() ~= "windows" then
	renderer_backend_d3d12 = false
end

print("Enabled backends: ")
if (renderer_backend_vulkan) then
	print("\tVulkan")
end
if (renderer_backend_d3d12) then
	print("\tDirectX12")
end
if (renderer_backend_metal) then
	print("\tMetal")
end

if (renderer_backend_vulkan) then
	if vulkan_include_directory == nil then
		if _OPTIONS["vulkan_include_directory"] ~= nil then
			vulkan_include_directory = _OPTIONS["vulkan_include_directory"]
		else
			error("vulkan headers not found you should manually specify directories")
		end
	end
end

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
		buildoptions { "-fobjc-arc", "-fmodules" }
	end
end

-- TODO: -isystem /usr/include breaks #include_next
if (vulkan_include_directory == '/usr/include') then
	vulkan_include_directory = ""
end

if (renderer_backend_vulkan) then
	include("sources/third_party/vk_mem_alloc/premake5.lua")
	include("sources/third_party/volk/premake5.lua")
end
include("sources/third_party/hashmap_c/premake5.lua")
include("sources/third_party/spirv_reflect/premake5.lua")
include("sources/third_party/cgltf/premake5.lua")
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
		"sources",
	}

	sysincludedirs
	{
		"sources/third_party"
	}

	if (use_sdl2) then
		defines
		{
			"FT_WINDOW_SDL=1"
		}
		
		sysincludedirs
		{
			sdl2_include_directory
		}
	else
		defines
		{
			"FT_WINDOW_SDL=0"
		}
	end

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
		"sources/os/thread/thread.h",
		"sources/os/thread/unix/unix_thread.c",
		"sources/os/thread/windows/windows_thread.c",
		"sources/os/time/timer.h",
		"sources/os/time/unix/unix_timer.c",
		"sources/os/time/windows/windows_timer.c",
		"sources/os/window/window.c",
		"sources/os/window/window.h",
		"sources/os/window/sdl/sdl_window.c",
		"sources/os/window/xlib/xlib_window.h",
		"sources/os/window/xlib/xlib_window.c",
		"sources/fs/fs.c",
		"sources/fs/fs.h"
	}

	filter { "system:macosx" }
		files
		{
			"sources/os/window/cocoa/cocoa_window.m",
		}
	filter { }
project "ft_renderer"
	kind "StaticLib"
	language "C"

	commons.opts()

	declare_backend_defines()

	includedirs
	{
		"sources",
	}

	sysincludedirs
	{
		vulkan_include_directory,
		"sources/third_party"
	}

	backend_dir = "sources/renderer/backend/"
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
		path.join(backend_dir, "render_graph.h"),
		path.join(backend_dir, "render_graph.c"),
		path.join(backend_dir, "shader_reflection.h"),
		path.join(backend_dir, "vulkan/vulkan_reflection.c"),
		path.join(backend_dir, "vulkan/vulkan_backend.c"),
		path.join(backend_dir, "vulkan/vulkan_backend.h"),
		path.join(backend_dir, "d3d12/d3d12_reflection.c"),
		path.join(backend_dir, "d3d12/d3d12_backend.c"),
		path.join(backend_dir, "d3d12/d3d12_backend.h"),
		path.join(backend_dir, "vulkan/vulkan_pass_hasher.c"),
		path.join(backend_dir, "vulkan/vulkan_pass_hasher.h"),
		path.join(nuklear_dir, "ft_nuklear.h"),
		path.join(nuklear_dir, "ft_nuklear.c"),
		path.join(nuklear_dir, "shaders/shader_nuklear_vert_spirv.c"),
		path.join(nuklear_dir, "shaders/shader_nuklear_frag_spirv.c"),
		path.join(scene_dir, "model_loader.h"),
		path.join(scene_dir, "model_loader.c")
	}

	if (renderer_backend_metal) then
		files
		{
			path.join(backend_dir, "metal/metal_backend.h"),
			path.join(backend_dir, "metal/metal_backend.m"),
			path.join(backend_dir, "metal/metal_reflection.m")
		}
	end

	fluent_engine = {}

	fluent_engine.link = function()
		if (use_sdl2) then
			sysincludedirs
			{
				sdl2_include_directory
			}

			syslibdirs
			{
				sdl2_library_directory
			}

			links
			{
				"SDL2"
			}
		end

		links
		{
			"hashmap_c",
			"cgltf",
			"spirv_reflect",
			"stb"
		}
		
		filter { "system:linux" }
			links { "X11" }
		filter { "system:macosx" }
			links { "Cocoa.framework" }
		filter { }

		filter { "system:windows" }
			linkoptions
			{
				"imm32.lib",
				"version.lib",
				"setupapi.lib",
				"winmm.lib",
				"xinput.lib",
				"ws2_32.lib",
			}
		filter {}

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
