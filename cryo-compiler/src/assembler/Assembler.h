#pragma once

#include "Token.h"
#include "common/Error.h"
#include "Instructions.h"
#include "VariableStack.h"

#include <optional>
#include <cstdint>
#include <filesystem>
#include <unordered_map>
#include <set>
#include <vector>

namespace Cryo::Assembler {

	struct Function
	{
    Function() = default;
    Function(const std::string& sig, uint32_t func_start, uint32_t return_size, const std::vector<uint32_t>& parameters_sizes)
      : Signature(sig), Instructions(), FunctionStart(func_start), ReturnSize(return_size), ParametersSizes(parameters_sizes)
    {}

		std::string Signature;
		std::vector<uint32_t> Instructions;
    uint32_t FunctionStart = 0;

    uint32_t ReturnSize = 0;
    std::vector<uint32_t> ParametersSizes;
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

    std::optional<Function> validate_function(uint32_t function_start, ErrorQueue& errors);
		
    void assemble_function(Function& func, ErrorQueue& errors);
		
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
