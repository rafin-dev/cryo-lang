#pragma once

#include "CryoAssembly.h"
#include "Stack.h"

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
	};

}
