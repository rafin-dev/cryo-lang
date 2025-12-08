#pragma once

#include <string_view>
#include <map>
#include <vector>
#include <cstdint>

#include "Token.h"

namespace Cryo::Assembler {

	enum CryoOpcode : uint32_t
	{
		/// Stack Layer Start: 4 bytes opcode
		STLS = 0x00000001,
		/// Stack Layer End: 4 bytes opcode
		STLE = 0x00000002,
		/// Stack Push: 4 bytes opcode, 4 bytes uint for size
		PUSH = 0x00000003,
    // Stack Pop: 4 bytes opcode, 4 bytes uint with count
    POP  = 0x00000004,

		/// Return: 4 bytes opcode
		RETURN = 0x01000000,
		/// Call function in the same assembly as the caller function, 4 bytes opcode, 4 bytes uint for function index
		CALL_from_assembly_index = 0x02000000,
		/// Call function in the same assembly as the caller function, 4 bytes opcode, 4 bytes uint for string literal index for the signature
		CALL_from_assembly_signature = 0x30000000
	};

	extern std::multimap<std::string_view, std::pair<CryoOpcode, std::vector<TokenType>>> s_InstructionsString;

}
