project "Engine"
	location (dirs.engine)
	dependson { "include" }

	kind "StaticLib"
	language "C++"
	cppdialect "C++20"
	
	debugdir "%{dirs.bin}"
	targetdir ("%{dirs.bin}")
	targetname("%{prj.name}_%{cfg.buildcfg}")
	objdir ("%{dirs.temp}/%{prj.name}")

	pchheader "pch.h"
	pchsource ("pch.cpp")
	
	files {
		"**.h",
		"**.cpp",
		"**.hlsl",
		"**.hlsli",
	}

	includedirs {
		".",
		dirs.include,
		dirs.engine
	}

	links {
		"d3d12.lib",
		"dxgi.lib",
		"d3dcompiler.lib",
		"dxguid.lib",
        "zlib-md.lib",
        "libxml2-md.lib",
		"libfbxsdk-md.lib",
		"DirectXTex.lib"
		-- "DirectXTK.lib"
	}
	
	--links	{"PhysX_64.lib", "PhysXCommon_64.lib", "PhysXFoundation_64.lib", "PVDRuntime_64.lib", "PhysXExtensions_static_64.lib", "PhysXPvdSDK_static_64.lib", "PhysXCooking_64.lib"}

	
	filter "configurations:Debug"
		defines "_DEBUG"
		runtime "Debug"
		symbols "on"
		libdirs { dirs.lib_debug }	
		links { "wininet.lib" }
		ignoredefaultlibraries { "LIBCMT" }
	filter "configurations:Release"
		defines "_RELEASE"
		runtime "Release"
		optimize "on"
		libdirs { dirs.lib_release }	
	filter "system:windows"
--		kind "StaticLib"
		staticruntime "off"
		symbols "On"		
		systemversion "latest"
		warnings "Default"
		--conformanceMode "On"
		--buildoptions { "/permissive" }
		flags { 
		--	"FatalWarnings", -- would be both compile and lib, the original didn't set lib
			-- "FatalCompileWarnings", -- CD3DX12_RESOURCE_BARRIER::Transition is evil and for now requires lvl3 warnings instead of 4 :c
			"MultiProcessorCompile"
		}
		
		defines {
			"WIN32",
			"_LIB"
		}

	shadermodel("5.1")
	os.mkdir(dirs.shaders)
	filter("files:**.hlsl")
		flags("ExcludeFromBuild")
			shaderobjectfileoutput(dirs.shaders .. "%{file.basename}.cso")

		
    filter("files:**PS.hlsl")
        removeflags("ExcludeFromBuild")
        shadertype("Pixel")

    filter("files:**VS.hlsl")
        removeflags("ExcludeFromBuild")
        shadertype("Vertex")

    filter("files:**GS.hlsl")
        removeflags("ExcludeFromBuild")
        shadertype("Geometry")
