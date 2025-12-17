#include "assembler/Token.h"
#include "common/Error.h"
#include "cryopch.h"
#include "Tokenizer.h"

#include "InstructionSet.h"
#include "Instructions.h"

#include <ostream>
#include <spdlog/spdlog.h>
#include <string_view>
#include <vector>

namespace Cryo::Assembler {

	Tokenizer::Tokenizer(const char* buffer, uint32_t buffer_size, const std::filesystem::path& path)
		: m_Buffer(buffer), m_BufferSize(buffer_size), m_FilePath(path)
	{
	}

   /*std::vector<Token> Tokenizer::tokenize(ErrorQueue& errors)
   {
     std::vector<Token> token_vec;
     token_vec.reserve(1000); // This is like, 16kb so it's fine

     for (uint32_t i = 0; i < m_BufferSize; i++)
     {
       TokenType type = TokenType::None;
        switch (m_Buffer[i]) // Prepositions
        {
        case '#': // Comment
          for (i++; m_Buffer[i] != '\n' && i < m_BufferSize; i++); // Skip comment
          break;

        case '$': // ID
          type = TokenType::ID;
          // Intended fallthrough
        case '@': // Type
          if (type == TokenType::None) { type = TokenType::Type; }
          switch
        }
     }

     return token_vec;
   }*/

	std::vector<Token> Tokenizer::tokenize(ErrorQueue& errors)
	{
		std::vector<Token> token_vec;

		for (uint32_t i = 0; i < m_BufferSize; i++)
		{
			char character = m_Buffer[i];
			const char* token_start = nullptr;
			uint32_t token_size = 0;
      bool was_inserted = false;

			// Separate 'words'
			while (character != ' ' && character != '\n' && character != '\r' && character != '\t' && i < m_BufferSize)
			{
        if (character == '"')
        {
          token_start = m_Buffer + i + 1;
          bool found_end = false;
          for (i++; i < m_BufferSize; i++)
          {
            character = m_Buffer[i];
            if (character == '"')
            {
              found_end = true;
              break;
            }
            token_size++;
          }
          if (!found_end)
          {
            errors.push_error(ERR_A_STRING_LITERAL_MISSING_END, m_FilePath, m_Buffer, m_BufferSize, token_vec.back().tokenText);
            return token_vec;
          }

          token_vec.emplace_back(TokenType::StringLiteral, std::string_view(token_start, token_size));
          was_inserted = true;
          break;
        }

				if (!token_start) { token_start = m_Buffer + i; }
				i++;
				token_size++;
				character = m_Buffer[i];
			}
			if (token_size == 0 || was_inserted) { continue; }

			token_vec.emplace_back(TokenType::None, std::string_view(token_start, token_size));
			uint32_t token = token_vec.size() - 1;

			check_and_split_token_preposition(token, token_vec, errors);
			if (errors.get_severity() == Error::level_critical) { return token_vec; }
		}

		return token_vec;
	}

