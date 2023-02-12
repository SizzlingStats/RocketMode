
local function add_tag(tag, value, project_name)
    if string.find(_ACTION, "vs") then
        require "vstudio"
        premake.override(premake.vstudio.vc2010.elements, "clCompile",
            function(oldfn, cfg)
                local calls = oldfn(cfg)
                if project_name == nil or cfg.project.name == project_name then
                    table.insert(calls, function(cfg)
                        premake.vstudio.vc2010.element(tag, nil, value)
                    end)
                end
                return calls
            end)
    end
end

solution "sizzlingvoice"
    basedir ".."
    location (_ACTION)
    targetdir "../addons"
    startproject "sizzlingvoice"
    debugdir "../addons"
    configurations { "Debug", "Release" }
    debugargs
    {
    }

    platforms "x32"
    flags { "MultiProcessorCompile", "Symbols" }
    exceptionhandling "Off"
    floatingpoint "Fast"
    floatingpointexceptions "off"
    rtti "Off"
    editandcontinue "Off"

    -- defines "_CRT_SECURE_NO_WARNINGS"
    configuration "Debug"
        defines { "DEBUG" }
    configuration "Release"
        defines { "NDEBUG" }
        optimize "Full"
        flags { "NoBufferSecurityCheck", "NoIncrementalLink" }
        buildoptions { "/GL" }
        linkoptions { "/LTCG" }
    configuration {}

    project "sizzlingvoice"
        kind "SharedLib"
        language "C++"
        configuration "gmake"
            buildoptions { "-std=c++17" }
        configuration {}
        files
        {
            "../ServerPlugin/**.h",
            "../ServerPlugin/**.cpp"
        }
        includedirs
        {
            "../ServerPlugin"
        }
        add_tag("SupportJustMyCode", "false", "sizzlingvoice")
        add_tag("LanguageStandard", "stdcpp17", "sizzlingvoice")
        add_tag("SDLCheck", "false", "sizzlingvoice")
        add_tag("ControlFlowGuard", "false", "sizzlingvoice")
    project "*"
