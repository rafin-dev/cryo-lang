#include "cryopch.h"
#include "Assembler.h"

#include "common/Error.h"
#include "InstructionSet.h"
#include "Instructions.h"
#include "Token.h"
#include "Tokenizer.h"
#include "TypeList.h"

#include <iterator>
#include <optional>
#include <spdlog/spdlog.h>

#include <cmath>
#include <cstdint>
#include <exception>
#include <string>
#include <tuple>

#define PUSH_ERROR(error_list, error, token_index) error_list.push_error(error, m_FilePath, m_Buffer.get(), m_BufferSize, m_Tokens[(token_index)].tokenText)

namespace Cryo::Assembler {

	Assembler::Assembler(const std::filesystem::path& path)
		: m_FilePath(path)
	{
		m_Tokens.reserve(200);
		// We can assume the caller checked if the file exists and the extension matches
		m_BufferSize = std::filesystem::file_size(m_FilePath);
		m_Buffer = std::make_unique<char[]>(m_BufferSize);

		std::ifstream file(m_FilePath, std::ios::binary);
		file.read(m_Buffer.get(), m_BufferSize);

		// By default, the .cryoInt file will be besides the .cryoAsm
		m_OutputFile = m_FilePath.replace_extension(".cryi");
	}

	void Assembler::assemble(ErrorQueue& errors)
	{
		Tokenizer tokenizer(m_Buffer.get(), m_BufferSize, m_FilePath);
		m_Tokens = tokenizer.tokenize(errors);
		if (errors.get_severity() == Error::level_critical)
		{
			return;
		}

		for (int i = 1; i < m_Tokens.size(); i++)
		{
      switch (m_Tokens[i].type) 
      {
        case TokenType::ID:
          if (m_Tokens[i - 1].type != TokenType::FunctionDeclaration)
          {
            m_StringLiterals.insert(std::string(m_Tokens[i].tokenText));
          }
          break;

        case TokenType::StringLiteral:
          m_StringLiterals.insert(std::string(m_Tokens[i].tokenText));
          break;

        default:
          break;
      }
		}

		for (uint32_t i = 0; i < m_Tokens.size(); i++)
		{
			if (m_Tokens[i].type == TokenType::FunctionDeclaration)
			{
        auto result = validate_function(i, errors);
				if (errors.get_severity() == Error::level_critical)
				{
					return;
				}
				if (result.has_value())
				{
					m_Functions.insert(std::pair(result.value().Signature, result.value()));
				}
			}
		}

		for (auto& ite : m_Functions)
		{
			assemble_function(ite.second, errors);
			if (errors.get_severity() == Error::level_critical)
			{
				return;
			}
		}
		if (errors.get_severity() != Error::level_none)
		{
			return;
		}
		serialize();
	}

	static std::unordered_set<TokenType> s_ValidTokensInFunctionBody =
	{
		TokenType::Instruction,
		TokenType::EndCommand,
		TokenType::ID,
		TokenType::Type,

		TokenType::U8,
		TokenType::U16,
		TokenType::U32,
		TokenType::U64,

		TokenType::I8,
		TokenType::I16,
		TokenType::I32,
		TokenType::I64,

		TokenType::F32,
		TokenType::F64,

		TokenType::StringLiteral
	};