	void Tokenizer::check_and_split_token_preposition(uint32_t token_index, std::vector<Token>& token_vec, ErrorQueue& errors)
	{
		for (uint32_t j = 0; j < token_vec[token_index].tokenText.size(); j++)
		{
			switch (token_vec[token_index].tokenText[j])
			{
			case '$':
				if (j == 0)
				{
					token_vec[token_index].type = TokenType::ID;
				}
				else
				{
					token_vec.emplace_back(TokenType::ID, std::string_view(&token_vec[token_index].tokenText[j], token_vec[token_index].tokenText.size() - j));
					uint32_t new_token = token_vec.size() - 1;
					token_vec[token_index].tokenText = std::string_view(&token_vec[token_index].tokenText[j], token_vec[token_index].tokenText.data() + j);
					check_and_split_token_preposition(new_token, token_vec, errors);
					if (errors.get_severity() == Error::level_critical) { return; }
				}
				break;

			case '@':
				if (j == 0)
				{
					token_vec[token_index].type = TokenType::Type;
				}
				else
				{
					token_vec.emplace_back(TokenType::Type, std::string_view(&token_vec[token_index].tokenText[j], token_vec[token_index].tokenText.size() - j));
					uint32_t new_token = token_vec.size() - 1;
					token_vec[token_index].tokenText = std::string_view(&token_vec[token_index].tokenText[j], token_vec[token_index].tokenText.data() + j);
					check_and_split_token_preposition(new_token, token_vec, errors);
					if (errors.get_severity() == Error::level_critical) { return; }
				}
				break;

			case ';':
				if (j == 0)
				{
					token_vec[token_index].type = TokenType::EndCommand;
					token_vec[token_index].tokenText = std::string_view(token_vec[token_index].tokenText.data(), 1);
				}
				else
				{
					token_vec.emplace_back(Token(TokenType::EndCommand, std::string_view(token_vec[token_index].tokenText.data() + j, 1)));

					if (j != token_vec[token_index].tokenText.size() - 1)
					{
						token_vec.emplace_back(Token(TokenType::None, 
							std::string_view(token_vec[token_index].tokenText.data() + j + 1, token_vec[token_index].tokenText.size() - j + 1)));
						uint32_t new_token = token_vec.size() - 1;
						check_and_split_token_preposition(new_token, token_vec, errors);
						if (errors.get_severity() == Error::level_critical) { return; }
					}

					token_vec[token_index].tokenText = std::string_view(token_vec[token_index].tokenText.data(), j);
				}
				break;
			}
		}
		if (token_vec[token_index].type != TokenType::None) { return; }

		check_and_split_token_keyword(token_index, token_vec, errors);
	}

	const std::unordered_map<std::string_view, TokenType> Tokenizer::s_TokenToType =
	{
		{ "fn", TokenType::FunctionDeclaration },
		{ "->", TokenType::ReturnTypeDeclaration },
		{ "{", TokenType::StartBody },
		{ "}", TokenType::EndBody },
		{ ";", TokenType::EndCommand },
      { ",", TokenType::Separator }
	};

	void Tokenizer::check_and_split_token_keyword(uint32_t token_index, std::vector<Token>& token_vec, ErrorQueue& errors)
	{
		for (auto& tk : s_TokenToType)
		{
			size_t index = token_vec[token_index].tokenText.find(tk.first);
			if (index == 0)
			{
				token_vec[token_index].type = tk.second;
				return;
			}
			else if (index != std::string_view::npos)
			{
				token_vec.emplace_back(tk.second, std::string_view(&token_vec[token_index].tokenText[index], token_vec[token_index].tokenText.size() - index));
				uint32_t new_token = token_vec.size() - 1;
				token_vec[token_index].tokenText = std::string_view(token_vec[token_index].tokenText.data(), index);
				if (token_vec[new_token].tokenText.size() > 1) { check_and_split_token_keyword(new_token, token_vec, errors); }
				break;
			}
		}

		// Check if token is of type value: int || float
		{
			bool is_numeric = true;
			bool is_float = false;
			for (char digit : token_vec[token_index].tokenText)
			{
				if (digit == '.')
				{
					if (is_float) // double dots: 1..2
					{
						errors.push_error(ERR_A_MULTIPLE_DOTS_IN_VALUE, m_FilePath, m_Buffer, m_BufferSize, token_vec[token_index].tokenText);
						return;
					}
					is_float = true;
				}
				else if (!std::isdigit(digit))
				{
					is_numeric = false;
					break;
				}
			}
			if (is_numeric)
			{
				if (is_float)
				{
					token_vec[token_index].type = TokenType::Float;
				}
				else
				{
					token_vec[token_index].type = TokenType::Integer;
				}
				return;
			}
		}

		// Check if token is instruction
		{
			if (InstructionSet::is_instruction(token_vec[token_index].tokenText))
			{
				token_vec[token_index].type = TokenType::Instruction;
				return;
			}
		}

		// Could not determine the token type
		errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, token_vec[token_index].tokenText);
	}

}
