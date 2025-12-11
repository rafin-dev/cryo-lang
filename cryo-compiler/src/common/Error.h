#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <filesystem>
#include <queue>
#include <memory>

namespace Cryo {

	struct Error
	{
		Error(std::string_view error_code, const std::filesystem::path& file_path, const char* file_buffer, uint32_t buffer_size, std::string_view token);

		/// <summary>
		/// Code specifying the error
		/// </summary>
		std::string_view ErrorCode;
		/// <summary>
		/// Error text, specific for the error code
		/// </summary>
		std::string_view ErrorText;

		/// <summary>
		/// File conatining the error
		/// </summary>
		std::filesystem::path FilePath;
		/// <summary>
		/// Line conatining the error
		/// </summary>
		std::string ErrorLine;
		uint32_t LineNumber = 1;
		/// <summary>
		/// Specific token containing the error (the owner string is ErrorLine)
		/// </summary>
		std::string_view TokenText;

		enum Severity
		{
			/// <summary>
			/// No error
			/// </summary>
			level_none = 0,
			/// <summary>
			/// Warning, compilation will still be succesful
			/// </summary>
			level_warning,
			/// <summary>
			/// Error, compilation will fail, but it does not interrupt compilation
			/// </summary>
			level_error,
			/// <summary>
			/// Critical, interrupts compilation when found
			/// </summary>
			level_critical
		};

		Severity ErrorSeverity;

	private:
		void log();

		friend class ErrorQueue;

		struct ErrorData
		{
			std::string_view ErrorText;
			Severity ErrorSeverity;
		};
		static std::unordered_map<std::string_view, ErrorData> s_ErrorTexts;
	};

	class ErrorQueue
	{
	public:
		void push_error(std::string_view error_code, const std::filesystem::path& file_path, const char* file_buffer, uint32_t buffer_size, std::string_view token);
		void log();

		Error::Severity get_severity() { return m_Severity; }

	private:
		Error::Severity m_Severity = Error::level_none;
		std::queue<Error> m_Errrors;
	};

}

// Assembler Errors
#define CRI_A_ASSEMBLY_FILE_DOES_NOT_EXIST                         "EA-0x1000"

#define ERR_A_INVALID_CHARACTER_IN_ID_OR_TYPE                      "EA-0x1001"
#define ERR_A_COULD_NOT_DETERMINE_TOKEN_TYPE                       "EA-0x1002"
#define ERR_A_MULTIPLE_DOTS_IN_VALUE                               "EA-0x1003"

#define ERR_A_UNEXPECTED_END                                       "EA-0x1004"

#define ERR_A_FUNCTION_DEFINITION_MISSING_IDENTIFIER               "EA-0x1005"
#define ERR_A_FUNCTION_DEFINITION_MISSING_PARAM_TYPE               "EA-0x1006"
#define ERR_A_FUNCTION_DEFINITION_MISSING_RETURN_DECLARATION       "EA-0x1007"
#define ERR_A_FUNCTION_DEFINITION_MISSING_RETURN_TYPE              "EA-0x1008"
#define ERR_A_FUNCTION_DEFINITION_MISSING_BODY                     "EA-0x1009"
#define ERR_A_INVALID_FUNCTION_DEFINTION                           "EA-0x100A"
#define ERR_A_INVALID_TOKEN_IN_FUNCTION_BODY                       "EA-0x100B"

#define ERR_A_UNEXPECTED_TOKEN_IN_INSTRUCTION_PARAMETERS           "EA-0x100C"
#define ERR_A_MISSING_SEMICOLON                                    "EA-0x100D"
#define ERR_A_VARIABLE_NAME_ALREDY_IN_USE                          "EA-0x100E"
#define ERR_A_STACK_DOES_NOT_CONTAIN_VARIABLES_TO_POP              "EA-0x100F"
#define ERR_A_VARIBALE_DOES_NOT_EXIST                              "EA-0x1010"
#define ERR_A_THERE_ARE_NO_STACK_LAYERS_TO_BE_CLOSED               "EA-0x1011"
#define ERR_A_UNKNOWN_TYPE                                         "EA-0x1012"