  // TODO: check if it hasn't ran out of tokens pretty much everywhere
	std::optional<Function> Assembler::validate_function(uint32_t function_start, ErrorQueue& errors)
	{
		// A valid function will have at least 9 tokens
		if (function_start + 8 >= m_Tokens.size())
		{
			PUSH_ERROR(errors, ERR_A_UNEXPECTED_END, m_Tokens.size() - 1);
			return std::nullopt;
		}

		// See if there's any 'fn' tokens inside the function body
		int invalid_fn_count = 0;
		bool found_valid_end = false;
		Function func;
		func.FunctionStart = function_start;
		uint32_t current_token = function_start + 1; // Go from 'fn' to the id '$...'
		for (; current_token < m_Tokens.size(); current_token++)
		{
			if (m_Tokens[current_token].type == TokenType::FunctionDeclaration)
			{
				invalid_fn_count++;
				PUSH_ERROR(errors, ERR_A_INVALID_FUNCTION_DEFINTION, current_token);
			}
			else if (m_Tokens[current_token].type == TokenType::EndBody)
			{
				found_valid_end = true;
				break;
			}
		}
		if (!found_valid_end)
		{
			PUSH_ERROR(errors, ERR_A_FUNCTION_DEFINITION_MISSING_BODY, current_token);
			return std::nullopt;
		}
		if (invalid_fn_count != 0)
		{
			return std::nullopt;
		}

		// See if the function has a:
		// id
		current_token = function_start + 1; // Go from 'fn' to the id '$...'
		if (m_Tokens[current_token].type != TokenType::ID)
		{
			PUSH_ERROR(errors, ERR_A_FUNCTION_DEFINITION_MISSING_IDENTIFIER, current_token);
			return std::nullopt;
		}
		func.Signature = std::string(m_Tokens[current_token].tokenText.data() + 1, m_Tokens[current_token].tokenText.size() - 1);

	  // Parameter types 
    {
      for (current_token++; current_token < m_Tokens.size() && m_Tokens[current_token].type == TokenType::Type; current_token++)
      {
        auto result = TypeList::get_size_from_type(m_Tokens[current_token].tokenText);
        if (!result.has_value())
        {
          PUSH_ERROR(errors, ERR_A_UNKNOWN_TYPE, current_token);
          return std::nullopt;
        }
        if (result.value() != 0)
        {
          func.ParametersSizes.emplace_back(result.value());
        }
        func.Signature += "::" + std::string(m_Tokens[current_token].tokenText.data() + 1, m_Tokens[current_token].tokenText.size() - 1);
      }
      if (m_Tokens[current_token].type != TokenType::ReturnTypeDeclaration)
      {
        PUSH_ERROR(errors, ERR_A_FUNCTION_DEFINITION_MISSING_RETURN_DECLARATION, current_token);
        return std::nullopt;
      }
    }

		// return declaration
    {
      current_token++;
      if (m_Tokens[current_token].type != TokenType::Type)
      {
        PUSH_ERROR(errors, ERR_A_FUNCTION_DEFINITION_MISSING_RETURN_TYPE, current_token);
        return std::nullopt;
      }
      auto result = TypeList::get_size_from_type(m_Tokens[current_token].tokenText);
      if (!result.has_value())
      {
        PUSH_ERROR(errors, ERR_A_UNKNOWN_TYPE, current_token);
        return std::nullopt;
      }
      func.ReturnSize = result.value();
    }
    
		func.Signature = "$" + std::string(m_Tokens[current_token].tokenText.data() + 1, m_Tokens[current_token].tokenText.size() - 1) + "::" + func.Signature;
		m_StringLiterals.insert(func.Signature);
		return func;
	}

	void Assembler::assemble_function(Function& func ,ErrorQueue& errors)
	{
		uint32_t current_token;
		for (current_token = func.FunctionStart; m_Tokens[current_token].type != TokenType::StartBody; current_token++);
    
    VariableStack variables;
    if (func.ReturnSize != 0)
    {
      variables.push_variable("$return", func.ReturnSize);
    }
    for (uint32_t i = 0; i < func.ParametersSizes.size(); i++)
    {
      variables.push_variable("$param_" + std::to_string(i), func.ParametersSizes[i]);
    }

		for (current_token++; current_token < m_Tokens.size() && m_Tokens[current_token].type != TokenType::EndBody; current_token++)
		{
			Token& token = m_Tokens[current_token];
			if (!s_ValidTokensInFunctionBody.contains(token.type))
			{
				PUSH_ERROR(errors, ERR_A_INVALID_TOKEN_IN_FUNCTION_BODY, current_token);
			}

			if (token.type == TokenType::Instruction)
			{
				assemble_instruction(current_token, func, variables, errors);
				if (errors.get_severity() == Error::level_critical) { return; }
			}
		}
	}

