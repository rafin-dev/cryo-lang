#include "cryopch.h"
#include "Assembler.h"

#include "Tokenizer.h"

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
		m_OutputFile = m_FilePath.replace_extension(".cryoInt");
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
			if (m_Tokens[i].type == TokenType::ID)
			{
				if (m_Tokens[i - 1].type != TokenType::FunctionDeclaration)
				{
					m_IDs.insert(std::string(m_Tokens[i].tokenText));
				}
			}
		}

		std::unordered_map<std::string, uint32_t> valid_functions;
		for (uint32_t i = 0; i < m_Tokens.size(); i++)
		{
			if (m_Tokens[i].type == TokenType::FunctionDeclaration)
			{
				std::string sig = validate_function(i, errors);
				if (errors.get_severity() == Error::level_critical)
				{
					return;
				}
				if (!sig.empty())
				{
					valid_functions.insert(std::pair(sig, i));
				}
			}
		}

		for (auto& ite : valid_functions)
		{
			assemble_function(ite.second, ite.first, errors);
			if (errors.get_severity() == Error::level_critical)
			{
				return;
			}
		}
		m_Functions;
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
		TokenType::Integer,
		TokenType::Float,
		TokenType::StringLiteral
	};

	std::string Assembler::validate_function(uint32_t function_start, ErrorQueue& errors)
	{
		// A valid function will have at least 9 tokens
		if (function_start + 8 >= m_Tokens.size())
		{
			errors.push_error(ERR_A_UNEXPECTED_END, m_FilePath, m_Buffer.get(), m_BufferSize, m_Tokens.back().tokenText);
			return std::string();
		}

		// See if there's any 'fn' tokens inside the function body
		int invalid_fn_count = 0;
		bool found_valid_end = false;
		std::string signature;
		uint32_t current_token = function_start + 1; // Go from 'fn' to the id '$...'
		for (; current_token < m_Tokens.size(); current_token++)
		{
			if (m_Tokens[current_token].type == TokenType::FunctionDeclaration)
			{
				invalid_fn_count++;
				errors.push_error(ERR_A_INVALID_FUNCTION_DEFINTION, m_FilePath, m_Buffer.get(), m_BufferSize, m_Tokens[current_token].tokenText);
			}
			else if (m_Tokens[current_token].type == TokenType::EndBody)
			{
				found_valid_end = true;
				break;
			}
		}
		if (!found_valid_end)
		{
			errors.push_error(ERR_A_UNEXPECTED_END, m_FilePath, m_Buffer.get(), m_BufferSize, m_Tokens[current_token].tokenText);
			return std::string();
		}
		if (invalid_fn_count != 0)
		{
			return std::string();
		}

		// See if the function has a:
		// id
		Function func;
		current_token = function_start + 1; // Go from 'fn' to the id '$...'
		if (m_Tokens[current_token].type != TokenType::ID)
		{
			errors.push_error(ERR_A_FUNCTION_DEFINITION_MISSING_IDENTIFIER, m_FilePath, m_Buffer.get(), m_BufferSize, m_Tokens[current_token].tokenText);
			return std::string();
		}
		signature = std::string(m_Tokens[current_token].tokenText.data() + 1, m_Tokens[current_token].tokenText.size() - 1);

		// at least one parameter type
		current_token++;
		if (m_Tokens[current_token].type != TokenType::Type)
		{
			errors.push_error(ERR_A_FUNCTION_DEFINITION_MISSING_PARAM_TYPE, m_FilePath, m_Buffer.get(), m_BufferSize, m_Tokens[current_token].tokenText);
			return std::string();
		}
		signature += "::" + std::string(m_Tokens[current_token].tokenText.data() + 1, m_Tokens[current_token].tokenText.size() - 1);
		// TODO: check for more parameters

		// return declaration
		current_token++;
		if (m_Tokens[current_token].type != TokenType::ReturnTypeDeclaration)
		{
			errors.push_error(ERR_A_FUNCTION_DEFINITION_MISSING_RETURN_DECLARATION, m_FilePath, m_Buffer.get(), m_BufferSize, m_Tokens[current_token].tokenText);
			return std::string();
		}

		// return type
		current_token++;
		if (m_Tokens[current_token].type != TokenType::Type)
		{
			errors.push_error(ERR_A_FUNCTION_DEFINITION_MISSING_RETURN_TYPE, m_FilePath, m_Buffer.get(), m_BufferSize, m_Tokens[current_token].tokenText);
			return std::string();
		}

		signature = "$" + std::string(m_Tokens[current_token].tokenText.data() + 1, m_Tokens[current_token].tokenText.size() - 1) + "::" + signature;
		m_IDs.insert(signature);
		return signature;
	}

	void Assembler::assemble_function(uint32_t function_start, const std::string& signature, ErrorQueue& errors)
	{
		uint32_t current_token;
		for (current_token = function_start; m_Tokens[current_token].type != TokenType::StartBody; current_token++);

		Function func;

		for (current_token++; current_token < m_Tokens.size() && m_Tokens[current_token].type != TokenType::EndBody; current_token++)
		{
			Token& token = m_Tokens[current_token];
			if (!s_ValidTokensInFunctionBody.contains(token.type))
			{
				errors.push_error(ERR_A_INVALID_TOKEN_IN_FUNCTION_BODY, m_FilePath, m_Buffer.get(), m_BufferSize, token.tokenText);
			}

			if (token.type == TokenType::Instruction)
			{
				assemble_instruction(current_token, func, errors);
				if (errors.get_severity() == Error::level_critical) { return; }
			}
		}

		func.Signature = signature;
		m_Functions.insert(std::pair(signature, func));
	}

	void Assembler::assemble_instruction(uint32_t instruction, Function& func, ErrorQueue& errors)
	{
		bool valid = true;
		uint32_t current_token = instruction;
		auto ite = s_InstructionsString.find(m_Tokens[instruction].tokenText);
		for (TokenType type : ite->second.second)
		{
			current_token++;
			if (type != m_Tokens[current_token].type)
			{
				valid = false;
				break;
			}
		}
		if (valid)
		{
			current_token++;
			if (m_Tokens[current_token].type != TokenType::EndCommand)
			{
				errors.push_error(ERR_A_MISSING_SEMICOLON, m_FilePath, m_Buffer.get(), m_BufferSize, m_Tokens[current_token - 1].tokenText);
				return;
			}

			func.Instructions.emplace_back((uint32_t)ite->second.first);
			current_token = instruction;

			for (int i = 0; i < ite->second.second.size(); i++)
			{
				current_token++;
				switch (m_Tokens[current_token].type)
				{
				case TokenType::Integer:
					func.Instructions.emplace_back(std::stoul(std::string(m_Tokens[current_token].tokenText)));
					break;

				case TokenType::Float:
					func.Instructions.emplace_back(std::stof(std::string(m_Tokens[current_token].tokenText)));
					break;

				case TokenType::ID:
				{
					uint32_t index = 0;
					for (auto ite = m_IDs.begin(); *ite != m_Tokens[current_token].tokenText; std::advance(ite, 1)) 
					{ 
						index++; 
					}

					func.Instructions.emplace_back(index);
					break;
				}

				default:
					errors.push_error(ERR_A_UNEXPECTED_TOKEN_IN_INSTRUCTION_PARAMETERS, m_FilePath, m_Buffer.get(), m_BufferSize, m_Tokens[current_token].tokenText);
					break;
				}
			}			

			return;
		}

		// Could not find a matching instruction
		errors.push_error(ERR_A_UNEXPECTED_TOKEN_IN_INSTRUCTION_PARAMETERS, m_FilePath, m_Buffer.get(), m_BufferSize, m_Tokens[current_token].tokenText);
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
		constexpr const char* header_string = "CRYOEXE";
		for (int i = 0; header_string[i] != 0; i++)
		{
			file_stream << header_string[i];
		}
		file_stream << '\0';

		// String literals
		uint32_t string_literals_size = 0;
		for (auto& id : m_IDs)
		{
			file_stream << id << '\0';
			string_literals_size += id.size() + 1;
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
			for (auto& ite : m_IDs) { if (func.second.Signature == ite) { break; } signature_id++; }
			
			// Signature string literal index
			WRITE_BINARY(file_stream, signature_id);

			// Function start
			function_indexes.insert(std::pair(func.second.Signature, file_stream.tellp()));
			constexpr uint32_t placeholder = 0;
			WRITE_BINARY(file_stream, placeholder);

			// INstruction count
			uint32_t size = func.second.Instructions.size();
			WRITE_BINARY(file_stream, size);
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