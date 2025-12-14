project "CryoC"
    kind "ConsoleApp"
    language "c++"
    cppdialect "c++23"

    targetdir ( "%{wks.location}/bin/" .. outputdir )
    objdir ( "%{wks.location}/bin-int/" .. outputdir )

    pchheader "cryopch.h"
    pchsource "src/cryppch.cpp"

    files 
    {
        "%{prj.location}/src/**.cpp",
        "%{prj.location}/src/**.h",
        "%{prj.location}/src/**.hpp",
        "%{prj.location}/vendor/spdlog/include/**.h",
        "%{prj.location}/vendor/spdlog/include/**.hpp"
    }

    includedirs
    {
        "%{prj.location}/src",
        "%{prj.location}/vendor/spdlog/include",
        "%{prj.location}/vendor/toml"
    }