	void Assembler::assemble_instruction(uint32_t instruction, Function& func, VariableStack& variables, ErrorQueue& errors)
	{
		uint32_t current_token = 0;
    std::vector<TokenType> parameters_type;
    parameters_type.reserve(4);
    bool has_semicolon = false;
    for (current_token = instruction + 1; current_token < m_Tokens.size(); current_token++)
    {
      if (m_Tokens[current_token].type == TokenType::EndCommand)
      {
        has_semicolon = true;
        break;
      }
      else if (m_Tokens[current_token].type == TokenType::Instruction || m_Tokens[current_token].type == TokenType::EndBody)
      {
        break;
      }
      parameters_type.emplace_back(m_Tokens[current_token].type);
    }
    if (!has_semicolon)
    {
      PUSH_ERROR(errors, ERR_A_MISSING_SEMICOLON, instruction);
      return;
    }

    CryoOpcode opcode = InstructionSet::get_opcode(m_Tokens[instruction].tokenText, parameters_type);
    if (opcode == CryoOpcode::NONE)
    {
      PUSH_ERROR(errors, ERR_A_UNEXPECTED_TOKEN_IN_INSTRUCTION_PARAMETERS, instruction);
      return;
    }

    func.Instructions.emplace_back(opcode);

    // Deal with parameters
    current_token = instruction + 1; // First parameter
    switch (opcode)
    {
      case CryoOpcode::STLS:
        variables.start_stack_layer();
        break;

      case CryoOpcode::STLE:
        if (!variables.end_stack_layer())
        {
          PUSH_ERROR(errors, ERR_A_THERE_ARE_NO_STACK_LAYERS_TO_BE_CLOSED, instruction);
          return;
        }
        break;

      case CryoOpcode::PUSH:
        {
          std::string_view type = m_Tokens[current_token].tokenText;
          auto result = TypeList::get_size_from_type(type);
          if (!result.has_value())
          {
            PUSH_ERROR(errors, ERR_A_UNKNOWN_TYPE, current_token);
            return;
          }

          func.Instructions.emplace_back(result.value());
          current_token++;
          if (!variables.push_variable(m_Tokens[current_token].tokenText, result.value())) // Register variable
          {
            PUSH_ERROR(errors, ERR_A_VARIABLE_NAME_ALREDY_IN_USE, current_token);
            return;
          }
        }
        break;

      case CryoOpcode::POP:
        {
          uint32_t count = std::stoul(std::string(m_Tokens[current_token].tokenText.data(), m_Tokens[current_token].tokenText.size() - 3)); // removes the u32
          func.Instructions.emplace_back(count);
          for (uint32_t i = 0; i < count; i++)
          {
            if (!variables.pop_variable())
            {
              PUSH_ERROR(errors, ERR_A_STACK_DOES_NOT_CONTAIN_VARIABLES_TO_POP, current_token);
              return;
            }
          }
        }
        break;

      case CryoOpcode::SETU32:
        {
          const VariableData* data = variables.get_variable(m_Tokens[current_token].tokenText);
          if (!data)
          {
            PUSH_ERROR(errors, ERR_A_VARIBALE_DOES_NOT_EXIST, current_token);
            return;
          }
          func.Instructions.emplace_back(data->Position);
          
          current_token++;
          uint32_t value = std::stoul(std::string(m_Tokens[current_token].tokenText.data(), m_Tokens[current_token].tokenText.size() - 3));
          func.Instructions.emplace_back(value);
        }
        break;

      case CryoOpcode::SETSTR:
        {
          const VariableData* data = variables.get_variable(m_Tokens[current_token].tokenText);
          if (!data)
          {
            PUSH_ERROR(errors, ERR_A_VARIBALE_DOES_NOT_EXIST, current_token);
            return;
          }
          func.Instructions.emplace_back(data->Position);
          
          current_token++;
          uint32_t string_index = 0;
          for (auto& str : m_StringLiterals)
          {
            if (str == m_Tokens[current_token].tokenText)
            {
              break;
            }
            string_index++;
          }

          func.Instructions.emplace_back(string_index);
        }
        break;

      case CryoOpcode::CALL_from_assembly_signature:
        {
          uint32_t sig_index = 0;
          for (auto ite = m_StringLiterals.begin(); *ite != m_Tokens[current_token].tokenText; ite = std::next(ite))
          {
            sig_index++;
          }

          func.Instructions.emplace_back(sig_index);
        }
        break;

      case CryoOpcode::IMPL:
        {
          uint32_t sig_index = 0;
          for (auto& str : m_StringLiterals)
          {
            if (str == m_Tokens[current_token].tokenText)
            {
              break;
            }
            sig_index++;
          }
          func.Instructions.emplace_back(sig_index);
        }
        break;

      default:
        break;
    }
  }

#define WRITE_BINARY(stream, x) stream.write(reinterpret_cast<const char*>(&x), sizeof(x))

