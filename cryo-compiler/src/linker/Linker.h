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
    ErrorQueue link_project(const std::filesystem::path& prj_int_dir, const std::filesystem::path& prj_bin_dir);
    ErrorQueue link_dependencies(const std::filesystem::path& dest, const std::filesystem::path& src);

  private:
    void parse_file(const std::filesystem::path& file_path, ErrorQueue& errors);
    void remap_ids(ErrorQueue& errors);
    void remap_func(Assembler::Function& func, const std::filesystem::path& file, ErrorQueue& errors);
    void serialize(const std::filesystem::path& output, ErrorQueue& errors);

    std::unordered_set<std::string> m_StringLiterals;
    std::unordered_map<std::filesystem::path, std::unordered_map<std::string, uint32_t>> m_OldIndex;
    std::unordered_map<std::filesystem::path, std::vector<std::string>> m_OldStringLists;
    std::unordered_map<std::filesystem::path, std::unordered_map<uint32_t, uint32_t>> m_OldStrIndexToNewStrIndex;

    std::unordered_map<std::filesystem::path, std::unordered_map<std::string, Assembler::Function>> m_Functions;
  };

}
