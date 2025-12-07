#pragma once

#include "Token.h"
#include "Common/Error.h"

#include <vector>

namespace Cryo::Assembler {

	class Tokenizer
	{
	public:
		Tokenizer(const char* buffer, uint32_t buffer_size, const std::filesystem::path& path);
		
		std::vector<Token> tokenize(ErrorQueue& errors);

	private:
		void check_and_split_token_preposition(uint32_t token_index, std::vector<Token>& token_vec, ErrorQueue& errors);
		void check_and_split_token_keyword(uint32_t token_index, std::vector<Token>& token_vec, ErrorQueue& errors);

		const char* m_Buffer = nullptr;
		uint32_t m_BufferSize = 0;
		std::filesystem::path m_FilePath;

		/// <summary>
		/// Contains info about tokens that are specif keywords/characters such as: 'fn', ';', '->'
		/// </summary>
		static const std::unordered_map<std::string_view, TokenType> s_TokenToType;
	};

}