	void Assembler::serialize()
	{
		if (!std::filesystem::exists(m_OutputFile.parent_path()))
		{
			std::filesystem::create_directories(m_OutputFile.parent_path());
		}

		std::ofstream file_stream(m_OutputFile, std::ios::out | std::ios::binary);

		constexpr uint32_t block_end = std::numeric_limits<uint32_t>::max();

		// Header
		constexpr const char* header_string = "CRYOINT";
		for (int i = 0; header_string[i] != 0; i++)
		{
			file_stream << header_string[i];
		}
		file_stream << '\0';

		// String literals
		uint32_t string_literals_size = 0;
		for (auto& str : m_StringLiterals)
		{
			file_stream << str << '\0';
			string_literals_size += str.size() + 1;
		}
		for (uint32_t spacing = sizeof(uint32_t) - (string_literals_size % sizeof(uint32_t)); spacing > 0; spacing--) // Align back into a list of uint32_t's
		{
			file_stream << '\0';
		}

		// Function declarations
		WRITE_BINARY(file_stream, block_end); // Indicates start of function declarations

		std::unordered_map<std::string, std::streamoff> function_indexes;
		for (auto& func : m_Functions)
		{
			uint32_t signature_id = 0;
			for (auto& ite : m_StringLiterals) { if (func.second.Signature == ite) { break; } signature_id++; }
			
			// Signature string literal index
			WRITE_BINARY(file_stream, signature_id);

			// Function start
			function_indexes.insert(std::pair(func.second.Signature, file_stream.tellp()));
			constexpr uint32_t placeholder = 0;
			WRITE_BINARY(file_stream, placeholder);

			// Instruction count
			uint32_t size = func.second.Instructions.size();
			WRITE_BINARY(file_stream, size);

      // Return size
		  WRITE_BINARY(file_stream, func.second.ReturnSize);

      // Parameters Sizes
      for (uint32_t param_size : func.second.ParametersSizes)
      {
        WRITE_BINARY(file_stream, param_size);
      }

      WRITE_BINARY(file_stream, block_end);
    }

		// Actual code
		WRITE_BINARY(file_stream, block_end);

		for (auto& func : m_Functions)
		{
			// Fill function start index in the function declaration
			std::streamoff pos = file_stream.tellp();
			file_stream.seekp(function_indexes[func.second.Signature]);

			uint32_t func_index = pos / sizeof(uint32_t);
			WRITE_BINARY(file_stream, func_index);

			file_stream.seekp(pos);

			// Actual function code
			file_stream.write(reinterpret_cast<const char*>(func.second.Instructions.data()), func.second.Instructions.size() * sizeof(func.second.Instructions[0]));
			WRITE_BINARY(file_stream, block_end);
		}
	}
}
