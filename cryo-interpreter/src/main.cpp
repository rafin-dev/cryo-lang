#include "cryopch.h"
#include "core/CryoState.h"

int main(int argc, const char* argv[])
{
	Cryo::CryoState state = Cryo::CryoState(argc - 1, argv + 1); // Ignore default argument with the exe's filepath
	if (!state.is_valid())
	{
		return -1;
	}
	state.run_entry_point();
}
