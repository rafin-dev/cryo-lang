#pragma once

#include "Instructions.h"
#include "Token.h"

#include <map>
#include <unordered_set>
#include <vector>
#include <string_view>
#include <unordered_map>

namespace Cryo::Assembler {

  class InstructionSet
  {
  public:
    static CryoOpcode get_opcode(std::string_view instruction_str, const std::vector<TokenType>& parameters);

    static bool is_instruction(std::string_view token);

    static uint32_t get_params_size(CryoOpcode opcode);

  private:
    static std::map<std::pair<std::string_view, std::vector<TokenType>>, CryoOpcode> s_Instructions;
    static std::unordered_set<std::string_view> s_InstructionList;
    static std::unordered_map<CryoOpcode, uint32_t> s_InstructionParamsSize;
  };

}

