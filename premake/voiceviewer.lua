
local base_dir = (solution().basedir .. "/voiceviewer/")

project "voiceviewer"
    kind "WindowedApp"
    language "C++"
    location (_ACTION .. "/" .. project().name)
    includedirs
    {
        base_dir,
        "../external"
    }
    files
    {
        base_dir .. "**.h",
        base_dir .. "**.cpp"
    }
    links
    {
        "imgui",
        "D3D11.lib"
    }
    add_tag("SDLCheck", "false", "voiceviewer")
    add_tag("ControlFlowGuard", "false", "voiceviewer")
project "*"
