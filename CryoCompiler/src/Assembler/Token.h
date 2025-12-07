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
		Comma, // ,

		Instruction, // PUSH, POP, CALL...
		EndCommand, // ;
		
		Integer, // 1, 2, -1, -2...
		Float, // 1.2, 1.3, 1.0...
		StringLiteral // "foo", "bar"...
	};

	struct Token
	{
		TokenType type;
		std::string_view tokenText;
	};

}