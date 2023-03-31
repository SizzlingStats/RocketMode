
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
    local copy_dll = "copy /Y \"$(TargetDir)" .. target_name .. "\" "
    postbuild_copy_dll = copy_dll .. "\"" .. srcds_path .. "tf/addons/" .. target_name .. "\""

    local vdf_name = "sizzlingvoice.vdf"
    local copy_vdf = "copy /Y \"$(TargetDir)" .. vdf_name .. "\" "
    postbuild_copy_vdf = copy_vdf .. "\"" .. srcds_path .. "tf/addons/" .. vdf_name .. "\""

    local wav_name = "*.wav"
    local copy_wav = "copy /Y \"$(TargetDir)" .. wav_name .. "\" "
    postbuild_copy_wav = copy_wav .. "\"" .. srcds_path .. "tf/addons/\""
end

solution "sizzlingvoice"
    basedir ".."
    location (_ACTION)
    targetdir "../addons"
    startproject "sizzlingvoice"
    debugdir "../addons"
    configurations { "Debug", "Release" }

    platforms "x32"
    flags { "MultiProcessorCompile" }
    exceptionhandling "Off"
    floatingpoint "Fast"
    floatingpointexceptions "off"
    rtti "Off"
    editandcontinue "Off"
    justmycode "Off"
    cppdialect "C++17"
    symbols "On"
    vectorextensions "SSE2"
    pic "On"
    targetprefix ""

    filter { "system:linux" }
        toolset "clang"
    filter {}

    --defines { "SDK_COMPAT" }
    defines { "_CRT_SECURE_NO_WARNINGS", "_CRT_NONSTDC_NO_WARNINGS", "INSTRSET=2" }
    configuration "Debug"
        defines { "DEBUG" }
        inlining "Explicit"
    configuration "Release"
        defines { "NDEBUG" }
        optimize "Full"
        flags { "NoBufferSecurityCheck", "NoIncrementalLink" }
    configuration {}
    
    filter { "Release", "toolset:msc*" }
        buildoptions { "/GL" }
        linkoptions { "/LTCG" }
    filter { "Release", "toolset:not msc*" }
        flags { "LinkTimeOptimization" }
    filter { "toolset:not msc*" }
        buildoptions { "-fvisibility=hidden", "-ffunction-sections", "-fdata-sections" }
        linkoptions { "-Wl,--no-undefined,--no-allow-shlib-undefined,--gc-sections" }
    filter {}

    project "sizzlingvoice"
        kind "SharedLib"
        language "C++"

        configuration "vs*"
            debugcommand (srcds_exe)
            debugargs "-console -game tf +sv_voicecodec vaudio_celt +map cp_granary"
            --debugargs "-game tf -console -nomaster -insecure -maxplayers 32 +sv_lan 1 -allowdebug -NOINITMEMORY +map cp_granary"
            postbuildcommands { postbuild_copy_dll, postbuild_copy_vdf, postbuild_copy_wav }
        configuration {}
        files
        {
            "../sizzlingvoice/**.h",
            "../sizzlingvoice/**.cpp",
            "../sizzlingvoice/**.natvis"
        }
        includedirs
        {
            "../sizzlingvoice",
            "../external"
        }
        links
        {
            "sourcesdk",
            "vectorclass",
            "hde32"
        }
        add_tag("SDLCheck", "false", "sizzlingvoice")
        add_tag("ControlFlowGuard", "false", "sizzlingvoice")
    project "*"

    dofile "sourcesdk.lua"
    dofile "vectorclass.lua"
    dofile "hde32.lua"
