#include "cryopch.h"
#include "CompilationEnvironment.h"

#include "common/Error.h"
#include "assembler/TypeList.h"
#include "assembler/Assembler.h"

#include <chrono>
#include <future>
#include <list>
#include <optional>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <functional>
#include <iostream>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Cryo {

  std::unordered_map<std::string_view, std::function<int(CompilationEnvironment*)>> CompilationEnvironment::s_ArgToAction = 
  {
    { "new",    &CompilationEnvironment::action_new     },
    { "build",  &CompilationEnvironment::action_build   },
    { "clean",  &CompilationEnvironment::action_clean   },
    { "run",    &CompilationEnvironment::acttion_run    },
    { "help",   &CompilationEnvironment::action_help    }
  };

  int CompilationEnvironment::execute_action(int argc, const char** argv)
  {
    m_Argc = argc;
    m_Argv = argv;

    spdlog::set_pattern("[%H:%M:%S|%z|%l] %v");
    if (m_Argc < 2)
    {
      spdlog::critical("Too few arguments!");
      return -1;
    }

    auto ite = s_ArgToAction.find(m_Argv[1]);
    if (ite == s_ArgToAction.end())
    {
      spdlog::critical("Unknown Action: {0}", argv[1]);
      std::cout << "Use 'help' for a list of Actions!" << std::endl;
      return -1;
    }

    return ite->second(this);
  }

  int CompilationEnvironment::action_new()
  {
    if (m_Argc != 3)
    {
      spdlog::critical("Action 'new' missing folder parameter!");
      return -1;
    }

    std::filesystem::path prj_folder(m_Argv[2]);
    if (std::filesystem::exists(prj_folder))
    {
      if (std::filesystem::is_directory(prj_folder))
      {
        if (!std::filesystem::is_empty(prj_folder))
        {
          spdlog::critical("Directory {0} exists and is not empty!", prj_folder.string());
          return -1;
        }
      }
      else
      {
        spdlog::critical("{0} alredy exists and is not a directory!", prj_folder.string());
        return -1;
      }
    }
    else
    {
      std::filesystem::create_directory(prj_folder);
    }

    std::filesystem::create_directory(prj_folder / "src");

    return 0;
  }

  ErrorQueue assemble_file(const std::filesystem::path& file)
  {
    ErrorQueue errors;

    Assembler::Assembler assembler = Assembler::Assembler(file);
    assembler.assemble(errors);

    return errors;
  }

  int CompilationEnvironment::action_build()
  {
    // Look for workspace root
    std::filesystem::path wks_dir;
    {
      auto result = find_workspace();
      if (!result.has_value())
      {
        spdlog::critical("Current directory is not inside of a Cryo workspace!");
        return -1;
      }
      wks_dir = result.value();
    }

    if (!std::filesystem::exists(wks_dir / "bin/int"))
    {
      std::filesystem::create_directories(wks_dir / "bin/int");
    }

    spdlog::info("Building...");

    Assembler::TypeList::clear_custom_types();

    ErrorQueue errors;
    // TODO: Compiler
    
    // Assembler
    std::vector<std::future<ErrorQueue>> results(std::thread::hardware_concurrency());
    spdlog::info("Starting compilation with {0} cores!", results.size());

    std::queue<std::filesystem::path> files;
    for (auto entry : std::filesystem::recursive_directory_iterator(wks_dir / "bin/int"))
    {
      if (entry.is_directory() || entry.path().extension() != ".crya")
      {
        continue;
      }
      files.push(entry.path());
    }
    for (int i = 0, done_count = 0; !files.empty(); i++)
    {
      auto& thread = results[i];
      if (!thread.valid())
      {
        std::filesystem::path file = files.front();
        spdlog::info("Assembling {0}", file.string());
        thread = std::async(&assemble_file, file);
        files.pop();
      }
      else 
      {
        std::future_status status = thread.wait_for(std::chrono::seconds(0));
        if (status == std::future_status::ready)
        {
          ErrorQueue queue = thread.get();
          errors.merge(queue);
        }
      }
      
      i = i % (results.size() - 1);
    }
    for (auto& thread : results)
    {
      if (thread.valid())
      {
        ErrorQueue queue = thread.get();
        errors.merge(queue);
      }
    }
    if (errors.get_severity() == Error::level_critical)
    {
      return -1;
    }

    spdlog::info("Linking...");
    // TODO: Linker

    errors.log();
    if (errors.get_severity() < Error::level_error)
    {
      spdlog::info("Compilation finished successfully!");
    }

    return 0;
  }

  int CompilationEnvironment::action_clean()
  {
    return 0;
  }

  int CompilationEnvironment::acttion_run()
  {
    return 0;
  }
 
  std::optional<std::filesystem::path> CompilationEnvironment::find_workspace()
  {
    std::filesystem::path wks_dir = std::filesystem::current_path();
    while (!std::filesystem::exists(wks_dir / "cryo.toml"))
    {
      if (!wks_dir.has_parent_path())
      {
        return std::nullopt;
      }
      wks_dir = wks_dir.parent_path();
    }

    return wks_dir;
  }

  std::unordered_map<std::string_view, std::string_view> s_ActionExplanations = 
  {
    { "new",    "new {folder} | Creates a new workspace at {folder} with a start project named {folder}!" },
    { "build",  "build {configuration} | Compiles workspace at the current folder, default configuration is Debug!" },
    { "clean",  "clean | Cleans compilation remaints!" },
    { "run",    "run {args...} | build and run the 'startup project' in the current workspace with {args...} as command line arguments!"},
    { "quit",   "" }
  };

  int CompilationEnvironment::action_help()
  {
    std::cout << "===============" << std::endl;
    std::cout << "==Action list==" << std::endl;
    std::cout << "===============" << std::endl;
    std::cout << "-new" << std::endl;
    std::cout << "-build" << std::endl;
    std::cout << "-clean" << std::endl;
    std::cout << "-run" << std::endl;
    std::cout << "===============" << std::endl;
    std::cout << "Type any action for an explanation, type 'quit' to quit." << std::endl;
    std::cout << "===============" << std::endl;

    std::string user_choice;
    while (user_choice != "quit")
    {
      user_choice = std::string();
      std::cin >> user_choice;
      
      auto ite = s_ActionExplanations.find(user_choice);
      if (ite == s_ActionExplanations.end())
      {
        std::cout << "Unknown Action!" << std::endl;
        std::cout << "===============" << std::endl;
        continue;
      }
      std::cout << ite->second << std::endl;
      std::cout << "===============" << std::endl;
    }

    return 0;
  }

}
