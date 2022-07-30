function toboolean(str)
	local bool = false
	if str == "true" or str == "1" then
		bool = true
	end
	return bool
end

newoption {
	trigger			= "vulkan_include_directory",
	description		= "Vulkan include directory",
}

newoption {
	trigger			= "vulkan_backend",
	description		= "enable vulkan backend",
	default			= "true"
}

newoption {
	trigger			= "d3d12_backend",
	description		= "enable directx12 backend",
	default			= "true"
}

newoption {
	trigger			= "metal_backend",
	description		= "enable metal backend",
	default			= "true"
}

vulkan_include_directory = os.findheader("vulkan/vulkan.h")

if (os.host() == "windows") then
	local vk_sdk = os.getenv("VULKAN_SDK")

	if (vk_sdk ~= nil) then
		vulkan_include_directory = vk_sdk .. "/Include"
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
		defines { "FT_VULKAN_BACKEND=1" }
	else
		defines { "FT_VULKAN_BACKEND=0" }
	end

	if (renderer_backend_d3d12)
	then
		defines { "FT_D3D12_BACKEND=1" }
	else
		defines { "FT_D3D12_BACKEND=0" }
	end

	if (renderer_backend_metal)
	then
		defines { "FT_METAL_BACKEND=1" }
		buildoptions { "-fobjc-arc", "-fmodules" }
	else
		defines { "FT_METAL_BACKEND=0" }
	end
end

-- TODO: -isystem /usr/include breaks #include_next
if (vulkan_include_directory == '/usr/include') then
	vulkan_include_directory = ""
end

if (renderer_backend_vulkan) then
	include("third_party/vk_mem_alloc/premake5.lua")
	include("third_party/volk/premake5.lua")
end
include("third_party/hashmap_c/premake5.lua")
include("third_party/spirv_reflect/premake5.lua")
include("third_party/cgltf/premake5.lua")
include("third_party/stb/premake5.lua")


project "fluent-engine"
	kind "StaticLib"
	language "C"

	filter { "configurations:debug" }
		symbols "On"
		optimize "Off"
		defines {
			"FT_DEBUG=1" 
		}
	filter { "configurations:release" }
		symbols "Off"
		optimize "Speed"
		defines {
			"FT_DEBUG=0" 
		}
	filter { "system:windows" }
		defines {
			"NOMINMAX",
			"_CRT_SECURE_NO_WARNINGS"
		}
	filter {}

	declare_backend_defines()

	includedirs {
		"sources",
	}

	sysincludedirs {
		"third_party",
		vulkan_include_directory
	}

	files {
		-- base
		"sources/base/base.h",
		"sources/base/log.h",
		"sources/base/log.c",
		-- app
		"sources/app/application.c",
		"sources/app/application.h",
		-- camera
		"sources/camera/camera.h",
		"sources/camera/camera.c",
		-- window
		"sources/window/input.c",
		"sources/window/input.h",
		"sources/window/key_codes.h",
		"sources/window/mouse_codes.h",
		-- thread
		"sources/thread/thread.h",
		"sources/thread/unix/unix_thread.c",
		"sources/thread/windows/windows_thread.c",
		-- time
		"sources/time/timer.h",
		"sources/time/unix/unix_timer.c",
		"sources/time/windows/windows_timer.c",
		-- window
		"sources/window/window.c",
		"sources/window/window.h",
		"sources/window/xlib/xlib_window.h",
		"sources/window/xlib/xlib_window.c",
		"sources/window/winapi/winapi_window.h",
		"sources/window/winapi/winapi_window.c",
		-- fs
		"sources/fs/fs.c",
		"sources/fs/fs.h",
		-- renderer
		"sources/renderer/backend/renderer_backend.c",
		"sources/renderer/backend/renderer_backend.h",
		"sources/renderer/backend/renderer_enums.h",
		"sources/renderer/backend/renderer_private.h",
		"sources/renderer/backend/resource_loader.c",
		"sources/renderer/backend/render_graph.h",
		"sources/renderer/backend/render_graph.c",
		"sources/renderer/backend/shader_reflection.h",
		"sources/renderer/backend/vulkan/vulkan_reflection.c",
		"sources/renderer/backend/vulkan/vulkan_backend.c",
		"sources/renderer/backend/vulkan/vulkan_backend.h",
		"sources/renderer/backend/d3d12/d3d12_reflection.c",
		"sources/renderer/backend/d3d12/d3d12_backend.c",
		"sources/renderer/backend/d3d12/d3d12_backend.h",
		"sources/renderer/backend/vulkan/vulkan_pass_hasher.c",
		"sources/renderer/backend/vulkan/vulkan_pass_hasher.h",
		"sources/renderer/nuklear/ft_nuklear.h",
		"sources/renderer/nuklear/ft_nuklear.c",
		"sources/renderer/nuklear/shaders/shader_nuklear_vert_spirv.c",
		"sources/renderer/nuklear/shaders/shader_nuklear_frag_spirv.c",
		"sources/renderer/scene/model_loader.h",
		"sources/renderer/scene/model_loader.c"
	}

	filter { "system:macosx" }
		files {
			-- window
			"sources/window/cocoa/cocoa_window.m",
			-- renderer
			"sources/renderer/backend/metal/metal_backend.h",
			"sources/renderer/backend/metal/metal_backend.m",
			"sources/renderer/backend/metal/metal_reflection.m"
		}
	filter { }

fluent_engine = {}

fluent_engine.link = function()

	links {
		"fluent-engine"
	}

	links {
		"hashmap_c",
		"cgltf",
		"spirv_reflect",
		"stb"
	}
	
	filter { "system:linux" }
		links { 
			"X11",
			"pthread",
			"dl",
			"m"
		}
	filter { "system:macosx" }
		links { "Cocoa.framework" }
	filter { "system:windows" }
		linkoptions	{
			"imm32.lib",
			"version.lib",
			"setupapi.lib",
			"winmm.lib",
			"xinput.lib",
			"ws2_32.lib",
		}
		entrypoint "mainCRTStartup"
	filter {}

	if (renderer_backend_vulkan) then
		links {
			"vk_mem_alloc",
			"volk",
		}
	end

	if (renderer_backend_d3d12) then
		links {
			"d3d12",
			"dxgi",
			"dxguid",
		}
	end
end
