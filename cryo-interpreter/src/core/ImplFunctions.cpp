#include "cryopch.h"
#include "CryoThread.h"

#include <functional>
#include <stdexcept>

namespace Cryo {
  
  std::unordered_map<std::string, CryoThread::ImplFunction> CryoThread::s_ImplFunctions =
  {
    { "$void::println_str::void*", 
      { CryoFunction{nullptr, 0, "$void::println::void*", 0, { 8 }, nullptr }, std::function<void(CryoThread*)>(&CryoThread::void_println_str_void) } }
  };

  void CryoThread::void_println_str_void()
  {
    const char* str = m_Stack.get_variable<char*>(0);

    if (str == nullptr)
    {
      throw std::logic_error("Fatal Error: str(char*) was null!");
    }

    std::cout << str << std::endl;
  }

}
