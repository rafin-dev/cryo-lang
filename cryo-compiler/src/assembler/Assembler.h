#pragma once

#include "Token.h"
#include "common/Error.h"
#include "Instructions.h"
#include "VariableStack.h"

#include <filesystem>
#include <unordered_map>
#include <set>

namespace Cryo::Assembler {

	struct Function
	{
		std::string Signature;
		std::vector<uint32_t> Instructions;
	};

	class Assembler
	{
	public:
		Assembler(const std::filesystem::path& path);

		void assemble(ErrorQueue& errors);

		const std::filesystem::path& get_output_location() { return m_OutputFile; }

	private:
		// Tokens
		std::vector<Token> m_Tokens;

		std::string validate_function(uint32_t function_start, ErrorQueue& errors);
		void assemble_function(uint32_t function_start, const std::string& signature, ErrorQueue& errors);
		void assemble_instruction(uint32_t instruction, Function& func, VariableStack& variables, ErrorQueue& errors);

		void serialize();

		// Output
		std::filesystem::path m_OutputFile;
		std::set<std::string> m_IDs;
		std::unordered_map<std::string, Function> m_Functions;

		// File
		std::filesystem::path m_FilePath;
		std::unique_ptr<char[]> m_Buffer;
		uint32_t m_BufferSize = 0;
	};

}
