#pragma once

#include "Token.h"
#include "common/Error.h"

#include <vector>

namespace Cryo::Assembler {

	class Tokenizer
	{
	public:
		Tokenizer(const char* buffer, uint32_t buffer_size, std::filesystem::path path);
		
		std::vector<Token> tokenize(ErrorQueue& errors) const;

	private:
		uint32_t find_token_end(std::vector<Token>& token_vec, TokenType type, uint32_t token_start, ErrorQueue& errors) const;
		void validate_value_token(Token& token, ErrorQueue& errors) const;

		const char* const m_Buffer = nullptr;
		const uint32_t m_BufferSize = 0;
		const std::filesystem::path m_FilePath;
	};

}
