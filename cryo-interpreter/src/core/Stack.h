#pragma once

#include "CryoAssembly.h"

#include <cstdint>
#include <vector>
#include <stack>

namespace Cryo {

  struct CallStackEntry
  {
    CallStackEntry() = default;
    CallStackEntry(const CryoFunction* func, const uint32_t* pc, uint32_t stack_start)
      : Function(func), ProgramCounter(pc), StackLayerCount(0), FunctsionStackStart(stack_start)
    {}

    const CryoFunction* Function = nullptr;
    const uint32_t* ProgramCounter = nullptr;
  
  private:
    uint32_t StackLayerCount = 0;
    uint32_t FunctsionStackStart = 0;

    friend class Stack;
  };

  class Stack
  {
  public:
    Stack(uint32_t stack_size_mb);

    bool push_variable(uint32_t size);
    bool pop_variable(uint32_t count);

    void start_stack_layer();
    bool end_stack_layer();

    void push_call_stack(const CryoFunction* func, const CryoFunction* calee, const uint32_t* pc);
    CallStackEntry pop_call_stack();

    template <typename T>
    T& get_variable(uint32_t stack_index)
    {
      if (!m_CallStack.empty())
      {
        stack_index += m_CallStack.top().FunctsionStackStart;
      }

      T* ptr = (T*)&m_StackBuffer[stack_index];
      return *ptr;
    }

    void clear();
    
  private:
    // Variables
    std::vector<uint8_t> m_StackBuffer;
    uint32_t m_StackCounter = 0;
    std::vector<uint32_t> m_StackEntries;
    std::stack<uint32_t> m_StackLayers;
  
    std::stack<CallStackEntry> m_CallStack; 
  };

}
