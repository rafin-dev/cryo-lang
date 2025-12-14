#include "cryopch.h"

#include <spdlog/spdlog.h>

#include "assembler/Assembler.h"
#include "assembler/TypeList.h"

#include "environment/CompilationEnvironment.h"

int main(int argc, const char** argv)
{
    // TODO: deal with more arguments
    //Cryo::Assembler::TypeList::clear_custom_types();
    //Cryo::ErrorQueue errors;
    //Cryo::Assembler::Assembler assembler(argv[argc - 1]);

    Cryo::CompilationEnvironment environment;
    return environment.execute_action(argc, argv);

    //assembler.assemble(errors);
    //if (errors.get_severity() != Cryo::Error::level_none)
    {
        //errors.log();
        return -2;
    }
    spdlog::info("Compilation finished successfully!");
}
