#include "cryopch.h"
#include "CryoThread.h"

#include "CryoInstructions.h"

namespace Cryo {

	CryoThread::CryoThread()
	{
		m_Stack.resize(8 * MB);
		m_StackCounter = m_Stack.size();
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
					m_StackLayers.push(m_StackCounter);
					break;
				}

			case STLE:
				{
					m_StackCounter = m_StackLayers.top();
					m_StackLayers.pop();
					break;
				}

			case PUSH:
				{
					m_ProgramCounter++; // Advance to the push size
					uint32_t size = *m_ProgramCounter;
					if (m_StackCounter < size)
					{
						// TODO: Exceptions
						std::cout << "Stack overflow exception!" << std::endl;
						return;
					}
					m_StackCounter -= size;
          m_StackEntries.push(size);

					std::cout << "Pushing [" << size << "] into the stack!" << std::endl;

					break;
				}

      case POP:
        {
          m_ProgramCounter++;

          uint32_t count = *m_ProgramCounter;
          std::cout << "Poping [" << count << "] entries from the stack!" << std::endl;
          for (uint32_t i = 0; i < count; i++)
          {
            m_StackCounter += m_StackEntries.top();
            m_StackEntries.pop();
          }
        }

			case RETURN:
				{
					std::cout << "Returning from [" << m_CurrentFunction->FunctionSignature << "]!" << std::endl;

					if (m_CallStack.empty()) // Return from call stack root
					{
						clear();
						return;
					}

					// TODO: implement dealing with parameters
					CallStackEntry entry = m_CallStack.top();
					m_CallStack.pop();

					m_ProgramCounter = entry.PC;
					m_CurrentFunction = entry.Func;

					break;
				}

			case CALL_from_assembly_signature:
				{
					m_ProgramCounter++;
					uint32_t signature_index = *m_ProgramCounter;

					auto result = m_CurrentFunction->OwnerAssembly->get_string_literal(signature_index);
					if (!result.has_value())
					{
						std::cout << "Fatal Error: Invalid CryoAssembly, atempt by [" << m_CurrentFunction->FunctionSignature << "] to call non existent function!" << std::endl;
						return;
					}

					const CryoFunction* function = m_CurrentFunction->OwnerAssembly->get_function_by_signature(std::string(result.value()));
					if (!function) // Function not found, invalid assembly
					{
						std::cout << "Fatal Error: Invalid CryoAssembly, atempt by [" << m_CurrentFunction->FunctionSignature << "] to call non existent function!" << std::endl;
						clear();
						return;
					}

					m_CallStack.push(CallStackEntry{ m_CurrentFunction, m_ProgramCounter });
					m_CurrentFunction = function;
					m_ProgramCounter = function->FunctionStart - 1; // Account for the m_ProgramCounter++ before the next loop iteration

					std::cout << "Calling [" << function->FunctionSignature << "]!" << std::endl;

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
		std::cout << "Fatal Error: Invalid CryoAssembly, function [" << func->FunctionSignature << "] lacked a RETURN instrcution!";
		clear();
	}

	void CryoThread::clear()
	{
		m_ProgramCounter = nullptr;
		m_CurrentFunction = nullptr;
		m_StackCounter = m_Stack.size();
		while (!m_StackLayers.empty()) { m_StackLayers.pop(); }
		while (!m_CallStack.empty()) { m_CallStack.pop(); }
	}

}
