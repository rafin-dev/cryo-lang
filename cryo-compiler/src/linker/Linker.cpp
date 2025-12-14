#include "common/Error.h"
#include "cryopch.h"
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include "Linker.h"

namespace Cryo::Linker {

  ErrorQueue Linker::link_project(const std::filesystem::path& prj_int_dir)
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

    return errors;
  }

  void Linker::parse_file(const std::filesystem::path& file_path, ErrorQueue& errors)
  {
    std::ifstream file_stream(file_path, std::ios::binary | std::ios::ate);
    
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
                                                       
        func.Instructions.resize(file_buffer[file_as_u32[i + 2]]); // Instruction count is the third
        std::memcpy(func.Instructions.data(), file_buffer.data() + file_as_u32[i + 1], func.Instructions.size()); // First instruction is the second

        func.ReturnSize = file_as_u32[i + 3]; // ReturnSize is the fourth
        for (i += 4; file_as_u32[i] != block_end; i++) // Array of return sizes
        {
          func.ParametersSizes.emplace_back(file_as_u32[i]);
        }

        functions.insert(std::pair(func.Signature, func));
      }
    }

    m_OldStrIndexToNewStrIndex.insert(std::pair(file_path, std::unordered_map<uint32_t, uint32_t>()));
    for (int i = 0; i < string_literals.size(); i++)
    {
      m_StringLiterals.insert(string_literals[i]);

      uint32_t new_index = 0;
      for (auto& str : m_StringLiterals)
      {
        if (str == string_literals[i])
        {break;}
        new_index++;
      }
      
      m_OldStrIndexToNewStrIndex[file_path].insert(std::pair(i, new_index));
    }
    m_Functions.insert(std::pair(file_path, functions));
  }

}
