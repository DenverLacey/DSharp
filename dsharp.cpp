#include <iostream>
#include <optional>
#include <assert.h>
#include <string>
#include <sstream>
#include <stdarg.h>

//
//
// TODOS:
// - HandleUTF8
//		Tokenizer currently only really supports ASCII.
// - HandleStringData
//		Identifier and string literal Tokens currently point to the source code.
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

constexpr size_t Print_Indentation_Size = 2;

//
//
// Helper Functions
//
//

template<class T>
std::string debug_str(const std::optional<T>& option) {
  std::stringstream s;
  
  if (option.has_value()) {
    s << "Some(" << option.value().debug_str() << ")";
  } else {
    s << "None";
  }

  return s.str();
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

  std::string debug_str() const {
    std::stringstream s;
    s << file << ':' << l0 + 1 << ':' << c0 + 1;
    return s.str();
  }
};

//
//
// Error Handling
//
//

[[noreturn]]
void error(const char *err, va_list args) {
  fprintf(stderr, "%sError: ", Color::Red);
  vfprintf(stderr, err, args);
  fprintf(stderr, "%s\n", Color::Reset);

  va_end(args);
  exit(EXIT_FAILURE);
}

[[noreturn]]
void error(const char *err, ...) {
  va_list args;
  va_start(args, err);

  error(err, args);
}

[[noreturn]]
void error(Code_Location location, const char *err, va_list args) {
  fprintf(stderr, "%sError @ %s: ", Color::Red, location.debug_str().c_str());
  vfprintf(stderr, err, args);
  fprintf(stderr, "%s\n", Color::Reset);

  va_end(args);
  exit(EXIT_FAILURE);
}

[[noreturn]]
void error(Code_Location location, const char *err, ...) {
  va_list args;
  va_start(args, err);

  error(location, err, args);
}

#define verify(condition, ...) if (!(condition)) error(__VA_ARGS__)

#define internal_error(...) internal_error_impl(__FILE__, __LINE__, __VA_ARGS__)

[[noreturn]]
void internal_error_impl(const char *file, size_t line, const char *errfmt, va_list args) {
  fprintf(stderr, "%sInternal Error @ %s:%zu: ", Color::Red, file, line);
  vfprintf(stderr, errfmt, args);
  fprintf(stderr, "%s\n", Color::Reset);

  va_end(args);
  exit(EXIT_FAILURE);
}

[[noreturn]]
void internal_error_impl(const char *file, size_t line, const char *errfmt, ...) {
  va_list args;
  va_start(args, errfmt);

  internal_error_impl(file, line, errfmt, args);
}

#define internal_verify(condition, ...) if (!(condition)) internal_error(__VA_ARGS__)

#define todo(...) todo_impl(__FILE__, __LINE__, __VA_ARGS__)

[[noreturn]]
void todo_impl(const char *file, size_t line, const char *fmt, va_list args) {
  fprintf(stderr, "%sTODO @ %s:%zu: ", Color::Yellow, file, line);
  vfprintf(stderr, fmt, args);
  fprintf(stderr, "%s\n", Color::Reset);

  va_end(args);
  exit(EXIT_FAILURE);
}

[[noreturn]]
void todo_impl(const char *file, size_t line, const char *fmt, ...) {
  va_list args;
  va_start(args, fmt);

  todo_impl(file, line, fmt, args);
}

//
//
// Type
//
//

enum class Type_Kind {
  No_Type,
  Null,
  Boolean,
  Character,
  Integer,
  Floating_Point,
  String,
};

union Type_Data {

};

struct Type {
  Type_Kind kind;
  Type_Data data;

  std::string debug_str() const {
    std::string s;

    switch (kind) {
      case Type_Kind::No_Type: {
        s = "No_Type";
      } break;
      case Type_Kind::Null: {
        s = "Null";
      } break;
      case Type_Kind::Boolean: {
        s = "Boolean";
      } break;
      case Type_Kind::Character: {
        s = "Character";
      } break;
      case Type_Kind::Integer: {
        s = "Integer";
      } break;
      case Type_Kind::Floating_Point: {
        s = "Floating_Point";
      } break;
      case Type_Kind::String: {
        s = "String";
      } break;

      default:
        internal_error("Invalid Type_Kind: %d!", kind);
    }

    return s;
  }
};

//
//
// AST
//
//

enum class AST_Kind {
  Symbol_Identifier,

  Literal_Null,
  Literal_Boolean,
  Literal_Character,
  Literal_Integer,
  Literal_Floating_Point,
  Literal_String,

  Unary,
  Binary_Add,
};

