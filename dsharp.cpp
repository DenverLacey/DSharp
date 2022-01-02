#include <iostream>
#include <optional>
#include <assert.h>
#include <string>
#include <variant>
#include <vector>
#include <sstream>
#include <fstream>
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

  void free() {
    ::free(data);
    size = 0;
    data = nullptr;
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

template<typename Ok>
class Result {
  std::variant<Ok, std::string> repr;

public:
  Result(const Ok& ok) {
    repr = ok;
  }

  Result(std::string err) {
    repr = std::move(err);
  }

  Ok unwrap() {
    if (std::holds_alternative<std::string>(repr)) {
      std::cerr << std::get<std::string>(repr);
      exit(EXIT_FAILURE);
    }
    return std::get<Ok>(repr);
  }

  bool is_ok() {
    return std::holds_alternative<Ok>(repr);
  }

  bool is_err() {
    return std::holds_alternative<std::string>(repr);
  }

  Ok ok() {
    return std::get<Ok>(repr);
  }

  const std::string& err() {
    return std::get<std::string>(repr);
  }
};

template<>
class Result<void> {
  bool _is_ok;
  std::string _err;

public:
  Result() : _is_ok(true) { }

  Result(std::string err) : _is_ok(false) {
    _err = std::move(err);
  }

  void unwrap() {
    if (!_is_ok) {
      std::cerr << _err;
      exit(EXIT_FAILURE);
    }
  }

  bool is_ok() {
    return _is_ok;
  }

  bool is_err() {
    return !_is_ok;
  }

  const std::string& err() {
    return _err;
  }
};

#define try_(expression) ({\
  auto result = expression;\
  if (result.is_err()) return { std::move(result.err()) };\
  result.ok();\
})

#define error(...) ({\
  auto result = error_impl(__VA_ARGS__);\
  assert(result.is_err());\
  return { std::move(result.err()) };\
})

Result<void> error_impl(const char *err, va_list args) {
  std::stringstream s;

  char *message;
  vasprintf(&message, err, args);

  s << Color::Red << "Error: " << message << Color::Reset << std::endl;

  free(message);
  va_end(args);

  return { s.str() };
}

Result<void> error_impl(const char *err, ...) {
  va_list args;
  va_start(args, err);

  return error_impl(err, args);
}

Result<void> error_impl(Code_Location location, const char *err, va_list args) {
  std::stringstream s;

  char *message;
  vasprintf(&message, err, args);

  s << Color::Red << "Error @ " << location.debug_str() << ": " << message << Color::Reset << std::endl;

  free(message);
  va_end(args);

  return { s.str() };
}

Result<void> error_impl(Code_Location location, const char *err, ...) {
  va_list args;
  va_start(args, err);

  return error_impl(location, err, args);
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
        s = std::to_string(static_cast<int>(kind));
        break;
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

  Binary_Variable_Instantiation,
  Binary_Constant_Instantiation,
  Binary_Variable_Declaration,
  Binary_Assignment,
  Binary_Add,

  Block,

  If,
};

