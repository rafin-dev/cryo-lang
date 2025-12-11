#pragma once

#include <unordered_map>
#include <cstdint>
#include <optional>
#include <string_view>

namespace Cryo::Assembler {

  class TypeList
  {
  public:
    static bool add_custom_type(std::string_view type_name, uint32_t type_size);
    static void clear_custom_types();

    static std::optional<uint32_t> get_size_from_type(std::string_view type_name);

  private:
    static std::unordered_map<std::string_view, uint32_t> s_TypeToSize;
  };

}

