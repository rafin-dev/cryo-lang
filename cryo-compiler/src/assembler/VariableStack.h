#pragma once

#include "cryopch.h"

#include <stack>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Cryo::Assembler {

  struct VariableData
  {
    std::string_view Name;
    uint32_t Size = 0;
    uint32_t Position = 0;
  };

  class VariableStack
  {
  public:
    bool push_variable(std::string_view name, uint32_t size);
    bool pop_variable();

    void start_stack_layer();
    bool end_stack_layer();

    const VariableData* get_top();

    const VariableData* get_variable(std::string_view name);

  private:
    std::unordered_map<std::string_view, VariableData> m_Variables;
    std::stack<std::string_view> m_Stack;
    // Stack with the count of variables in each layer
    std::stack<uint32_t> m_StackLayers;
    uint32_t m_StackCounter = 0;
  };

}
