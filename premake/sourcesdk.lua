
local base_dir = (solution().basedir .. "/external/sourcesdk/")

group "external"
    project "sourcesdk"
        kind "StaticLib"
        language "C++"
        location (_ACTION .. "/" .. project().name)
        includedirs
        {
            base_dir
        }
        files
        {
            base_dir .. "**.h",
            base_dir .. "**.cpp"
        }
        add_tag("SDLCheck", "false", "sourcesdk")
        add_tag("ControlFlowGuard", "false", "sourcesdk")
    project "*"
group ""
