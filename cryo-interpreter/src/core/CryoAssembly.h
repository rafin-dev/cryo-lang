#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <optional>
#include <unordered_map>

namespace Cryo {

	class CryoAssembly;

	struct CryoFunction
	{
		uint32_t* FunctionStart = nullptr;
		uint32_t InstrutionCount = 0;
		std::string_view FunctionSignature;
		const CryoAssembly* OwnerAssembly = nullptr;
	};

	class CryoAssembly
	{
	public:
		CryoAssembly(const std::filesystem::path& path);
		~CryoAssembly();

		/// <summary>
		/// Used to check if the assembly was able to load properly
		/// </summary>
		/// <returns> Returns true if valid, false if not </returns>
		bool is_valid() const { return m_AssemblyBuffer != nullptr; }

		const std::filesystem::path& get_path() const { return m_AssemblyPath; }

		/// <summary>
		/// Used to retrieve a CryoFunction by it's signature
		/// </summary>
		/// <param name="signature"> Function signature </param>w
		/// <returns> Returns a pointer to the function if it finds it, nullptr if it doesn't </returns>
		const CryoFunction* get_function_by_signature(const std::string& signature) const;
		const CryoFunction* get_function_by_index(uint32_t index) const;

		std::optional<std::string_view> get_string_literal(uint32_t index) const;

	private:
		std::filesystem::path m_AssemblyPath;
		uint32_t* m_AssemblyBuffer;

		std::vector<std::string_view> m_StringLiterals;

		std::vector<CryoFunction> m_Functions;
		std::unordered_map<std::string, uint32_t> m_FunctionFromSignature;
		std::unordered_map<uint32_t, uint32_t> m_FunctionFromLocation;
	};

}
