----------------------------------------------------------------------------
-- the dirs table is a listing of absolute paths, since we generate projects
-- and files it makes a lot of sense to make them absolute to avoid problems
outputdir = "%{cfg.buildcfg}-%{cfg.system}"
dirs = {}
dirs["root"] 			= os.realpath("../")
dirs["bin"]				= os.realpath(dirs.root .. "bin/")
dirs["temp"]			= os.realpath(dirs.root .. "intermediate/")
dirs["projectfiles"]	= os.realpath(dirs.root .. "local/")
dirs["source"] 			= os.realpath(dirs.root .. "src/")
dirs["lib_debug"]	= os.realpath(dirs.root .. "lib/debug/")
dirs["lib_release"]	= os.realpath(dirs.root .. "lib/release/")
dirs["content"] 	= os.realpath(dirs.root .. "content/")

dirs["game"] 			= os.realpath(dirs.source .. "game/")

dirs["include"]		= os.realpath(dirs.root .. "include/")
dirs["engine"]			= os.realpath(dirs.source .. "engine/")

dirs["shaders"]	=  os.realpath(dirs.content .. "shaders/")

if not os.isdir (dirs.bin) then
	os.mkdir (dirs.bin)
end