std::string debug_str(AST_Kind kind) {
  #define CASE(kind) case AST_Kind::kind: return #kind

  switch (kind) {
    CASE(Symbol_Identifier);
    CASE(Literal_Null);
    CASE(Literal_Boolean);
    CASE(Literal_Character);
    CASE(Literal_Integer);
    CASE(Literal_Floating_Point);
    CASE(Unary);
    CASE(Binary_Variable_Instantiation);
    CASE(Binary_Constant_Instantiation);
    CASE(Binary_Variable_Declaration);
    CASE(Binary_Assignment);
    CASE(Binary_Add);
    CASE(Block);
    CASE(If);

    default:
      return std::to_string(static_cast<int>(kind));
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
    printf("%*skind: %s\n", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", debug_str(kind).c_str());
    printf("%*stype: %s\n", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", debug_str(type).c_str());
    printf("%*slocation: %s\n", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", location.debug_str().c_str());
  }

  void print_member(const char *name, size_t indentation, AST *member) const {
    printf("%*s%s: ", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", name);
    member->debug_print(indentation + 1);
  }

  void print_unary(const struct AST_Unary *unary, size_t indentation) const;
  void print_binary(const struct AST_Binary *binary, size_t indentation) const;
  void print_block(const struct AST_Block *block, size_t indentation) const;
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

struct AST_Block : AST {
  std::vector<AST *> nodes;
};

struct AST_If : AST {
  AST *condition;
  AST *then_block;
  AST *else_block;
};

void AST::debug_print(size_t indentation) const {
  #define CASE_UNARY(kind, name) case AST_Kind::kind: {\
    const AST_Unary *self = dynamic_cast<const AST_Unary *>(this);\
    internal_verify(self, "Failed to cast to `const AST_Unary *`");\
    printf("`" name "`:\n");\
    print_unary(self, indentation);\
  } break
  
  #define CASE_BINARY(kind, name) case AST_Kind::kind: {\
    const AST_Binary *self = dynamic_cast<const AST_Binary *>(this);\
    internal_verify(self, "Faield to cast to `const AST_Binary *`");\
    printf("`" name "`:\n");\
    print_binary(self, indentation);\
  } break

  #define CASE_BLOCK(kind, name) case AST_Kind::kind: {\
    const AST_Block *self = dynamic_cast<const AST_Block *>(this);\
    internal_verify(self, "Failed to cast to `const AST_Block *`");\
    printf("`" name "`:\n");\
    print_block(self, indentation);\
  } break

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

    CASE_UNARY(Unary, "Unary");
    
    CASE_BINARY(Binary_Variable_Instantiation, ":=");
    CASE_BINARY(Binary_Constant_Instantiation, "::");
    CASE_BINARY(Binary_Variable_Declaration, ":");
    CASE_BINARY(Binary_Assignment, "=");
    CASE_BINARY(Binary_Add, "+");

    CASE_BLOCK(Block, "{}");

    case AST_Kind::If: {
      const AST_If *self = dynamic_cast<const AST_If *>(this);
      internal_verify(self, "Failed to cast to `const AST_If *`");

      printf("`if`:\n");
      print_base_members(indentation);

      print_member("condition", indentation, self->condition);
      print_member("then", indentation, self->then_block);
      if (self->else_block) print_member("else", indentation, self->else_block);
    } break;
    
    default:
      internal_error("Unhandled AST_Kind: %s!", debug_str(kind).c_str());
  }

  #undef CASE_UNARY
  #undef CASE_BINARY
  #undef CASE_BLOCK
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

void AST::print_block(const AST_Block *block, size_t indentation) const {
  print_base_members(indentation);

  for (size_t i = 0; i < block->nodes.size(); i++) {
    printf("%*s%zu: ", 
      static_cast<int>(Print_Indentation_Size * (indentation + 1)), "",
      i
    );
    block->nodes[i]->debug_print(indentation + 1);
  }
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

std::string debug_str(Token_Precedence precedence) {
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
      return std::to_string(static_cast<int>(precedence));
  }

  #undef CASE
}

Token_Precedence operator+(Token_Precedence p, int step) {
    auto q = static_cast<int>(p) + step;
    q = std::clamp(q, static_cast<int>(Token_Precedence::None), static_cast<int>(Token_Precedence::Primary));
    return static_cast<Token_Precedence>(q);
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
  Delimeter_Left_Curly,
  Delimeter_Right_Curly,

	// punctuation
	Punctuation_Equal,
	Punctuation_Colon,
	Punctuation_Plus,
	Punctuation_Dash,
	Punctuation_Star,
	Punctuation_Slash,

	// keywords
  Keyword_If,
  Keyword_Else,
};

std::string debug_str(Token_Kind kind) {
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
    CASE(Delimeter_Left_Curly);
	  CASE(Delimeter_Right_Curly);

	  // punctuation
	  CASE(Punctuation_Equal);
	  CASE(Punctuation_Colon);
	  CASE(Punctuation_Plus);
	  CASE(Punctuation_Dash);
	  CASE(Punctuation_Star);
	  CASE(Punctuation_Slash);

	  // keywords  

    default:
      return std::to_string(static_cast<int>(kind));
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
      CASE(Delimeter_Left_Curly, None);
	    CASE(Delimeter_Right_Curly, None);

	    // punctuation
	    CASE(Punctuation_Equal, Assignment);
	    CASE(Punctuation_Colon, Colon);
	    CASE(Punctuation_Plus, Term);
	    CASE(Punctuation_Dash, Term);
	    CASE(Punctuation_Star, Factor);
	    CASE(Punctuation_Slash, Factor);

	    // keywords
      CASE(Keyword_If, None);
      CASE(Keyword_Else, None);

      default:
        internal_error("Unhandled Token_Kind: %s!", debug_str(kind).c_str());
    }

    #undef CASE
  }
};

struct Tokenizer {
	size_t line = 0;
	size_t coloumn = 0;
  const char *filename;
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
      filename,
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
		char c = next_char();

