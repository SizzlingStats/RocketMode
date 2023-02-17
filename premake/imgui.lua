
local base_dir = (solution().basedir .. "/external/imgui/")

group "external"
    project "imgui"
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
        add_tag("SDLCheck", "false", "imgui")
        add_tag("ControlFlowGuard", "false", "imgui")
    project "*"
group ""
