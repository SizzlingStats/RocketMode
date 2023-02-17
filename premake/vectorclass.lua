
local base_dir = (solution().basedir .. "/external/vectorclass/")

group "external"
    project "vectorclass"
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
        add_tag("SDLCheck", "false", "vectorclass")
        add_tag("ControlFlowGuard", "false", "vectorclass")
    project "*"
group ""
