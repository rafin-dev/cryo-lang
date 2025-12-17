cmake_minimum_required(VERSION 3.2)

project(cryo)

set(CMAKE_CXX_STANDARD 23)

add_executable(cryo src/main.cpp
        src/cryopch.h
        src/cryopch.cpp

        src/core/CryoAssembly.h
        src/core/CryoAssembly.cpp
        src/core/CryoInstructions.h
        src/core/CryoState.h
        src/core/CryoState.cpp
        src/core/CryoThread.h
        src/core/CryoThread.cpp
        src/core/ImplFunctions.cpp
        src/core/Stack.h
        src/core/Stack.cpp
)

include_directories(src)

target_precompile_headers(cryo
    PUBLIC
        src/cryopch.h
)
