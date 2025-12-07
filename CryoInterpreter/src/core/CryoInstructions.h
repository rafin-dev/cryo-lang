#pragma once

#include <cstdint>

namespace Cryo {

	/// <summary>
	/// Enum conatining the instructions opcodes
	/// </summary>
	enum CryoOpcode : uint32_t
	{
		/// <summary>
		/// Stack Layer Start: 4 bytes opcode
		/// </summary>
		STLS = 0x00000001,
		/// <summary>
		/// Stack Layer End: 4 bytes opcode
		/// </summary>
		STLE = 0x00000002,
		/// <summary>
		/// Stack Push: 4 bytes opcode, 4 bytes uint for size
		/// </summary>
		PUSH = 0x00000003,

		/// <summary>
		/// Return: 4 bytes opcode
		/// </summary>
		RETURN = 0x01000000,
		/// <summary>
		/// Call function in the same assembly as the caller function, 4 bytes opcode, 4 bytes uint for function index
		/// </summary>
		CALL_from_assembly_index = 0x02000000,
		/// <summary>
		/// Call function in the same assembly as the caller function, 4 bytes opcode, 4 bytes uint for string literal index for the signature
		/// </summary>
		CALL_from_assembly_signature = 0x30000000
	};

}