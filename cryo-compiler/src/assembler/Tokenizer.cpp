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

	Tokenizer::Tokenizer(const char* buffer, uint32_t buffer_size, std::filesystem::path path)
		: m_Buffer(buffer), m_BufferSize(buffer_size), m_FilePath(std::move(path)) {}

	std::vector<Token> Tokenizer::tokenize(ErrorQueue& errors) const {
		std::vector<Token> token_vec;
		token_vec.reserve(1000); // This is like 16kb, so it's fine

		for (uint32_t i = 0; i < m_BufferSize; i++) {
			TokenType type = TokenType::None;
			switch (m_Buffer[i]) {
				case '#': // Comment
					for (i++; i < m_BufferSize && m_Buffer[i] != '\n'; i++); // Skip comment
					break;

				case '"': {
						bool found_end = false;
						const char* token_start = m_Buffer + i + 1; // remove the "
						uint32_t token_size = 1;
						for (i++; i < m_BufferSize; i++) {
							if (m_Buffer[i] == '"') {
								found_end = true;
								break;
							}
							token_size++;
						}
						token_vec.emplace_back(TokenType::StringLiteral, std::string_view(token_start, token_size - 1));
						if (!found_end) {
							errors.push_error(ERR_A_STRING_LITERAL_MISSING_END, m_FilePath, m_Buffer, m_BufferSize, token_vec.back().tokenText);
							return token_vec;
						}
					}
					break;

				case ',':
					type = TokenType::Separator;
				case ';':
					if (type == TokenType::None) { type = TokenType::EndCommand; }
				case '{':
					if (type == TokenType::None) { type = TokenType::StartBody; }
				case '}':
					if (type == TokenType::None) { type = TokenType::EndBody; }
					token_vec.emplace_back(type, std::string_view(m_Buffer + i, 1));
					break;


				case '-':
					if (i + 1 < m_BufferSize && m_Buffer[i + 1] == '>') {
						token_vec.emplace_back(TokenType::ReturnTypeDeclaration, std::string_view(m_Buffer + i, 2));
						i++;
					}
					else {
						errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, std::string_view(m_Buffer + i, 1));
					}
					break;

				case 'f':
					if (i + 1 < m_BufferSize && m_Buffer[i + 1] == 'n') {
						token_vec.emplace_back(TokenType::FunctionDeclaration, std::string_view(m_Buffer + i, 2));
						i++;
					}
					else {
						errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, std::string_view(m_Buffer + i, 1));
					}
					break;

				case '@':
					type = TokenType::Type;
				case '$':
					if (type == TokenType::None) { type = TokenType::ID; }
					i = find_token_end(token_vec, type, i, errors);
					break;

				default:
					if (m_Buffer[i] != ' ' && m_Buffer[i] != '\n' && m_Buffer[i] != '\r') {
						i = find_token_end(token_vec, TokenType::None, i, errors);
						Token& token = token_vec.back();

						if (isdigit(token.tokenText[0])) { // TODO: Value like 10U32 5I32 3.32F32...
							validate_value_token(token, errors);
						}
						else if (InstructionSet::is_instruction(token.tokenText)) { // Only option left is an instruction
							token.type = TokenType::Instruction;
						}
						else {
							errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, token.tokenText);
						}
					}
					break;
			}
		}

		return token_vec;
	}

	uint32_t Tokenizer::find_token_end(std::vector<Token>& token_vec, TokenType type, uint32_t token_start, ErrorQueue &errors) const {
		const char* txt_start = m_Buffer + token_start;
		uint32_t token_size = 1;

		for (uint32_t i = token_start + 1; i < m_BufferSize; i++) {
			if (m_Buffer[i] == ',' || m_Buffer[i] == ';' || m_Buffer[i] == ' ' || m_Buffer[i] == '\n') { // ID or Type token ended
				break;
			}

			if (!isalnum(m_Buffer[i]) && m_Buffer[i] != '_' && m_Buffer[i] != ':' && m_Buffer[i] != '*') { // Invalid character in ID or Type
				errors.push_error(ERR_A_INVALID_CHARACTER_IN_ID_OR_TYPE, m_FilePath, m_Buffer, m_BufferSize, std::string_view(txt_start, token_size));
				break;
			}

			token_size++;
		}
		token_vec.emplace_back(type, std::string_view(txt_start, token_size));

		return token_start + token_size - 1;
	}

	void Tokenizer::validate_value_token(Token &token, ErrorQueue &errors) const {
		uint32_t digit_size = 0;
		bool found_dot = false;
		for (char c : token.tokenText) {
			if (!isdigit(c)) {
				if (found_dot) { break; }
				if (c == '.') {
					found_dot = true;
				}
				else {
					break;
				}
			}
			digit_size++;
		}
		if (digit_size == token.tokenText.size()) {
			errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, token.tokenText);
			return;
		}
		switch (token.tokenText[digit_size]) {
			case 'u':
				if (token.tokenText[digit_size + 1] == '8') {
					if (token.tokenText.size() == digit_size + 2) {
						token.type = TokenType::U8;
					}
					else {
						errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, token.tokenText);
					}
				}
				else if (token.tokenText[digit_size + 1] == '1') {
					if (digit_size + 3 == token.tokenText.size() && token.tokenText[digit_size + 2] == '6') {
						token.type = TokenType::U16;
					}
					else {
						errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, token.tokenText);
					}
				}
				else if (token.tokenText[digit_size + 1] == '3') {
					if (digit_size + 3 == token.tokenText.size() && token.tokenText[digit_size + 2] == '2') {
						token.type = TokenType::U32;
					}
					else {
						errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, token.tokenText);
					}
				}
				else if (token.tokenText[digit_size + 1] == '6') {
					if (digit_size + 3 == token.tokenText.size() && token.tokenText[digit_size + 2] == '4') {
						token.type = TokenType::U64;
					}
					else {
						errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, token.tokenText);
					}
				}
				break;

			case 'i':
				if (token.tokenText[digit_size + 1] == '8') {
					if (token.tokenText.size() == digit_size + 1) {
						token.type = TokenType::I8;
					}
					else {
						errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, token.tokenText);
					}
				}
				else if (token.tokenText[digit_size + 1] == '1') {
					if (digit_size + 3 == token.tokenText.size() && token.tokenText[digit_size + 2] == '6') {
						token.type = TokenType::I16;
					}
					else {
						errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, token.tokenText);
					}
				}
				else if (token.tokenText[digit_size + 1] == '3') {
					if (digit_size + 3 == token.tokenText.size() && token.tokenText[digit_size + 2] == '2') {
						token.type = TokenType::I32;
					}
					else {
						errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, token.tokenText);
					}
				}
				else if (token.tokenText[digit_size + 1] == '6') {
					if (digit_size + 3 == token.tokenText.size() && token.tokenText[digit_size + 2] == '4') {
						token.type = TokenType::I64;
					}
					else {
						errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, token.tokenText);
					}
				}
				break;

			case 'f':
				if (token.tokenText[digit_size + 1] == '3') {
					if (digit_size + 3 == token.tokenText.size() && token.tokenText[digit_size + 2] == '2') {
						token.type = TokenType::F32;
					}
					else {
						errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, token.tokenText);
					}
				}
				else if (token.tokenText[digit_size + 1] == '6') {
					if (digit_size + 3 == token.tokenText.size() && token.tokenText[digit_size + 2] == '4') {
						token.type = TokenType::F64;
					}
					else {
						errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, token.tokenText);
					}
				}
				break;

			default:
				errors.push_error(ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE, m_FilePath, m_Buffer, m_BufferSize, token.tokenText);
		}
	}
}
