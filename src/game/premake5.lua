project "Game"
	location (dirs.game)
	dependson { "engine" }
	
	kind "WindowedApp"
	language "C++"
	cppdialect "C++20"

	debugdir "%{dirs.bin}"
	targetdir ("%{dirs.bin}")
	targetname("%{prj.name}_%{cfg.buildcfg}")
	objdir ("%{dirs.temp}/%{prj.name}")

	links { "engine" }

	pchheader "pch.h"
	pchsource ("pch.cpp")

	includedirs { ".", dirs.include, dirs.engine }

	files {
		"**.h",
		"**.cpp",
		"**.hlsl",
        "**.hlsli",
	}

	--verify_or_create_settings("Game")
	
	filter "configurations:Debug"
		defines "_DEBUG"
		runtime "Debug"
		symbols "on"
		libdirs { dirs.lib_debug }
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
			-- "FatalCompileWarnings",
			"MultiProcessorCompile"
		}
		
		defines {
			"WIN32",
			"_LIB"
		}

	shadermodel("5.0")
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