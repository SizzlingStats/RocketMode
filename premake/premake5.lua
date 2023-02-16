
function add_tag(tag, value, project_name)
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

newoption
{
    trigger     = "srcds-path",
    value       = "path",
    description = "Path to your srcds installation. Used for debugging and binary deployment."
}

local srcds_path = _OPTIONS["srcds-path"]
local srcds_exe = nil
local postbuild_move = nil
local postbuild_move_vdf = nil
if srcds_path ~= nil then
    srcds_exe = srcds_path .. "srcds.exe"
    
    local target_name = "$(TargetName)$(TargetExt)"
    local mklink_dll = "copy /Y \"$(TargetDir)" .. target_name .. "\" "
    postbuild_link_dll = mklink_dll .. "\"" .. srcds_path .. "tf/addons/" .. target_name .. "\""
    
    local vdf_name = "sizzlingvoice.vdf"
    local mklink_vdf = "copy /Y \"$(TargetDir)" .. vdf_name .. "\" "
    postbuild_link_vdf = mklink_vdf .. "\"" .. srcds_path .. "tf/addons/" .. vdf_name .. "\""
end

solution "sizzlingvoice"
    basedir ".."
    location (_ACTION)
    targetdir "../addons"
    startproject "sizzlingvoice"
    debugdir "../addons"
    configurations { "Debug", "Release" }

    platforms "x32"
    flags { "MultiProcessorCompile", "Symbols" }
    exceptionhandling "Off"
    floatingpoint "Fast"
    floatingpointexceptions "off"
    rtti "Off"
    editandcontinue "Off"
    justmycode "Off"
    cppdialect "C++17"

    -- defines "_CRT_SECURE_NO_WARNINGS"
    defines { "INSTRSET=2" }
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

        configuration "vs*"
            debugcommand (srcds_exe)
            debugargs "-console -game tf +sv_voicecodec vaudio_celt +map cp_granary"
            postbuildcommands { postbuild_link_dll, postbuild_link_vdf }
        configuration {}
        files
        {
            "../sizzlingvoice/**.h",
            "../sizzlingvoice/**.cpp"
        }
        includedirs
        {
            "../sizzlingvoice",
            "../external"
        }
        links
        {
            "sourcesdk"
        }
        add_tag("SDLCheck", "false", "sizzlingvoice")
        add_tag("ControlFlowGuard", "false", "sizzlingvoice")
    project "*"

    dofile "sourcesdk.lua"
