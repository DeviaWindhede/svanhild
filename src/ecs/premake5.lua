project "ECS"
	location (dirs.ecs)

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
	}

	includedirs {
		".",
	}
	

include (dirs.ecs_tester)