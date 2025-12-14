#pragma once

#include <filesystem>
#include <functional>
#include <optional>
#include <string_view>
#include <unordered_map>

namespace Cryo {

  // This was REALLY inspired by Rust's Cargo, I just think they have a nice project structure
  class CompilationEnvironment
  {
  public:
    int execute_action(int argc, const char** argv);

  private:
    int action_new();
    int action_build();
    int action_clean();
    int acttion_run();
    int action_help();

    std::optional<std::filesystem::path> find_workspace();

    int m_Argc = 0;
    const char** m_Argv = nullptr;
    
    static std::unordered_map<std::string_view, std::function<int(CompilationEnvironment*)>> s_ArgToAction;
  };

}
