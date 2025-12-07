#pragma once

#include "CryoAssembly.h"
#include "CryoThread.h"

#include <vector>

namespace Cryo {

	class CryoState
	{
	public:
		CryoState(int argc, const char* argv[]);
		
		/// <summary>
		///  Used to check if the state was able to load properly
		/// </summary>
		/// <returns> Returns [true] if valid, [false] if not </returns>
		bool is_valid() { return m_Assemblies.size() != 0; }

		void run_entry_point();

	private:
		CryoThread m_MainThread;
		std::vector<CryoAssembly> m_Assemblies;

		const int m_Argc = 0;
		const char** m_Argv = nullptr;
	};

}