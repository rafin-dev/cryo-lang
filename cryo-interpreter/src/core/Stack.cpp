#include "core/CryoAssembly.h"
#include "cryopch.h"
#include "Stack.h"

#include <cstdint>
#include <exception>
#include <stdexcept>

#define MB 1000000

namespace Cryo {

  Stack::Stack(uint32_t stack_Size_mb)
    : m_StackBuffer(stack_Size_mb * MB)
  {
  }

  bool Stack::push_variable(uint32_t size)
  {
    m_StackCounter += size;
    if (m_StackCounter >= m_StackBuffer.size())
    {
      return false;
    }

    m_StackEntries.emplace_back(size);

    if (!m_StackLayers.empty())
    {
      m_StackLayers.top() += 1;
    }

    return true;
  }

  bool Stack::pop_variable(uint32_t count)
  {
    for (uint32_t i = 0; i < count; i++)
    {
      if (m_StackEntries.empty() || (!m_StackLayers.empty() && m_StackLayers.top() == 0)) // Ensure there's a valid variable to be poped
      {
        return false;
      }
      
      m_StackCounter -= m_StackEntries.back();
      m_StackEntries.erase(std::prev(m_StackEntries.end()));
      if (!m_StackLayers.empty())
      {
        m_StackLayers.top() -= 1;
      }
    }
    
    return true; 
  }

  void Stack::start_stack_layer()
  {
    m_StackLayers.push(0);
    if (!m_CallStack.empty())
    {
      m_CallStack.top().StackLayerCount += 1;
    }
  }

  bool Stack::end_stack_layer()
  {
    if (m_StackLayers.empty() || m_CallStack.top().StackLayerCount == 0)
    {
      return false;
    }

    pop_variable(m_StackLayers.top());
    m_StackLayers.pop();
    
    if (!m_CallStack.empty())
    {
      m_CallStack.top().StackLayerCount -= 1;
    }

    return true;
  }

  void Stack::push_call_stack(const CryoFunction* func, const CryoFunction* calee, const uint32_t* pc)
  {
    uint32_t func_stack_offset = 0;
    for (uint32_t i = 0; i < calee->ParameterSizes.size(); i++)
    {
      uint32_t param_size = calee->ParameterSizes[calee->ParameterSizes.size() - 1 - i]; // Reverse iterate
      if (m_StackEntries[m_StackEntries.size() - 1 - i] != param_size)
      {
        throw std::logic_error("Parameters used do not match function declaration!");
      }
      func_stack_offset += param_size;
    }
    if (calee->ReturnTypeSize != 0)
    {
      if (m_StackEntries[m_StackEntries.size() - 1 - calee->ParameterSizes.size()] != calee->ReturnTypeSize)
      {
        throw std::logic_error("Unhandled function return!");
      }
      func_stack_offset += calee->ReturnTypeSize;
    }

    m_CallStack.push(CallStackEntry(func, pc, m_StackCounter - func_stack_offset));
    start_stack_layer(); // Function Layer
  }

  CallStackEntry Stack::pop_call_stack()
  {
    if (m_CallStack.empty())
    {
      return CallStackEntry();
    }

    CallStackEntry entry = m_CallStack.top();
    for (uint32_t i = 0; i < entry.StackLayerCount; i++)
    {
      end_stack_layer(); // Clear all uncleared layers in the function
    }
    end_stack_layer(); // Function layer

    m_CallStack.pop();
    return entry;
  }

  void Stack::clear()
  {
    m_StackCounter = 0;
    m_StackEntries.clear();
    m_StackLayers = std::stack<uint32_t>();
    m_CallStack = std::stack<CallStackEntry>();
  }
}
