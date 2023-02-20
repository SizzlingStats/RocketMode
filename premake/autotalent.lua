
local base_dir = (solution().basedir .. "/external/autotalent/")

group "external"
    project "autotalent"
        kind "StaticLib"
        language "C++"
        location (_ACTION .. "/" .. project().name)
        defines { "_CRT_NONSTDC_NO_WARNINGS" }
        includedirs
        {
            base_dir
        }
        files
        {
            base_dir .. "**.h",
            base_dir .. "**.c"
        }
        add_tag("SDLCheck", "false", "autotalent")
        add_tag("ControlFlowGuard", "false", "autotalent")
    project "*"
group ""
