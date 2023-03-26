
local base_dir = (solution().basedir .. "/external/hde32/")

group "external"
    project "hde32"
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
            base_dir .. "**.c"
        }
        add_tag("SDLCheck", "false", "hde32")
        add_tag("ControlFlowGuard", "false", "hde32")
    project "*"
group ""
