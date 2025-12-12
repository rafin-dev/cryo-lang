#include "cryopch.h"
#include "CryoThread.h"

#include <functional>
#include <stdexcept>

namespace Cryo {
  
  std::unordered_map<std::string, CryoThread::ImplFunction> CryoThread::s_ImplFunctions =
  {
    { "$void::println_str::uint32", 
      { CryoFunction{nullptr, 0, "$void::println::uint32", 0, { 4 }, nullptr }, std::function<void(CryoThread*)>(&CryoThread::void_println_str_void) } }
  };

  void CryoThread::void_println_str_void()
  {
    auto str = m_CurrentFunction->OwnerAssembly->get_string_literal(m_Stack.get_variable<uint32_t>(0));
    if (!str.has_value())
    {
      throw std::logic_error("Fatal Error: Invalid string literal!");
    }

    std::cout << str.value() << std::endl;
  }

}
