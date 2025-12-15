#include "assembler/Instructions.h"
#include "cryopch.h"
#include "Linker.h"

#include "common/Error.h"

#include <filesystem>
#include <spdlog/spdlog.h>

#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <iterator>
#include <limits>

namespace Cryo::Linker {

  ErrorQueue Linker::link_project(const std::filesystem::path& prj_int_dir, const std::filesystem::path& prj_bin_dir)
  {
    ErrorQueue errors;
   
    for (auto& entry : std::filesystem::recursive_directory_iterator(prj_int_dir))
    {
      if (entry.is_directory() || entry.path().extension() != ".cryi")
      {
        continue;
      }

      parse_file(entry.path(), errors);
      if (errors.get_severity() == Error::level_critical)
      {
        return errors;
      }
    }
    if (errors.get_severity() > Error::level_warning)
    {
      return errors;
    }

    remap_ids(errors);
    if (errors.get_severity() > Error::level_warning) { return errors; }

    serialize(prj_bin_dir / "main.crye", errors);

    return errors;
  }

  void Linker::parse_file(const std::filesystem::path& file_path, ErrorQueue& errors)
  {
    std::ifstream file_stream(file_path, std::ios::binary | std::ios::in);
  
    std::vector<uint32_t> file_buffer(std::filesystem::file_size(file_path));
    if (!file_stream.read((char*)file_buffer.data(), file_buffer.size()))
    {
      errors.push_error(ERR_L_UNABLE_TO_OPEN_FILE, file_path);
      return;
    }
    file_stream.close();

    std::vector<std::string> string_literals;
    std::unordered_map<std::string, Assembler::Function> functions;

    const char* file_as_char = (const char*)file_buffer.data();
    const uint32_t* file_as_u32 = file_buffer.data();

    const char* expected_header = "CRYOINT";
    for (uint32_t i = 0; i < 8; i++)
    {
      if (file_as_char[i] != expected_header[i])
      {
        errors.push_error(ERR_L_UNABLE_TO_VALIDATE_HEADER, file_path);
      }
    }

    constexpr uint32_t block_end = std::numeric_limits<uint32_t>::max();
    
    // String literals
    {
      file_as_char += 8; // After header, find all string literals
      uint32_t strings_size = 0;
      for (file_as_u32++; file_as_u32 < file_buffer.data() + file_buffer.size(); file_as_u32++)
      {
        if (*file_as_u32 == block_end)
        {
          break;
        }
        strings_size++;
      }
      file_as_u32++; // Place it right after the strings
      strings_size *= sizeof(uint32_t);
    
      bool last_was_null = false;
      const char* current_string_start = file_as_char;
      uint32_t current_string_size = 0;
      for (int i = 0; i < strings_size; i++)
      {
        if (file_as_char[i] != '\0')
        {
          current_string_size++;
          last_was_null = false;
        }
        else
        {
          if (last_was_null) { break; }
          last_was_null = true;

          string_literals.emplace_back(std::string(current_string_start, current_string_size));
          current_string_start = file_as_char + i + 1;
          current_string_size = 0;
        }
      }
    }
    
    // Functions
    {
      bool last_was_end = false;
      uint32_t funcs_size = 0;
      for (const uint32_t* ptr = file_as_u32; ptr < file_buffer.data() + file_buffer.size(); ptr++) // Look for 2 block_end in a row
      {
        if (*ptr == block_end)
        {
          if (last_was_end)
          {
            break;
          }
          last_was_end = true;
        }
        else
        {
          last_was_end = false;
          funcs_size++;
        }
      }

      for (uint32_t i = 0; i < funcs_size; i++)
      {
        Assembler::Function func;
        func.Signature = string_literals[file_as_u32[i]]; // String index is the first thing in the func declaration
        
        func.Instructions.resize(file_as_u32[i + 2]); // Instruction count is the third
        std::memcpy(func.Instructions.data(), file_buffer.data() + file_as_u32[i + 1], func.Instructions.size() * sizeof(uint32_t)); // First instruction is the second

        func.ReturnSize = file_as_u32[i + 3]; // ReturnSize is the fourth
        for (i += 4; file_as_u32[i] != block_end; i++) // Array of return sizes
        {
          func.ParametersSizes.emplace_back(file_as_u32[i]);
        }

        functions.insert(std::pair(func.Signature, func));
      }
    }

    m_OldIndex.insert(std::pair(file_path, std::unordered_map<std::string, uint32_t>()));
    m_OldStringLists.insert(std::pair(file_path, string_literals));
    m_OldStrIndexToNewStrIndex.insert(std::pair(file_path, std::unordered_map<uint32_t, uint32_t>()));
    for (int i = 0; i < string_literals.size(); i++)
    {
      m_StringLiterals.insert(string_literals[i]);
    }
    for (int i = 0; i < string_literals.size(); i++)
    {
      m_OldIndex[file_path].insert(std::pair(string_literals[i], i));
      m_OldStrIndexToNewStrIndex[file_path].insert(std::pair(i, 0));
    }
    m_Functions.insert(std::pair(file_path, functions));
  }

