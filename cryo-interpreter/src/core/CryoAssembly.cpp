#include "cryopch.h"
#include <ostream>
#include "CryoAssembly.h"

namespace Cryo {

	CryoAssembly::CryoAssembly(const std::filesystem::path& path)
		: m_AssemblyPath(path)
	{
		uint16_t failed = false;
		failed |= (!std::filesystem::exists(m_AssemblyPath)) << 0;
		failed |= (!std::filesystem::is_regular_file(m_AssemblyPath)) << 1;
		failed |= (path.extension() != ".cryoExe") << 2;
		if (failed)
		{
			std::cout << "Failed to load CryoAssembly at [" << m_AssemblyPath.string() << "]: ";
			switch (failed)
			{
			case 1 << 0:
				std::cout << "File does not exist!";
				break;

			case 1 << 1:
				std::cout << "File is not a regular file!";
				break;

			case 1 << 2:
				std::cout << "File extension does not match!";
				break;
			}
			return;
		}

		size_t file_size = std::filesystem::file_size(m_AssemblyPath);
		if (file_size < 12) // 12 bytes is the current minimum for a valid assembly
		{
			std::cout << "CryoAssembly at [" << m_AssemblyPath.string() << "] does not meet the minimun size of 12 bytes!" << std::endl;
			return;
		}

		m_AssemblyBuffer = (uint32_t*)std::malloc(file_size);
		if (m_AssemblyBuffer == nullptr)
		{
			std::cout << "Failed to alllocate a buffer of size [" << file_size << "] for the CryoAssembly at [" << m_AssemblyPath.string() << "]!";
			return;
		}
		
		// Read file into the buffer
		std::ifstream fin(m_AssemblyPath, std::ios::in | std::ios::binary);
		fin.read((char*)m_AssemblyBuffer, file_size);

		// Validate file header
		const char* header_string = (const char*)m_AssemblyBuffer;
		constexpr const char* expected_string = "CRYOEXE";
		for (int i = 0; i < 8; i++)
		{
			if (header_string[i] != expected_string[i])
			{
				std::cout << "Failed to validate header for CryoAssembly at [" << m_AssemblyPath.string() << "]!";
				return;
			}
		}

		constexpr uint32_t block_end = std::numeric_limits<uint32_t>::max();

		// Read string literals
		uint32_t strings_size = 0;
		for (uint32_t i = 1; m_AssemblyBuffer[i] != block_end; i++) { strings_size++; }
		strings_size *= sizeof(uint32_t);

		const char* string_literals = reinterpret_cast<const char*>(m_AssemblyBuffer) + 8; // Right after the header
		bool last_was_null = false;
		const char* current_string_start = string_literals;
		uint32_t current_string_size = 0;
		for (int i = 0; i < strings_size; i++)
		{
			if (string_literals[i] != '\0')
			{
				current_string_size++;
				last_was_null = false;
			}
			else
			{
				if (last_was_null) { break; }
				last_was_null = true;

				m_StringLiterals.emplace_back(std::string_view(current_string_start, current_string_size));
				current_string_start = string_literals + i + 1;
				current_string_size = 0;
			}
		}

		uint32_t* function_ptr = m_AssemblyBuffer + 1 + (strings_size / sizeof(uint32_t)); // Jump ahead of the header and string literals
		while (function_ptr[0] != block_end || function_ptr[1] != block_end)
		{
      function_ptr += 1;
			// func { uint32_t signature_id, uint32_t instruction_start, uint32_t instruction_count, uint32_t return_size, uint32_t param_sizes[?] }
			CryoFunction func = {};
			uint32_t t = function_ptr[1];
			func.FunctionStart = m_AssemblyBuffer + function_ptr[1];
			func.InstrutionCount = function_ptr[2];
			func.FunctionSignature = m_StringLiterals[function_ptr[0]];
			
      func.ReturnTypeSize = function_ptr[3];
      func.ParameterSizes.reserve(5);
      uint32_t param_count = 0;
      for (int i = 4; function_ptr[i] != block_end; i++)
      {
        param_count++;
        func.ParameterSizes.emplace_back(function_ptr[i]);
      }

      func.OwnerAssembly = this;

			m_Functions.emplace_back(func);

			m_FunctionFromLocation.insert(std::pair(func.FunctionStart - m_AssemblyBuffer, m_Functions.size() - 1));
			m_FunctionFromSignature.insert(std::pair(func.FunctionSignature, m_Functions.size() - 1));
		
      function_ptr += 4 + param_count; // Minimun size + parameters
    }
	}

	CryoAssembly::~CryoAssembly()
	{
		free(m_AssemblyBuffer);
	}

	const CryoFunction* CryoAssembly::get_function_by_signature(const std::string& signature) const
	{
		auto ite = m_FunctionFromSignature.find(signature);
		if (ite == m_FunctionFromSignature.end())
		{
			return nullptr;
		}
		return &m_Functions[ite->second];
	}

	const CryoFunction* CryoAssembly::get_function_by_index(uint32_t index) const
	{
		auto ite = m_FunctionFromLocation.find(index);
		if (ite == m_FunctionFromLocation.end())
		{
			return nullptr;
		}
		return &m_Functions[ite->second];
	}

	std::optional<std::string_view> CryoAssembly::get_string_literal(uint32_t index) const
	{
		if (index >= m_StringLiterals.size())
		{
			return std::nullopt;
		}
		return m_StringLiterals[index];
	}

}
