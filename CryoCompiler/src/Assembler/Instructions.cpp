#include "cryopch.h"

#include "Token.h"
#include "Instructions.h"

namespace Cryo::Assembler {

	std::unordered_map<std::string_view, std::pair<CryoOpcode, std::vector<TokenType>>> s_InstructionsString =
	{
		{ "STLS",		{ CryoOpcode::STLS, {} } },
		{ "STLE",		{ CryoOpcode::STLE, {} } },

		{ "PUSH",		{ CryoOpcode::PUSH, { TokenType::Integer } } },
		
		{ "RETURN",		{ CryoOpcode::RETURN, {} } },
		{ "CALL",		{ CryoOpcode::CALL_from_assembly_signature, { TokenType::ID } } }
	};

}