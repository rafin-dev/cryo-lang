#pragma once

#include "common/Error.h"
#include "assembler/Assembler.h"

#include <filesystem>
#include <unordered_set>
#include <unordered_map>
#include <string>

namespace Cryo::Linker {

  class Linker
  {
  public:
    Linker();

    ErrorQueue link_project(const std::filesystem::path& prj_int_dir);
    ErrorQueue link_dependencies(const std::filesystem::path& dest, const std::filesystem::path& src);

  private:
    void parse_file(const std::filesystem::path& file_path, ErrorQueue& errors);

    std::unordered_set<std::string> m_StringLiterals;
    std::unordered_map<std::filesystem::path, std::unordered_map<uint32_t, uint32_t>> m_OldStrIndexToNewStrIndex;

    std::unordered_map<std::filesystem::path, std::unordered_map<std::string, Assembler::Function>> m_Functions;
  };

}