const char *debug_str(AST_Kind kind) {
  #define CASE(kind) case AST_Kind::kind: return #kind

  switch (kind) {
    CASE(Symbol_Identifier);
    CASE(Literal_Null);
    CASE(Literal_Boolean);
    CASE(Literal_Character);
    CASE(Literal_Integer);
    CASE(Literal_Floating_Point);
    CASE(Unary);
    CASE(Binary_Add);

    default:
      internal_error("Invalid AST_Kind: %d!", kind);
  }

  #undef CASE
}

struct AST {
  AST_Kind kind;
  std::optional<Type> type;
  Code_Location location;

  virtual ~AST() {}

  void debug_print(size_t indentation = 0) const;

protected:
  void print_base_members(size_t indentation) const {
    printf("%*skind: %s\n", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", debug_str(kind));
    printf("%*stype: %s\n", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", debug_str(type).c_str());
    printf("%*slocation: %s\n", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", location.debug_str().c_str());
  }

  void print_member(const char *name, size_t indentation, AST *member) const {
    printf("%*s%s: ", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", name);
    member->debug_print(indentation + 1);
  }

  void print_unary(const struct AST_Unary *unary, size_t indentation) const;
  void print_binary(const struct AST_Binary *binary, size_t indentation) const;
};

struct AST_Symbol : AST {
  String symbol;
};

struct AST_Literal : AST {
  union {
	  bool boolean;
	  char32_t character;
	  int64_t integer;
	  double floating_point;
	  String string;
  } as;
};

struct AST_Unary : AST {
  AST *sub;
};

struct AST_Binary : AST {
  AST *lhs;
  AST *rhs;
};

void AST::debug_print(size_t indentation) const {
  switch (kind) {
    case AST_Kind::Symbol_Identifier: {
      const AST_Symbol *self = dynamic_cast<const AST_Symbol *>(this);
      internal_verify(self, "Failed to cast to `const AST_Symbol *`");

      printf("`Symbol_Identifier`:\n");
      print_base_members(indentation);

      printf("%*sid: `%.*s`\n",
        static_cast<int>(Print_Indentation_Size * (indentation + 1)), "",
        static_cast<int>(self->symbol.size), self->symbol.data
      );
    } break;
    case AST_Kind::Literal_Null: {
      const AST_Literal *self = dynamic_cast<const AST_Literal *>(this);
      internal_verify(self, "Failed to cast to `const AST_Literal *`");

      printf("`Literal_Null`:\n");
      print_base_members(indentation);
    } break;
    case AST_Kind::Literal_Boolean: {
      const AST_Literal *self = dynamic_cast<const AST_Literal *>(this);
      internal_verify(self, "Failed to cast to `const AST_Literal *`");

      printf("`Literal_Boolean`:\n");
      print_base_members(indentation);

      printf("%*svalue: %s\n",
        static_cast<int>(Print_Indentation_Size * (indentation + 1)), "",
        self->as.boolean ? "true" : "false"
      );
    } break;
    case AST_Kind::Literal_Character: {
      const AST_Literal *self = dynamic_cast<const AST_Literal *>(this);
      internal_verify(self, "Failed to cast to `const AST_Literal *`");

      printf("`Literal_Character`:\n");
      print_base_members(indentation);

      // @TOOD:
      // :HandleUTF8
      //
      printf("%*svalue: %c\n",
        static_cast<int>(Print_Indentation_Size * (indentation + 1)), "",
        self->as.character
      );
    } break;
    case AST_Kind::Literal_Integer: {
      const AST_Literal *self = dynamic_cast<const AST_Literal *>(this);
      internal_verify(self, "Failed to cast to `const AST_Literal *`");

      printf("`Literal_Integer`:\n");
      print_base_members(indentation);

      printf("%*svalue: %lld\n", 
        static_cast<int>(Print_Indentation_Size * (indentation + 1)), "",
        self->as.integer
      );
    } break;
    case AST_Kind::Literal_Floating_Point: {
      const AST_Literal *self = dynamic_cast<const AST_Literal *>(this);
      internal_verify(self, "Failed to cast to `const AST_Literal *`");

      printf("`Literal_Floating_Point`:\n");
      print_base_members(indentation);

      printf("%*svalue: %f\n", 
        static_cast<int>(Print_Indentation_Size * (indentation + 1)), "",
        self->as.floating_point
      );
    } break;
    case AST_Kind::Unary: {
      const AST_Unary *self = dynamic_cast<const AST_Unary *>(this);
      internal_verify(self, "Failed to cast to `const AST_Unary *`");

      printf("Unary:\n");
      print_unary(self, indentation);
    } break;
    case AST_Kind::Binary_Add: {
      const AST_Binary *self = dynamic_cast<const AST_Binary *>(this);
      internal_verify(self, "Failed to cast to `const AST_Binary *`");

      printf("`+`:\n");
      print_binary(self, indentation);
    } break;

    default:
      internal_error("Invalid AST_Kind: %d!", kind);
  }
}

void AST::print_unary(const AST_Unary *unary, size_t indentation) const {
  print_base_members(indentation);
  print_member("sub", indentation, unary->sub);
}

void AST::print_binary(const AST_Binary *binary, size_t indentation) const {
  print_base_members(indentation);
  print_member("lhs", indentation, binary->lhs);
  print_member("rhs", indentation, binary->rhs);
}

//
//
// Parser
//
//

enum class Token_Precedence {
  None,
  Assignment, // = += -= *= /= &= etc.
  Colon,      // :
  Cast,       // as
  Range,      // .. ...
  Or,         // ||
  And,        // &&
  BitOr,      // |
  Xor,        // ^
  BitAnd,     // &
  Equality,   // == !=
  Comparison, // < > <= >=
  Shift,      // << >>
  Term,       // + -
  Factor,     // * / %
  Unary,      // !
  Call,       // . () []
  Primary,
};

const char *debug_str(Token_Precedence precedence) {
  #define CASE(precedence) case Token_Precedence::precedence: return #precedence

  switch (precedence) {
    CASE(None);
    CASE(Assignment);
    CASE(Colon);
    CASE(Cast);
    CASE(Range);
    CASE(Or);         
    CASE(And);        
    CASE(BitOr);      
    CASE(Xor);        
    CASE(BitAnd);     
    CASE(Equality);   
    CASE(Comparison); 
    CASE(Shift);      
    CASE(Term);       
    CASE(Factor);     
    CASE(Unary);      
    CASE(Call);       
    CASE(Primary);

    default:
      internal_error("Invalid Token_Precedence: %d!", precedence);
  }

  #undef CASE
}

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

const char *debug_str(Token_Kind kind) {
  #define CASE(kind) case Token_Kind::kind: return #kind

  switch (kind) {
  	CASE(Eof);

	  // literals
	  CASE(Literal_Null);
	  CASE(Literal_True);
	  CASE(Literal_False);
	  CASE(Literal_Boolean);
	  CASE(Literal_Character);
	  CASE(Literal_Integer);
	  CASE(Literal_Floating_Point);
	  CASE(Literal_String);

	  // symbols
	  CASE(Symbol_Identifier);

	  // delimeters
	  CASE(Delimeter_Newline);
	  CASE(Delimeter_Semicolon);
	  CASE(Delimeter_Left_Parenthesis);
	  CASE(Delimeter_Right_Parenthesis);

	  // punctuation
	  CASE(Punctuation_Equal);
	  CASE(Punctuation_Colon);
	  CASE(Punctuation_Plus);
	  CASE(Punctuation_Dash);
	  CASE(Punctuation_Star);
	  CASE(Punctuation_Slash);

	  // keywords  

    default:
      internal_error("Invalid Token_Kind: %d!", kind);
  }

  #undef CASE
}

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

  Token_Precedence precedence() const {
    #define CASE(kind, prec) case Token_Kind::kind: return Token_Precedence::prec

    switch (kind) {
      CASE(Eof, None);

	    // literals
	    CASE(Literal_Null, None);
	    CASE(Literal_True, None);
	    CASE(Literal_False, None);
	    CASE(Literal_Boolean, None);
	    CASE(Literal_Character, None);
	    CASE(Literal_Integer, None);
	    CASE(Literal_Floating_Point, None);
	    CASE(Literal_String, None);

	    // symbols
	    CASE(Symbol_Identifier, None);

	    // delimeters
	    CASE(Delimeter_Newline, None);
	    CASE(Delimeter_Semicolon, None);
	    CASE(Delimeter_Left_Parenthesis, Call);
	    CASE(Delimeter_Right_Parenthesis, None);

	    // punctuation
	    CASE(Punctuation_Equal, Assignment);
	    CASE(Punctuation_Colon, Colon);
	    CASE(Punctuation_Plus, Term);
	    CASE(Punctuation_Dash, Term);
	    CASE(Punctuation_Star, Factor);
	    CASE(Punctuation_Slash, Factor);

	    // keywords

      default:
        internal_error("Invalid Token_Kind: %d!", kind);
    }

    #undef CASE
  }
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

    // @TODO:
    // :HandleUTF8
    //
		verify(isalnum(character) || ispunct(character) || isspace(character), current_location(), "Invalid character in character literal `%c`.", character);
		verify(next_char() == '\'', current_location(), "Expected a single-quote `'`' to terminate character literal.");
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

    bool is_floating_point = false;
		char *start = source.data - 1;
		size_t size = 1;

		while (isdigit(peek_char())) {
			size++;
			next_char();
		}

    if (peek_char() == '.' && isdigit(peek_char(1))) {
      is_floating_point = true;
      next_char();
      size++;

      while (isdigit(peek_char())) {
        size++;
        next_char();
      }
    }

		char *word = reinterpret_cast<char *>(alloca(size));
		memcpy(word, start, size);

    printf("word=%s\n", word);

    Token token;

    if (is_floating_point) {
      double number = atof(word);
      token = make_token(
        Token_Kind::Literal_Floating_Point,
        Token_Data {
          .floating_point = number
        }
      );
    } else {
		  int64_t number = atoll(word);
		  token = make_token(
		  	Token_Kind::Literal_Integer,
		  	Token_Data { 
		  		.integer = number
		  	}
		  );
    }

    return token;
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
			todo("Keywords not implemented!");
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
        error(current_location(), "Unknown operator `%c`.", c);
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
	String source = const_cast<char *>("a + 1.3");

	Tokenizer tokenizer;
	tokenizer.source = source;

	Token t1 = tokenizer.next();
	Token t2 = tokenizer.next();
	Token t3 = tokenizer.next();
	Token t4 = tokenizer.next();  

	{
		std::cout << Color::Cyan << "t1:" << Color::Reset << std::endl;
		
		internal_verify(t1.kind == Token_Kind::Symbol_Identifier, "Unexpected kind: %s!", debug_str(t1.kind));
		std::cout << "\tkind: Symbol_Identifier" << std::endl;

		internal_verify(t1.data.string == "a", "Unexpected value: \"%.*s\"", t1.data.string.size, t1.data.string.data);
		std::cout << "\tdata.string: \"a\"" << std::endl;

    internal_verify(t1.precedence() == Token_Precedence::None, "Unexpected precedence: %s!", debug_str(t1.precedence()));
    std::cout << "\tprecedence: None" << std::endl;

		std::cout << "\tlocation: Ln " << t1.location.l0 << ", Col " << t1.location.c0 << std::endl;
	}

	{
		std::cout << Color::Cyan << "t2:" << Color::Reset << std::endl;
		
		internal_verify(t2.kind == Token_Kind::Punctuation_Plus, "Unexpected kind: %s!", debug_str(t2.kind));
		std::cout << "\tkind: Punctuation_Plus" << std::endl;

    internal_verify(t2.precedence() == Token_Precedence::Term, "Unexpected precedence: %s!", debug_str(t2.precedence()));
    std::cout << "\tprecedence: Term" << std::endl;

		std::cout << "\tlocation: Ln " << t2.location.l0 << ", Col " << t2.location.c0 << std::endl;
	}

	{
		std::cout << Color::Cyan << "t3:" << Color::Reset << std::endl;
		
		internal_verify(t3.kind == Token_Kind::Literal_Floating_Point, "Unexpected kind: %s!", debug_str(t3.kind));
		std::cout << "\tkind: Literal_Floating_Point" << std::endl;

		internal_verify(t3.data.floating_point == 1.3, "Unexpected value: %f!", t3.data.floating_point);
		std::cout << "\tdata.floating_point: 1.3" << std::endl;

    internal_verify(t3.precedence() == Token_Precedence::None, "Unexpected precedence: %s!", debug_str(t3.precedence()));
    std::cout << "\tprecedence: None" << std::endl;

		std::cout << "\tlocation: Ln " << t3.location.l0 << ", Col " << t3.location.c0 << std::endl;
	}
	
	internal_verify(t4.kind == Token_Kind::Eof, "Unexpected kind: %s!", debug_str(t4.kind));
  internal_verify(t4.precedence() == Token_Precedence::None, "Unexpected precedence: %s!", debug_str(t4.precedence()));

  {
    AST_Symbol symbol;
    symbol.kind = AST_Kind::Symbol_Identifier;
    symbol.type = {};
    symbol.location = t1.location;
    symbol.symbol = t1.data.string;

    AST_Literal literal;
    literal.kind = AST_Kind::Literal_Floating_Point;
    literal.type = Type { .kind = Type_Kind::Floating_Point };
    literal.location = t3.location;
    literal.as.floating_point = t3.data.floating_point;

    AST_Binary add;
    add.kind = AST_Kind::Binary_Add;
    add.location = t2.location;
    add.lhs = &symbol;
    add.rhs = &literal;

    add.debug_print();
  }

	std::cout << Color::Green << "SUCCESS!" << Color::Reset << std::endl;

	return 0;
}