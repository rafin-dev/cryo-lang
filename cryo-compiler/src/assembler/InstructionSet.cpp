#include "cryopch.h"
#include "InstructionSet.h"

#include "Instructions.h"

namespace Cryo::Assembler {

  CryoOpcode InstructionSet::get_opcode(std::string_view instruction_str, const std::vector<TokenType>& params)
  {
    auto ite = s_Instructions.find({ instruction_str, params });
    if (ite == s_Instructions.end())
    {
      return CryoOpcode::NONE;
    }

    return ite->second;
  }

  bool InstructionSet::is_instruction(std::string_view token)
  {
    return s_InstructionList.contains(token);
  }

  uint32_t InstructionSet::get_params_size(CryoOpcode opcode)
  {
    return s_InstructionParamsSize[opcode];
  }

  std::map<std::pair<std::string_view, std::vector<TokenType>>, CryoOpcode> InstructionSet::s_Instructions = 
  {
    { std::make_pair("STLS", std::vector<TokenType>{}), CryoOpcode::STLS },
    { std::make_pair("STLE", std::vector<TokenType>{}), CryoOpcode::STLE },
    { std::make_pair("PUSH", std::vector{ TokenType::Type, TokenType::ID }), CryoOpcode::PUSH },
    { std::make_pair("POP",  std::vector{ TokenType::U32 }), CryoOpcode::POP },
   
    { std::make_pair("SETU32", std::vector{ TokenType::ID, TokenType::U32 }), CryoOpcode::SETU32 },

    { std::make_pair("SETSTR", std::vector{ TokenType::ID, TokenType::StringLiteral }), CryoOpcode::SETSTR },

    { std::make_pair("RETURN", std::vector<TokenType>{} ), CryoOpcode::RETURN },
    { std::make_pair("CALL", std::vector{ TokenType::ID }), CryoOpcode::CALL_from_assembly_signature },
    { std::make_pair("IMPL", std::vector{ TokenType::ID }), CryoOpcode::IMPL }
  };

  std::unordered_set<std::string_view> InstructionSet::s_InstructionList = 
  {
    "STLS",
    "STLE",
    "PUSH",
    "POP",
    
    "SETU32",

    "SETSTR",
    
    "RETURN",
    "CALL",
    "IMPL"
  };

  std::unordered_map<CryoOpcode, uint32_t> InstructionSet::s_InstructionParamsSize = 
  {
    { STLS,    0 },
    { STLE,    0 },
    { PUSH,    4 },
    { POP,     4 },
    { SETU32,  8 },
    { SETSTR,  8 },
    { RETURN,  0 },
    { CALL_from_assembly_signature, 4 },
    { IMPL,    4 }
  };

}
