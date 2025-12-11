#include "cryopch.h"
#include <optional>
#include "TypeList.h"

namespace Cryo::Assembler {

  std::unordered_map<std::string_view, uint32_t> TypeList::s_TypeToSize;

  void TypeList::clear_custom_types()
  {
    s_TypeToSize = 
    {
      { "@void",     0 },
      
      { "@void*",    8 },
      
      { "@uint8",    1 },
      { "@uint16",   2 },
      { "@uint32",   4 },
      { "@uint64",   8 },

      { "@int8",     1 },
      { "@int16",    2 },
      { "@int32",    4 },
      { "@int64",    8 },

      { "@float32",  4 },
      { "@float64",  8 }
    };
  }

  bool TypeList::add_custom_type(std::string_view type_name, uint32_t type_size)
  {
    if (s_TypeToSize.contains(type_name))
    {
      return false;
    }

    s_TypeToSize.insert(std::pair(type_name, type_size));
    return true;
  }

  std::optional<uint32_t> TypeList::get_size_from_type(std::string_view type_name)
  {
    if (!s_TypeToSize.contains(type_name))
    {
      return std::nullopt;
    }

    return s_TypeToSize[type_name];
  }

}
