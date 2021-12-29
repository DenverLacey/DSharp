#include <iostream>
#include <optional>
#include <assert.h>

//
//
// TODOS:
// - HandleUTF8
//		Tokenizer currently only really supports ASCII.
// - HandleStringData
//		Identifier and string literal Tokens currently point to the source code.
// - HandleNumberLiterals
//		next_number_token()` only handles integers.
//
//


//
//
// Forward Declarations
//
//


//
//
// Globals and Constants
//
//

namespace Color {
	constexpr const char *Reset   = "\e[0m";
	constexpr const char *Black   = "\e[30m";
	constexpr const char *Red     = "\e[31m";
	constexpr const char *Green   = "\e[32m";
	constexpr const char *Yellow  = "\e[33m";
	constexpr const char *Blue    = "\e[34m";
	constexpr const char *Magenta = "\e[35m";
	constexpr const char *Cyan    = "\e[36m";
	constexpr const char *White   = "\e[37m";
}

//
//
// Helper Data Structures
//
//

struct String {
	size_t size;
	char *data;

	String() = default;

	String(size_t size, char *data) :
		size(size),
		data(data)
	{
	}

	String(char *data) : 
		size(strlen(data)),
		data(data)
	{
	}

	char advance() {
		char c = *data;
		
		data++;
		size--;

		return c;
	}

	bool operator==(const String& other) const {
		return size == other.size && memcmp(data, other.data, size) == 0;
	}

	bool operator==(const char *other) const {
		return (other[size] == '\0') && (memcmp(data, other, size) == 0);
	}

	bool operator!=(const String& other) const {
		return !(*this == other);
	}

	bool operator!=(const char *other) const {
		return !(*this == other);
	}
};

template<typename T>
struct Array {
	size_t count;
	T *elems;
};

struct Code_Location {
	size_t l0, c0;
	const char *file;
};

//
//
// Tokenizer
//
//

enum class Token_Kind {
	Eof,

	// literals
	Literal_Null,
	Literal_True,
	Literal_False,
	Literal_Boolean,
	Literal_Character,
	Literal_Integer,
	Literal_Floating_Point,
	Literal_String,

	// symbols
	Symbol_Identifier,

	// delimeters
	Delimeter_Newline,
	Delimeter_Semicolon,
	Delimeter_Left_Parenthesis,
	Delimeter_Right_Parenthesis,

	// punctuation
	Punctuation_Equal,
	Punctuation_Colon,
	Punctuation_Plus,
	Punctuation_Dash,
	Punctuation_Star,
	Punctuation_Slash,

	// keywords
};

union Token_Data {
	bool boolean;
	char32_t character;
	int64_t integer;
	double floating_point;
	String string;
};

struct Token {
	Token_Kind kind;
	Token_Data data;
	Code_Location location;
};

struct Tokenizer {
	size_t line = 0;
	size_t coloumn = 0;
	String source;
	Token previous_token;
	std::optional<Token> peeked_token;

	// @TODO:
	// :HandleUTF8
	//
	char32_t peek_char(int skip = 0) {
		return source.data[skip];
	}

	// @TODO:
	// :HandleUTF8
	//
	char32_t next_char() {
		char32_t next_char = peek_char();
		source.advance();
		coloumn++;
		return next_char;
	}

	Code_Location current_location() {
		return Code_Location {
			line,
			coloumn,
			"<@TODO: FILENAME>"
		};
	}

	Token make_token(Token_Kind kind, Token_Data data = {}) {
		return Token { 
			kind, 
			data, 
			current_location() 
		};
	}

	bool is_whitespace(char32_t c) {
		return c != '\n' && isspace(c);
	}

	bool is_identifier_character(char32_t c) {
		return c == '_' || isalnum(c);
	}

	char32_t skip_to_begining_of_next_token() {
		char c = peek_char();

		while (is_whitespace(c)) {
			c = next_char();

			if (c == '/' && peek_char(1) == '/') {
				while (c != '\n') {
					c = next_char();
				}
			}

			if (c == '\n' && previous_token.kind == Token_Kind::Delimeter_Newline) {
				while (c == '\n') {
					c = next_char();
					line++;
				}
			}
		}

		return c;
	}

	Token peek() {
		if (peeked_token.has_value()) {
			return peeked_token.value();
		}
		peeked_token = next();
		return peeked_token.value();
	}

	Token next() {
		if (peeked_token.has_value()) {
			previous_token = peeked_token.value();
			peeked_token = {};
			return previous_token;
		}

		// @TODO:
		// :HandleUTF8
		//

		char32_t c = skip_to_begining_of_next_token();

		if (c == '\0') {
			previous_token = make_token(Token_Kind::Eof);
		} else if (c == '\n') {
			previous_token = make_token(Token_Kind::Delimeter_Newline);
			line++;
		} else if (c == '\'') {
			previous_token = next_character_token();
		} else if (c == '\"') {
			previous_token = next_string_token();
		} else if (isdigit(c) || (c == '.' && isdigit(peek_char(1)))) {
			previous_token = next_number_token();
		} else if (isalpha(c) || (c == '_' && is_identifier_character(peek_char(1)))) {
			previous_token = next_keyword_or_identifier_token();
		} else {
			previous_token = next_punctuation_token(c);
		}

		return previous_token;
	}