  void Linker::remap_ids(ErrorQueue& errors)
  {
    for (auto& assembly : m_OldStrIndexToNewStrIndex)
    {
      for (auto& ite : assembly.second)
      {
        const std::string& str = m_OldStringLists[assembly.first].at(ite.first);
        uint32_t new_index = 0;
        for (auto& n_str : m_StringLiterals)
        {
          if (str == n_str)
          {
            break;
          }
          new_index++;
        }
        ite.second = new_index;
      }
    }

    for (auto& assembly : m_Functions)
    {
      for (auto& func : assembly.second)
      {
        remap_func(func.second, assembly.first, errors);
        if (errors.get_severity() == Error::level_critical)
        {
          return;
        }
      }
    }
  }

  void Linker::remap_func(Assembler::Function& func, const std::filesystem::path& file, ErrorQueue& error)
  {
    for (uint32_t i = 0; i < func.Instructions.size(); i++)
    {
      switch (func.Instructions[i])
      {
        case Assembler::PUSH:
          i++;
          break;

        case Assembler::SETSTR:
          i += 2;
         
          func.Instructions[i] = m_OldStrIndexToNewStrIndex[file].at(func.Instructions[i]);

          break;

        case Assembler::CALL_from_assembly_signature:
          i++;
          func.Instructions[i] = m_OldStrIndexToNewStrIndex[file].at(func.Instructions[i]);

          break;

        case Assembler::IMPL:
          i++;
          func.Instructions[i] = m_OldStrIndexToNewStrIndex[file].at(func.Instructions[i]);
          break;

        default:
          uint32_t instruction_parameters_size = 0;// TODO: get actual size
          i += instruction_parameters_size;
      }
    }
  }

#define WRITE_BINARY(stream, x) stream.write(reinterpret_cast<const char*>(&x), sizeof(x)) 

  void Linker::serialize(const std::filesystem::path& output, ErrorQueue& errors)
  {
    std::ofstream file_stream(output);
    
    constexpr uint32_t block_end = std::numeric_limits<uint32_t>::max();

    constexpr const char* header_string = "CRYOEXE";

    file_stream.write(header_string, 8);

    uint32_t strings_size = 0;
    for (auto& str : m_StringLiterals)
    {
      file_stream << str << '\0';
      strings_size += str.size() + 1;
    }
    for (uint32_t spacing = sizeof(uint32_t) - (strings_size % sizeof(uint32_t)); spacing > 0; spacing--)
    {
      file_stream << '\0';
    }

    WRITE_BINARY(file_stream, block_end);
    
    std::unordered_map<std::string, std::streamoff> function_indexes;
    for (auto assembly : m_Functions)
    {
      for (auto func : assembly.second)
      {
        uint32_t signature_index = m_OldStrIndexToNewStrIndex.at(assembly.first).at(m_OldIndex[assembly.first].at(func.second.Signature));
        WRITE_BINARY(file_stream, signature_index);

        function_indexes.insert(std::pair(func.second.Signature, file_stream.tellp()));
        constexpr uint32_t placeholder = 0;
        WRITE_BINARY(file_stream, placeholder);

        uint32_t size = func.second.Instructions.size();
        WRITE_BINARY(file_stream, size);

        WRITE_BINARY(file_stream, func.second.ReturnSize);

        for (auto param : func.second.ParametersSizes)
        {
          WRITE_BINARY(file_stream, param);
        }

        WRITE_BINARY(file_stream, block_end);
      }
    }
    WRITE_BINARY(file_stream, block_end);

    for (auto& assembly : m_Functions)
    {
      for (auto& func : assembly.second)
      {
        std::streamoff pos = file_stream.tellp();
        file_stream.seekp(function_indexes[func.second.Signature]);

        uint32_t func_index = pos / sizeof(uint32_t);
        WRITE_BINARY(file_stream, func_index);

        file_stream.seekp(pos);
        file_stream.write(reinterpret_cast<const char*>(func.second.Instructions.data()), func.second.Instructions.size() * sizeof(uint32_t));
        WRITE_BINARY(file_stream, block_end);
      }
    }
  }

}
