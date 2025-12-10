#include "cryopch.h"
#include "VariableStack.h"

namespace Cryo::Assembler {

  bool VariableStack::push_variable(std::string_view name, uint32_t size)
  {
    if (m_Variables.contains(name))
    {
      return false; 
    }

    m_Variables.insert(std::pair(name, VariableData { name, size, m_StackCounter })); 
    m_StackCounter += size;
    m_Stack.push(name);

    if (!m_StackLayers.empty())
    {
      m_StackLayers.top() += 1;
    }

    return true;
  }
  
  bool VariableStack::pop_variable()
  {
    if (m_Variables.empty())
    {
      return false;
    }

    auto ite = m_Variables.find(m_Stack.top());
    m_StackCounter -= ite->second.Size;

    m_Variables.erase(ite);
    m_Stack.pop();

    if (!m_StackLayers.empty())
    {
      if (m_StackLayers.top() == 0)
      {
        return false;
      }
      m_StackLayers.top() -= 1;
    }

    return true;
  }

  void VariableStack::start_stack_layer()
  {
    m_StackLayers.push(0);
  }

  bool VariableStack::end_stack_layer()
  {
    if (m_StackLayers.empty())
    {
      return false;
    }

    m_StackLayers.pop();
    return true;
  }

  const VariableData* VariableStack::get_top()
  {
    if (m_Variables.empty())
    {
      return nullptr;
    }

    return &m_Variables.at(m_Stack.top());
  }

  const VariableData* VariableStack::get_variable(std::string_view name)
  {
    auto ite = m_Variables.find(name);

    if (ite == m_Variables.end())
    {
      return nullptr;
    }

    return &ite->second;
  }

}
