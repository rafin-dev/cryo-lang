workspace "Cryo"
    startproject "Cryo"
    configurations { "Debug", "Release", "Dist" }
    platforms { "x64" }
	
	defaultplatform ("x64")

    filter "configurations:Debug"
        defines { "DEBUG" }
        symbols "On"

    filter "configurations:Release"
        defines { "RELEASE" }
        optimize "On"

    filter "configurations:Dist"
        defines { "Dist" }
        optimize "on"

    filter { "platforms:x64" }
        architecture "x86_64"
        
    filter {}

    targetdir "bin/%{cfg.buildcfg}/"

    cdialect "C99"
    cppdialect "C++23"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}/%{prj.name}"

include "CryoInterpreter"
include "CryoCompiler"
