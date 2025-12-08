#pragma once

#include "CryoAssembly.h"

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

		uint32_t* m_ProgramCounter = nullptr;
		const CryoFunction* m_CurrentFunction = nullptr;

		std::vector<uint8_t> m_Stack;
		uint32_t m_StackCounter = 0;
    std::stack<uint32_t> m_StackEntries;
    std::stack<uint32_t> m_StackLayers;

		struct CallStackEntry
		{
			const CryoFunction* Func = nullptr;
			uint32_t* PC;
		};
		std::stack<CallStackEntry> m_CallStack;
	};

}