		while (is_whitespace(c)) {
			if (c == '/' && peek_char(1) == '/') {
				while (c != '\n') {
					c = next_char();
				}
			}

			if (c == '\n' && previous_token.kind == Token_Kind::Delimeter_Newline) {
				while (c == '\n') {
					c = next_char();
					line++;
          coloumn = 0;
				}
			}

			c = next_char();
		}

		return c;
	}

	Result<Token> peek() {
		if (peeked_token.has_value()) {
			return peeked_token.value();
		}
		peeked_token = try_(next());
		return peeked_token.value();
	}

	Result<Token> next() {
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
      coloumn = 0;
		} else if (c == '\'') {
			previous_token = try_(next_character_token());
		} else if (c == '\"') {
			previous_token = next_string_token();
		} else if (isdigit(c) || (c == '.' && isdigit(peek_char()))) {
			previous_token = next_number_token();
		} else if (isalpha(c) || (c == '_' && is_identifier_character(peek_char()))) {
			previous_token = next_keyword_or_identifier_token();
		} else {
			previous_token = try_(next_punctuation_token(c));
		}

		return previous_token;
	}

	Result<Token> next_character_token() {
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
		bool is_floating_point = false;

    // @NOTE:
		// This is because we've already moved passed the first character. 
		// This should be dealt with more explicitly or by reworking other stuff.
		//
    // @TODO:
		// :HandleUTF8
		//
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
    // @HACK:
    // `source.data - 1` because we moved passed the first character in the symbol.
    //
		auto word = String { 1, source.data - 1 };

		while (is_identifier_character(peek_char())) {
			word.size++;
			next_char();
		}

		Token token;
    if (word == "null") {
      token = make_token(Token_Kind::Literal_Null);
    } else if (word == "true") {
        token = make_token(
          Token_Kind::Literal_Boolean,
          Token_Data {
            .boolean = true
          }
        );
    } else if (word == "false") {
      token = make_token(
          Token_Kind::Literal_Boolean,
          Token_Data {
            .boolean = false
          }
        );
    } else if (word == "if") {
      token = make_token(Token_Kind::Keyword_If);
		} else if (word == "else") {
      token = make_token(Token_Kind::Keyword_Else);
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

	Result<Token> next_punctuation_token(char32_t c) {
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
      case '{': {
        token = make_token(Token_Kind::Delimeter_Left_Curly);
      } break;
      case '}': {
        token = make_token(Token_Kind::Delimeter_Right_Curly);
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

struct Parser {
  Tokenizer tokenizer;

  bool check(Token_Kind kind) {
    // @NOTE: 
    // Maybe `check()` and `match()` should return `Result<bool>`s or something
    //
    return tokenizer.peek().unwrap().kind == kind;
  }

  bool skip_check(Token_Kind kind) {
    skip_newlines();
    return check(kind);
  }

  // @NOTE: See above!
  //
  bool match(Token_Kind kind) {
    if (check(kind)) {
      tokenizer.next().unwrap();
      return true;
    }
    return false;
  }

  bool skip_match(Token_Kind kind) {
    skip_newlines();
    return match(kind);
  }

  Result<Token> expect(Token_Kind kind, const char *err, ...) {
    va_list args;
    va_start(args, err);
    return expect(kind, err, args);
  }

  Result<Token> expect(Token_Kind kind, const char *err, va_list args) {
    auto t = try_(tokenizer.next());
    verify(t.kind == kind, t.location, err, args);

    va_end(args);
    return t;
  }

  Result<Token> skip_expect(Token_Kind kind, const char *err, ...) {
    va_list args;
    va_start(args, err);

    skip_newlines();
    return expect(kind, err, args);
  }

  Result<Token> expect_statement_terminator(const char *err, ...) {
    va_list args;
    va_start(args, err);

    auto t = try_(tokenizer.next());
    verify(
      t.kind == Token_Kind::Delimeter_Newline || t.kind == Token_Kind::Delimeter_Semicolon || t.kind == Token_Kind::Eof,
      t.location, 
      err, 
      args
    );
    
    va_end(args);
    return t;
  }

  void skip_newlines() {
    while (match(Token_Kind::Delimeter_Newline)) {}
  }

  Result<AST *> parse_declaration() {
    AST *node = nullptr;
    
    if (false) {
      todo("This is where declaration parsing functions will go.");
    } else {
      node = try_(parse_statement());
    }

    return node;
  }

  Result<AST *> parse_statement() {
    AST *node = nullptr;

    if (check(Token_Kind::Delimeter_Left_Curly)) {
      node = try_(parse_block());
    } else if (check(Token_Kind::Keyword_If)) {
      node = try_(parse_if_statement());
    } else {
      node = try_(parse_expression_or_assignment());
      try_(expect_statement_terminator("Expected end of statement!"));
    }

    return node;
  }

  Result<AST_If *> parse_if_statement() {
    auto location = try_(skip_expect(Token_Kind::Keyword_If, "Expected `if` statement!")).location;
    skip_newlines();
    
    auto condition = try_(parse_expression());

    auto then_block = try_(parse_block());

    AST *else_block = nullptr;
    if (skip_match(Token_Kind::Keyword_Else)) {
      if (skip_check(Token_Kind::Keyword_If)) {
        else_block = try_(parse_if_statement());
      } else {
        skip_newlines();
        else_block = try_(parse_block());
      }
    }

    AST_If *node = new AST_If;
    node->kind = AST_Kind::If;
    node->location = location;
    node->condition = condition;
    node->then_block = then_block;
    node->else_block = else_block;

    return node;
  }

  Result<AST *> parse_expression_or_assignment() {
    return parse_precedence(Token_Precedence::Assignment);
  }

  Result<AST *> parse_expression() {
    auto expression = try_(parse_expression_or_assignment());

    verify(expression->kind != AST_Kind::Binary_Assignment, expression->location, "Cannot assign in expression context.");
    verify(expression->kind != AST_Kind::Binary_Variable_Instantiation, expression->location, "Cannot instantiate new variables in expression context.");
    verify(expression->kind != AST_Kind::Binary_Constant_Instantiation, expression->location, "Cannot instantiate new constants in expression context.");
    verify(expression->kind != AST_Kind::Binary_Variable_Declaration, expression->location, "Cannot declare new variables in expression context.");
    
    return expression;
  }

  Result<AST *> parse_precedence(Token_Precedence precedence) {
    Token token = try_(tokenizer.next());
    verify(token.kind != Token_Kind::Eof, token.location, "Unexpected end of input!");

    auto previous = try_(parse_prefix(token));
    internal_verify(previous, "`parse_prefix()` returned null!");

    while (precedence <= try_(tokenizer.peek()).precedence()) {
      Token token = try_(tokenizer.next());
      previous = try_(parse_infix(token, previous));
      internal_verify(previous, "`parse_infix()` returned null!");
    }

    return previous;
  }

  Result<AST *> parse_prefix(Token token) {
    AST *node = nullptr;

    switch (token.kind) {
      case Token_Kind::Symbol_Identifier: {
        AST_Symbol *identifier = new AST_Symbol;
        identifier->kind = AST_Kind::Symbol_Identifier;
        identifier->location = token.location;

        // @TODO:
        // :HandleStringData
        //
        identifier->symbol = token.data.string;

        node = identifier;
      } break;
      case Token_Kind::Literal_Null: {
        AST_Literal *literal = new AST_Literal;
        literal->kind = AST_Kind::Literal_Null;
        literal->location = token.location;

        node = literal;
      } break;
      case Token_Kind::Literal_Boolean: {
        AST_Literal *literal = new AST_Literal;
        literal->kind = AST_Kind::Literal_Boolean;
        literal->location = token.location;
        literal->as.boolean = token.data.boolean;

        node = literal;
      } break;
      case Token_Kind::Literal_Character: {
        AST_Literal *literal = new AST_Literal;
        literal->kind = AST_Kind::Literal_Character;
        literal->location = token.location;
        literal->as.character = token.data.character;

        node = literal;
      } break;
      case Token_Kind::Literal_Integer: {
        AST_Literal *literal = new AST_Literal;
        literal->kind = AST_Kind::Literal_Integer;
        literal->location = token.location;
        literal->as.integer = token.data.integer;

        node = literal;
      } break;
      case Token_Kind::Literal_Floating_Point: {
        AST_Literal *literal = new AST_Literal;
        literal->kind = AST_Kind::Literal_Floating_Point;
        literal->location = token.location;
        literal->as.floating_point = token.data.floating_point;

        node = literal;
      } break;
      case Token_Kind::Literal_String: {
        AST_Literal *literal = new AST_Literal;
        literal->kind = AST_Kind::Literal_String;
        literal->location = token.location;

        // @TODO:
        // :HandleStringData
        //
        literal->as.string = token.data.string;

        node = literal;
      } break;

      default:
        error(token.location, "Unexpected type of expression!");
    }

    return node;
  }

  Result<AST *> parse_infix(Token token, AST *previous) {
    AST *node = nullptr;
    auto precedence = token.precedence();
    auto location = token.location;

    switch (token.kind) {
      case Token_Kind::Punctuation_Colon:
        node = try_(parse_colon(previous, location));
        break;
      case Token_Kind::Punctuation_Equal:
        node = try_(parse_binary(AST_Kind::Binary_Assignment, precedence, previous, location));
        break;
      case Token_Kind::Punctuation_Plus:
        node = try_(parse_binary(AST_Kind::Binary_Add, precedence, previous, location));
        break;
      case Token_Kind::Punctuation_Dash:
        todo("Not yet implemented!");
        node = try_(parse_binary(AST_Kind::Binary_Add, precedence, previous, location));
        break;
      case Token_Kind::Punctuation_Star:
        todo("Not yet implemented!");
        node = try_(parse_binary(AST_Kind::Binary_Add, precedence, previous, location));
        break;
      case Token_Kind::Punctuation_Slash:
        todo("Not yet implemented!");
        node = try_(parse_binary(AST_Kind::Binary_Add, precedence, previous, location));
        break;

      default:
        error(location, "Unexpected type of expression within a greater expression!");
    }

    return node;
  }

  Result<AST *> parse_unary(AST_Kind kind, Code_Location location) {
    skip_newlines();

    auto sub = try_(parse_precedence(Token_Precedence::Unary));

    auto unary = new AST_Unary;
    unary->kind = kind;
    unary->location = location;
    unary->sub = sub;

    return unary;
  }

  Result<AST *> parse_binary(AST_Kind kind, Token_Precedence precedence, AST *lhs, Code_Location location) {
    skip_newlines();

    auto rhs = try_(parse_precedence(precedence + 1));

    auto binary = new AST_Binary;
    binary->kind = kind;
    binary->location = location;
    binary->lhs = lhs;
    binary->rhs = rhs;

    return binary;
  }

  Result<AST_Block *> parse_block() {
    auto location = try_(skip_expect(Token_Kind::Delimeter_Left_Curly, "Expected `{` to begin block!")).location;

    auto block = new AST_Block;
    block->kind = AST_Kind::Block;

    while (true) {
      skip_newlines();
      if (check(Token_Kind::Delimeter_Right_Curly) || check(Token_Kind::Eof)) break;

      auto node = try_(parse_declaration());
      block->nodes.push_back(node);
    }

    auto right_curly = try_(skip_expect(Token_Kind::Delimeter_Right_Curly, "Expected `}` to terminate block!"));
    block->location = location;

    return block;
  }

  Result<AST *> parse_colon(AST *previous, Code_Location location) {
    AST *node = nullptr;

    if (match(Token_Kind::Punctuation_Equal)) {
      auto rhs = try_(parse_expression());

      AST_Binary *binary = new AST_Binary;
      binary->kind = AST_Kind::Binary_Variable_Instantiation;
      binary->location = location;
      binary->lhs = previous;
      binary->rhs = rhs;

      node = binary;
    } else if (match(Token_Kind::Punctuation_Colon)) {
      auto rhs = try_(parse_expression());

      AST_Binary *binary = new AST_Binary;
      binary->kind = AST_Kind::Binary_Constant_Instantiation;
      binary->location = location;
      binary->lhs = previous;
      binary->rhs = rhs;

      node = binary;
    } else {
      todo("Variable declarations not yet implemented.");
    }

    return node;
  }
};

AST *parse(String source, const char *filename) {
  Parser p;
  p.tokenizer.source = source;
  p.tokenizer.filename = filename;

  while (true) {
    p.skip_newlines();
    if (p.check(Token_Kind::Eof)) break;

    auto result = p.parse_declaration();
    if (result.is_err()) {
      std::cerr << result.err();
    } else {
      auto node = result.ok();
      node->debug_print();
    }
  }

  return nullptr;
}

//
//
// Entry Point
//
//

Result<String> read_entire_file(const char *path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  verify(file.is_open(), "'%s' could not be opened.", path);
  
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  
  auto source = String { static_cast<size_t>(size), reinterpret_cast<char *>(malloc(size)) };
  file.read(source.data, size);
  verify(file.good(), "Could not read from '%s'.", path);
  
  return source;
}

int main(int argc, const char **argv) {
  if (argc <= 1) {
    std::cerr << "Please provide source file to compile." << std::endl;
    return EXIT_FAILURE;
  }

  const char *filename = argv[1];

  String source = read_entire_file(filename).unwrap();
  parse(source, filename);

  source.free();
	return 0;
}
