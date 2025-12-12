#pragma once

#include "CryoAssembly.h"
#include "Stack.h"

#include <unordered_map>
#include <functional>
#include <vector>
#include <stack>

#define MB 1000000

namespace Cryo {

	class CryoThread
	{
	public:
		CryoThread();

		void execute(const CryoFunction* func);

	private:
		void clear();

		const uint32_t* m_ProgramCounter = nullptr;
		const CryoFunction* m_CurrentFunction = nullptr;

    Stack m_Stack;

    struct ImplFunction
    {
      CryoFunction FunctionData;
      std::function<void(CryoThread*)> Function;
    };

    static std::unordered_map<std::string, ImplFunction> s_ImplFunctions;

    void void_println_str_void();
	};

}
