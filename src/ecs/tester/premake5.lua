project "ECS Tester"
	location (dirs.ecs_tester)
	dependson { "ecs" }

	kind "WindowedApp"
	language "C++"
	cppdialect "C++20"
	
	debugdir "%{dirs.bin}"
	targetdir ("%{dirs.bin}")
	targetname("%{prj.name}_%{cfg.buildcfg}")
	objdir ("%{dirs.temp}/%{prj.name}")

	links { "ecs" }

	pchheader "pch.h"
	pchsource ("pch.cpp")
	
	files {
		"**.h",
		"**.cpp",
	}

	includedirs {
		".",
		dirs.ecs
	}
	
