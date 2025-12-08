#pragma once

#include <cstdint>

namespace Cryo {

	/// <summary>
	/// Enum conatining the instructions opcodes
	/// </summary>
	enum CryoOpcode : uint32_t
	{
		/// Stack Layer Start: 4 bytes opcode
		STLS = 0x00000001,
		/// Stack Layer End: 4 bytes opcode
		STLE = 0x00000002,
		/// Stack Push: 4 bytes opcode, 4 bytes uint for size
		PUSH = 0x00000003,
    POP  = 0x00000004,

		/// Return: 4 bytes opcode
		RETURN = 0x01000000,
		/// Call function in the same assembly as the caller function, 4 bytes opcode, 4 bytes uint for function index
		CALL_from_assembly_index = 0x02000000,
		/// Call function in the same assembly as the caller function, 4 bytes opcode, 4 bytes uint for string literal index for the signature
		CALL_from_assembly_signature = 0x30000000
	};

}