	Token next_character_token() {
		char32_t character = next_char();
		assert(isalnum(character) || ispunct(character) || isspace(character));
		assert(next_char() == '\'');
		return make_token(
			Token_Kind::Literal_Character, 
			Token_Data { 
				.character = character 
			}
		);
	}

	Token next_string_token() {
		// @TODO:
		// :HandleStringData
		//
		auto string = String { 0, source.data };

		char32_t c = next_char();
		while (c != '\"' && c != '\0') {
			string.size++;
			c = next_char();
		}

		return make_token(
			Token_Kind::Literal_String, 
			Token_Data { 
				.string = string 
			}
		);
	}

	Token next_number_token() {
		// @TODO:
		// :HandleUTF8
		//
		// @NOTE:
		// This is because we've already moved passed the first character. 
		// This should be dealt with more explicitly or by reworking other stuff.
		//
		char *start = source.data - 1;
		size_t size = 0;

		// @TODO:
		// :HandleNumberLiterals
		//
		while (isdigit(peek_char())) {
			size++;
			next_char();
		}

		char *word = reinterpret_cast<char *>(alloca(size));
		memcpy(word, start, size);

		int64_t number = atoll(word);

		return make_token(
			Token_Kind::Literal_Integer,
			Token_Data { 
				.integer = number
			}
		);
	}

	Token next_keyword_or_identifier_token() {
		auto word = String { 0, source.data };

		while (is_identifier_character(peek_char())) {
			word.size++;
			next_char();
		}

		Token token;
		if (false) {
			// @TODO:
			// check if `word` is a keyword
			//
			assert(!"Keywords not implemented!");
		} else {
			token = make_token(
				Token_Kind::Symbol_Identifier,
				Token_Data {
					.string = word
				}
			);
		}

		return token;
	}

	Token next_punctuation_token(char32_t c) {
		Token token;

		switch (c) {
			case ';': {
				token = make_token(Token_Kind::Delimeter_Semicolon);
			} break;
			case '(': {
				token = make_token(Token_Kind::Delimeter_Left_Parenthesis);
			} break;
			case ')': {
				token = make_token(Token_Kind::Delimeter_Right_Parenthesis);
			} break;
			case '=': {
				token = make_token(Token_Kind::Punctuation_Equal);
			} break;
			case ':': {
				token = make_token(Token_Kind::Punctuation_Colon);
			} break; 
			case '+': {
				token = make_token(Token_Kind::Punctuation_Plus);
			} break;
			case '-': {
				token = make_token(Token_Kind::Punctuation_Dash);
			} break;
			case '*': {
				token = make_token(Token_Kind::Punctuation_Star);
			} break;
			case '/': {
				token = make_token(Token_Kind::Punctuation_Slash);
			} break;

			default:
				assert(!"Invalid character encountered while tokenizing punctuation!");
		}

		return token;
	}
};

//
//
// Entry Point
//
//

int main(int argc, const char **argv) {
	String source = const_cast<char *>("a + \"Hello\"");

	Tokenizer tokenizer;
	tokenizer.source = source;

	Token t1 = tokenizer.next();
	Token t2 = tokenizer.next();
	Token t3 = tokenizer.next();
	Token t4 = tokenizer.next();

	{
		std::cout << Color::Cyan << "t1:" << Color::Reset << std::endl;
		
		assert(t1.kind == Token_Kind::Symbol_Identifier);
		std::cout << "\tKind: Symbol_Identifier" << std::endl;

		assert(t1.data.string == "a");
		std::cout << "\tdata.string: \"a\"" << std::endl;

		std::cout << "\tLocation: Ln " << t1.location.l0 << ", Col " << t1.location.c0 << std::endl;
	}

	{
		std::cout << Color::Cyan << "t2:" << Color::Reset << std::endl;
		
		assert(t2.kind == Token_Kind::Punctuation_Plus);
		std::cout << "\tKind: Punctuation_Plus" << std::endl;

		std::cout << "\tLocation: Ln " << t2.location.l0 << ", Col " << t2.location.c0 << std::endl;
	}

	{
		std::cout << Color::Cyan << "t3:" << Color::Reset << std::endl;
		
		assert(t3.kind == Token_Kind::Literal_String);
		std::cout << "\tKind: Literal_String" << std::endl;

		assert(t3.data.string == "Hello");
		std::cout << "\tdata.string: \"Hello\"" << std::endl;

		std::cout << "\tLocation: Ln " << t3.location.l0 << ", Col " << t3.location.c0 << std::endl;
	}
	
	assert(t4.kind == Token_Kind::Eof);

	std::cout << Color::Green << "SUCCESS!" << Color::Reset << std::endl;

	return 0;
}
