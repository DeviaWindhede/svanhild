include "../premake/extensions.lua"
-- include for common stuff 
include "../premake/common.lua"

workspace "svanhild"
	location "../"
	startproject "Game"
	architecture "x64"

	configurations {
		"Debug",
		"Release"
	}

	include (dirs.include)
	include (dirs.ecs)
	include (dirs.engine)
	include (dirs.game)
