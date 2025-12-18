cmake_minimum_required(VERSION 4.1)

project(cryoc)

set(CMAKE_CXX_STANDARD 23)

add_executable(cryoc src/main.cpp
        src/cryopch.h
        src/cryopch.cpp

        src/assembler/Assembler.h
        src/assembler/Assembler.cpp
        src/assembler/Instructions.h
        src/assembler/InstructionSet.h
        src/assembler/InstructionSet.cpp
        src/assembler/Token.h
        src/assembler/Tokenizer.h
        src/assembler/Tokenizer.cpp
        src/assembler/TypeList.h
        src/assembler/TypeList.cpp
        src/assembler/VariableStack.h
        src/assembler/VariableStack.cpp

        src/linker/Linker.h
        src/linker/Linker.cpp

        src/environment/CompilationEnvironment.h
        src/environment/CompilationEnvironment.cpp

        src/common/Error.h
        src/common/Error.cpp
)

include_directories(src
        vendor/spdlog/include
        vendor/toml
)

target_precompile_headers(cryo
    PUBLIC
        src/cryopch.h
)
