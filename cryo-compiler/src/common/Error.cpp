#include "cryopch.h"
#include "Error.h"

#include <spdlog/spdlog.h>
#include <spdlog/fmt/bundled/color.h>

namespace Cryo {

	std::unordered_map<std::string_view, Error::ErrorData> Error::s_ErrorTexts =
	{
		{ CRI_A_ASSEMBLY_FILE_DOES_NOT_EXIST,                             { "Cryo Assembly file does not exist!",                        Error::level_critical } },
		{ ERR_A_INVALID_CHARACTER_IN_ID_OR_TYPE,                          { "Invalid character in Cryo Assembly Identifier/Type!",       Error::level_error } },
		{ ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE,                           { "Could not determine token type!",                           Error::level_error } },
		{ ERR_A_MULTIPLE_DOTS_IN_VALUE,                                   { "Multiple dots in value token!",                             Error::level_error } },
		{ ERR_A_UNEXPECTED_END,                                           { "Unexpected end!",                                           Error::level_error } },
		{ ERR_A_FUNCTION_DEFINITION_MISSING_IDENTIFIER,                   { "Function definition missing identifier",                    Error::level_error } },
		{ ERR_A_FUNCTION_DEFINITION_MISSING_PARAM_TYPE,                   { "Function definition missing parameter type",                Error::level_error } },
		{ ERR_A_FUNCTION_DEFINITION_MISSING_RETURN_DECLARATION,	          { "Function definition missing return declaration!",           Error::level_error } },
		{ ERR_A_FUNCTION_DEFINITION_MISSING_RETURN_TYPE,                  { "Function definition missing return Type!",                  Error::level_error } },
		{ ERR_A_FUNCTION_DEFINITION_MISSING_BODY,                         { "Function definition missing body!",                         Error::level_error } },
		{ ERR_A_INVALID_FUNCTION_DEFINTION,                               { "Invalid function definition!",                              Error::level_error } },
		{ ERR_A_INVALID_TOKEN_IN_FUNCTION_BODY,                           { "Invalid token in function body!",                           Error::level_error } },
		{ ERR_A_UNEXPECTED_TOKEN_IN_INSTRUCTION_PARAMETERS,               { "Unexpected token in instruction parameters!",               Error::level_error } },
		{ ERR_A_MISSING_SEMICOLON,                                        { "Missing semicolon!",                                        Error::level_error } },
    { ERR_A_VARIABLE_NAME_ALREDY_IN_USE,                              { "Variable name alredy in use!",                              Error::level_error } },
	  { ERR_A_STACK_DOES_NOT_CONTAIN_VARIABLES_TO_POP,                  { "Stack does not contain variables to pop!",                  Error::level_error } },
    { ERR_A_VARIBALE_DOES_NOT_EXIST,                                  { "Variable does not exist!",                                  Error::level_error } },
    { ERR_A_THERE_ARE_NO_STACK_LAYERS_TO_BE_CLOSED,                   { "There are no StackLayers to be closed!",                    Error::level_error } },
    { ERR_A_UNKNOWN_TYPE,                                             { "Unknown type used!",                                        Error::level_error } }
  };

	Error::Error(std::string_view error_code, const std::filesystem::path& file_path, const char* file_buffer, uint32_t buffer_size, std::string_view token)
	{
		// Validate error code
		auto ite = s_ErrorTexts.find(error_code);
		if (ite == s_ErrorTexts.end())
		{
			spdlog::critical("Catastrophic internal failure: Invalid Error code [{0}]!", error_code);
			std::terminate(); // this problably means someone tried to magic string the code instead of using the defines
		}
		if (token.data() < file_buffer)
		{
			spdlog::critical("Catastrophic internal failure: Token is not part of file_buffer!");
			std::terminate();
		}

		ErrorCode = error_code;
		ErrorText = ite->second.ErrorText;
		ErrorSeverity = ite->second.ErrorSeverity;

		FilePath = file_path;

		// Get the line and make a string_view for the token
		const char* line_start = token.data();
		while (line_start > file_buffer)
		{
			if (*line_start == '\n')
			{
				line_start++;
				break;
			}
			line_start--;
		}
		const char* line_end = token.data();
		while (line_end != (file_buffer + buffer_size))
		{
			if (*line_end == '\n')
			{
				break;
			}
			line_end++;
		}
		const char* i = file_buffer;
		while (i != line_start)
		{
			if (*i == '\n') { LineNumber++; }
			i++;
		}

		ErrorLine = std::string(line_start, line_end);
		uint32_t token_index_in_line = token.data() - line_start;
		TokenText = std::string_view(ErrorLine.data() + token_index_in_line, token.size());
	}

	void Error::log()
	{
		spdlog::level::level_enum log_level = spdlog::level::off;
		switch (ErrorSeverity)
		{
		case level_warning:
			log_level = spdlog::level::warn;
			break;

		case level_error:
			log_level = spdlog::level::err;
			break;

		case level_critical:
			log_level = spdlog::level::critical;
			break;

		default:
			spdlog::critical("Catastrophic internal failure: Invalid Error severity!");
			std::terminate();
		}

		auto code = std::string(ErrorCode);
		spdlog::log(log_level, "{0} {1} at line {2}:[ {3}{4}{5} ]",
			code,
			FilePath.string(),
			LineNumber,
			std::string((const char*)ErrorLine.data(), TokenText.data()),
			fmt::format(fmt::fg(fmt::terminal_color::red) | fmt::emphasis::bold, "{0}", std::string(TokenText)),
			std::string(TokenText.data() + TokenText.size(), (const char*)ErrorLine.data() + ErrorLine.size()));

		spdlog::log(log_level, "{0} {1} at line {2}: {3}", code, FilePath.string(), LineNumber, ErrorText);
	}

	void ErrorQueue::push_error(std::string_view error_code, const std::filesystem::path& file_path, const char* file_buffer, uint32_t buffer_size, std::string_view token)
	{
		auto& err = m_Errrors.emplace(error_code, file_path, file_buffer, buffer_size, token);
		if (err.ErrorSeverity > m_Severity) { m_Severity = err.ErrorSeverity; }
	}

	void ErrorQueue::log()
	{
		while (m_Errrors.size())
		{
			m_Errrors.front().log();
			m_Errrors.pop();
		}
	}

}
