#pragma once

#include <string_view>

namespace Cryo::Assembler {

	enum TokenType
	{
		None = 0,

		FunctionDeclaration, // fn
		StartBody, // {
		EndBody, // } 
		
		ID, // $...
				
		ReturnTypeDeclaration, // ->
		
		Type, // uint32, int32...

		Instruction, // PUSH, POP, CALL...
		EndCommand, // ;
		Separator, // ,

		U8,
		U16,
		U32,
		U64,

		I8,
		I16,
		I32,
		I64,

		F32,
		F64, // 1.2, 1.3, 1.0...

		StringLiteral // "foo", "bar"...
	};

	struct Token
	{
		TokenType type;
		std::string_view tokenText;
	};

}
