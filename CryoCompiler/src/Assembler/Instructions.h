#pragma once

#include <string_view>
#include <unordered_map>
#include <vector>

#include "Token.h"

namespace Cryo::Assembler {

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

	extern std::unordered_map<std::string_view, std::pair<CryoOpcode, std::vector<TokenType>>> s_InstructionsString;

}