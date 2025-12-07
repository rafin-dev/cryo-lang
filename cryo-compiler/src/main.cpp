#include "cryopch.h"

#include <spdlog/spdlog.h>

#include "assembler/Assembler.h"

int main(int argc, const char** argv)
{
    spdlog::set_pattern("[%H:%M:%S|%z|%l] %v");
    if (argc < 2) 
    {
        spdlog::critical("Missing arguments!");
        return -1;
    }

    std::filesystem::path path(argv[1]);
    if (!std::filesystem::exists(path) || path.extension() != ".cryoAsm")
    {
        spdlog::critical("Invalid file: [{0}]!", path.string());
        return -1;
    }

    // TODO: deal with more arguments
    Cryo::ErrorQueue errors;
    Cryo::Assembler::Assembler assembler(argv[argc - 1]);

    assembler.assemble(errors);
    if (errors.get_severity() != Cryo::Error::level_none)
    {
        errors.log();
        return -2;
    }
    spdlog::info("Compilation finished successfully!");
}
