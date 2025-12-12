#include "cryopch.h"
#include "CryoThread.h"

#include "CryoInstructions.h"
#include <cstdint>
#include <iostream>
#include <iterator>
#include <ostream>
#include <stdexcept>
#include <format>
#include <string_view>
#include <strings.h>

namespace Cryo {

	CryoThread::CryoThread()
	  : m_Stack(8)
  {
	}

	void CryoThread::execute(const CryoFunction* func)
	{
		m_CurrentFunction = func;
		for (m_ProgramCounter = m_CurrentFunction->FunctionStart; (m_ProgramCounter - m_CurrentFunction->FunctionStart) < m_CurrentFunction->InstrutionCount; m_ProgramCounter++)
		{
			CryoOpcode opcode = (CryoOpcode)*m_ProgramCounter;
			switch (opcode)
			{
			case STLS:
				{
					m_Stack.start_stack_layer();
					break;
				}

			case STLE:
				{
          if (!m_Stack.end_stack_layer())
          {
            throw std::logic_error(std::format("Fatal Error: Invalid CryoAssembly, atmept by [{}] to end non existent stack layer!", m_CurrentFunction->FunctionSignature));
          }

					break;
				}

			case PUSH:
				{
					m_ProgramCounter++; // Advance to the push size
					uint32_t size = *m_ProgramCounter;
					if (!m_Stack.push_variable(size))
					{
						// TODO: CryoExceptions
						std::cout << "Stack overflow exception!" << std::endl;
						return;
					}

					break;
				}

      case POP:
        {
          m_ProgramCounter++;

          uint32_t count = *m_ProgramCounter;
          if (!m_Stack.pop_variable(count))
          {
            throw std::logic_error(std::format("Fatal Error: Invalid CryoAssembly, atempt by [{}] to pop non existent variable!", m_CurrentFunction->FunctionSignature));
          }

          break;
        }

      case SETU32:
        {
          m_ProgramCounter++;
          uint32_t index = *m_ProgramCounter;
          m_ProgramCounter++;
          uint32_t value = *m_ProgramCounter;

          m_Stack.get_variable<uint32_t>(index) = value;

          break;
        }

      case SETSTR:
        {
          m_ProgramCounter++;
          uint32_t var_index = *m_ProgramCounter;
          m_ProgramCounter++;
          uint32_t str_index = *m_ProgramCounter;

          auto result = m_CurrentFunction->OwnerAssembly->get_string_literal(str_index);
          if (!result.has_value())
          {
            throw std::logic_error("Fatal Error: Invalid String literal!");
          }

          m_Stack.get_variable<const char*>(var_index) = result.value().data();

          break;
        }

			case RETURN:
				{
          CallStackEntry call_stack_entry = m_Stack.pop_call_stack();
					if (call_stack_entry.Function == nullptr) // Return from call stack root
					{
						clear();
						return;
					}

					// TODO: implement dealing with parameters
          m_CurrentFunction = call_stack_entry.Function;
          m_ProgramCounter = call_stack_entry.ProgramCounter;

					break;
				}

			case CALL_from_assembly_signature:
				{
					m_ProgramCounter++;
					uint32_t signature_index = *m_ProgramCounter;

					auto result = m_CurrentFunction->OwnerAssembly->get_string_literal(signature_index);
					if (!result.has_value())
					{
					  throw std::logic_error(std::format("Fatal Error: Invalid CryoAssembly, atempt by [{}] to call non existent function!", 
                  m_CurrentFunction->FunctionSignature));
          }

					const CryoFunction* function = m_CurrentFunction->OwnerAssembly->get_function_by_signature(std::string(result.value()));
				if (!function) // Function not found, invalid assembly
					{
					  throw new std::logic_error(std::format("Fatal Error: Invalid CryoAssembly, atempt to call invalid function [{}]!", result.value()));
          }

					m_Stack.push_call_stack(m_CurrentFunction, function, m_ProgramCounter);
					m_CurrentFunction = function;
					m_ProgramCounter = function->FunctionStart - 1; // Account for the m_ProgramCounter++ before the next loop iteration

					break;
				}

      case IMPL:
        {
          m_ProgramCounter++;
          uint32_t sig_index = *m_ProgramCounter;
          auto signature = m_CurrentFunction->OwnerAssembly->get_string_literal(sig_index);
          if (!signature.has_value())
          {
            throw std::logic_error(std::format("Fatal Error: Invalid CryoAssembly, atempt by [{}] to call non existent IMPL function!", 
                  m_CurrentFunction->FunctionSignature));
          }

          auto ite = s_ImplFunctions.find(std::string(signature.value()));
          if (ite == s_ImplFunctions.end())
          {
            throw std::logic_error(std::format("Fatal Error: IMPL function [{}] does not exist!", signature.value()));
          }
          m_Stack.push_call_stack(m_CurrentFunction, &ite->second.FunctionData, m_ProgramCounter);
          ite->second.Function(this);
          m_Stack.pop_call_stack();

          break;
        }

			default:
				{	
					std::cout << "Fatal Error: Unknown instruction: [" << std::hex << opcode << "]!" << std::endl;
					break;
				}
			}
		}

		// If this code is reached, the function lacked a return statement, quit invalid assembly
		std::cout << "Fatal Error: Invalid CryoAssembly, function [" << func->FunctionSignature << "] lacked a RETURN instrcution!" << std::endl;
		clear();
	}

	void CryoThread::clear()
	{
		m_ProgramCounter = nullptr;
		m_CurrentFunction = nullptr;
	  m_Stack.clear();
  }

}
