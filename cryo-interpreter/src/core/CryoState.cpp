#include "cryopch.h"
#include "CryoState.h"
#include <string.h>

namespace Cryo {

	/// <summary>
	/// Constructor for a CryoState, expects the caller to have filtered out the inital argument with the program path
	/// </summary>
	/// <param name="argc"> argument count </param>
	/// <param name="argv"> argument values </param>
	CryoState::CryoState(int argc, const char* argv[])
		: m_Argc(argc), m_Argv(argv)
	{
		for (int i = 0; i < m_Argc; i++)
		{
			const char* arg = m_Argv[i];
			if (arg[0] == '-')
			{
				size_t arg_lenght = strlen(arg);
				std::set<char> used_modifiers;
				// Allow multiple modifiers in one argument like "-abcd" instead of "-a -b -c -d"
				for (int c = 1; c < arg_lenght; c++) // c starts at 1 to ignore the '-' character
				{
					if (arg[c] == '-' || used_modifiers.find(arg[c]) != used_modifiers.end()) // Pass if it finds another '-' to allow for arguments like "--abcd"
					{
						continue;
					}

					// Deal with the modifier argument
					switch (arg[c])
					{
					default:
						std::cout << "unknown modifier argument: " << arg[c] << std::endl; // Unknown modifier found, quit
						return;
					}

					used_modifiers.insert(arg[c]);
				}
			}
			else // the first argument thaat doesn't start with '-' and wasn't dealt with by the modifiers is a CryoAssembly filepath
			{
				m_Assemblies.emplace_back(std::filesystem::path(argv[i]));
				if (!m_Assemblies[0].is_valid()) // Failing to load the initial assembly is a critical failure, clear m_Assemblies to indicate that the state is not valid
				{
					m_Assemblies.clear();
				}
			}
		}
	}

	void CryoState::run_entry_point()
	{
		auto func = m_Assemblies[0].get_function_by_signature("$void::main::void");
		if (!func)
		{
			std::cout << "Failed to find entry point in assembly [" << m_Assemblies[0].get_path().string() << "]!" << std::endl;
			return;
		}
		m_MainThread.execute(func);
	}

}
