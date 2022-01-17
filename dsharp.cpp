#include <iostream>
#include <optional>
#include <assert.h>
#include <string>
#include <variant>
#include <vector>
#include <sstream>
#include <fstream>
#include <stdarg.h>
#include <limits>
#include <forward_list>
#include <unordered_map>

//
//
// TODOS:
// - HandleUTF8
//		Tokenizer currently only really supports ASCII.
// - HandleStringData
//		Identifier and string literal Tokens currently point to the source code.
// - TypeEquality
//		Implement proper equality checks for `Type`s.
// - CalculateNumericSize
//		Calculate floating point literals required size to determine int type.
// - ImplementParseTypeSignature
//		Implement proper `parse_type_signature()` function for `Parser`.
//
//

//
//
// typedefs
//
//

using PID = size_t;
using Size = size_t;
using Address = uint16_t;

namespace Runtime_Type {
	using Boolean = bool;

	using Character = char32_t;

	using Integer8  = int8_t;
	using Integer16 = int16_t;
	using Integer32 = int32_t;
	using Integer64 = int64_t;

	using Floating_Point32 = float;
	using Floating_Point64 = double;

	struct String {
		Integer64 size;
		char *chars;
	};
};

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

Size minimum_required_size_for_literal(int64_t value) {
	if (value <= std::numeric_limits<Runtime_Type::Integer8>::max() && value >= std::numeric_limits<Runtime_Type::Integer8>::min()) return sizeof(Runtime_Type::Integer8);
	if (value <= std::numeric_limits<Runtime_Type::Integer16>::max() && value >= std::numeric_limits<Runtime_Type::Integer16>::min()) return sizeof(Runtime_Type::Integer16);
	if (value <= std::numeric_limits<Runtime_Type::Integer32>::max() && value >= std::numeric_limits<Runtime_Type::Integer32>::min()) return sizeof(Runtime_Type::Integer32);
	return sizeof(Runtime_Type::Integer64);
}

Size minimum_required_size_for_literal(double value) {
	// @TODO:
	// :CalculateNumericSize
	//

	// if (value <= std::numeric_limits<Runtime_Type::Floating_Point32>::max()) return sizeof(Runtime_Type::Floating_Point32);
	return sizeof(Runtime_Type::Floating_Point64);
}

//
//
// Helper Data Structures
//
//

struct String {
	size_t size;
	char *chars;

	String() = default;

	String(size_t size, char *chars) :
		size(size),
		chars(chars)
	{
	}

	String(char *str) : 
		size(strlen(str)),
		chars(str)
	{
	}

	char advance() {
		char c = *chars;
		
		chars++;
		size--;

		return c;
	}

	void free() {
		::free(chars);
		size = 0;
		chars = nullptr;
	}

	std::string str() const {
		return std::string { chars, size };
	}

	bool operator==(const String& other) const {
		return size == other.size && memcmp(chars, other.chars, size) == 0;
	}

	bool operator==(const char *other) const {
		return (other[size] == '\0') && (memcmp(chars, other, size) == 0);
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

template<typename Ok, typename Err = std::string>
class Result {
	std::variant<Ok, Err> repr;

public:
	Result(const Ok& ok) {
		repr = ok;
	}

	Result(const Err& err) {
		repr = err;
	}

	Ok unwrap() {
		if (std::holds_alternative<Err>(repr)) {
			std::cerr << std::get<Err>(repr);
			exit(EXIT_FAILURE);
		}
		return std::get<Ok>(repr);
	}

	bool is_ok() {
		return std::holds_alternative<Ok>(repr);
	}

	bool is_err() {
		return std::holds_alternative<Err>(repr);
	}

	const Ok& ok() {
		return std::get<Ok>(repr);
	}

	const Err& err() {
		return std::get<std::string>(repr);
	}
};

template<typename Err>
class Result<void, Err> {
	bool _is_ok;
	Err _err;

public:
	Result() : _is_ok(true) { }

	Result(const Err& err) : _is_ok(false) {
		_err = err;
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

	const Err& err() {
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

std::string debug_str(Type_Kind kind) {
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

struct Primitive_Type_Data {
	Size size;
};

union Type_Data {
	Primitive_Type_Data primitive;
};

struct Type {
	Type_Kind kind;
	Type_Data data;

	static constexpr Type Boolean() {
		return Type {
			.kind = Type_Kind::Boolean,
			.data = Type_Data {
				.primitive = { .size = sizeof(Runtime_Type::Boolean) }
			}
		};
	}

	static constexpr Type Character() {
		return Type {
			.kind = Type_Kind::Character,
			.data = Type_Data {
				.primitive = { .size = sizeof(Runtime_Type::Character) }
			}
		};
	}

	static constexpr Type Integer(size_t size) {
		internal_verify(size == 1 || size == 2 || size == 4 || size == 8, "Invalid size argument: %zu!", size);

		return Type {
			.kind = Type_Kind::Integer,
			.data = Type_Data {
				.primitive = { .size = size }
			}
		};
	}

	static constexpr Type Floating_Point(size_t size) {
		internal_verify(size == 4 || size == 8, "Invalid size argument: %zu!", size);

		return Type {
			.kind = Type_Kind::Floating_Point,
			.data = Type_Data {
				.primitive = { .size = size }
			}
		};
	}

	static constexpr Type String() {
		return Type {
			.kind = Type_Kind::String,
			.data = Type_Data {
				.primitive = { .size = sizeof(Runtime_Type::String) }
			}
		};
	}

	std::string debug_str() const {
		std::string s;

		switch (kind) {
			// primitive types
			case Type_Kind::No_Type:
			case Type_Kind::Null:
			case Type_Kind::Boolean:
			case Type_Kind::Character:
			case Type_Kind::String:
				s = ::debug_str(kind);
				break;

			// primitive types with many size variants
			case Type_Kind::Integer:
			case Type_Kind::Floating_Point: {
				std::stringstream ss;
				ss << ::debug_str(kind) << data.primitive.size * 8;
				s = ss.str();
			} break;

			default:
				s = std::to_string(static_cast<int>(kind));
				break;
		}

		return s;
	}

	std::string display_str() const {
		std::string s;

		switch (kind) {
			case Type_Kind::No_Type: {
				s = "!";
			} break;
			case Type_Kind::Null: {
				s = "Null";
			} break;
			case Type_Kind::Boolean: {
				s = "bool";
			} break;
			case Type_Kind::Character: {
				s = "char";
			} break;
			case Type_Kind::Integer: {
				std::stringstream ss;
				ss << "i" << data.primitive.size * 8;
				s = ss.str();
			} break;
			case Type_Kind::Floating_Point: {
				std::stringstream ss;
				ss << "f" << data.primitive.size * 8;
				s = ss.str();
			} break;
			case Type_Kind::String: {
				s = "string";
			} break;

			default:
				internal_error("Unhandled Type_Kind: %s!", ::debug_str(kind).c_str());
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

	Unary_Not,
	Unary_Negate,

	Binary_Variable_Declaration,
	Binary_Assignment,
	Binary_While,
	Binary_Add,
	Binary_Subtract,
	Binary_Multiply,
	Binary_Divide,
	Binary_And,
	Binary_Or,
	Binary_EQ,
	Binary_NE,

	Block,
	Block_Comma,

	Variable_Instantiation,
	Constant_Instantiation,
	Function_Declaration,
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
		CASE(Literal_String);
		CASE(Unary_Not);
		CASE(Unary_Negate);
		CASE(Binary_Variable_Declaration);
		CASE(Binary_Assignment);
		CASE(Binary_While);
		CASE(Binary_Add);
		CASE(Binary_Subtract);
		CASE(Binary_Multiply);
		CASE(Binary_Divide);
		CASE(Binary_And);
		CASE(Binary_Or);
		CASE(Binary_EQ);
		CASE(Binary_NE);
		CASE(Block);
		CASE(Block_Comma);
		CASE(Variable_Instantiation);
		CASE(Constant_Instantiation);
		CASE(Function_Declaration);
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

struct AST_Type_Signature : AST {
	Type value_type;
};

struct AST_If : AST {
	AST *condition;
	AST *then_block;
	AST *else_block; // optional
};

struct AST_Variable_Instantiation : AST {
	AST_Symbol *symbol;
	AST_Type_Signature *specified_type_signature; // optional
	AST *initializer;
};

struct AST_Function_Declaration : AST {
	AST_Block *parameters;
	// @TODO:
	// :ImplementParseTypeSignature
	// AST_Type_Signature *return_type_signature; // optional
	//
	AST *return_type_signature; // optional
	AST_Block *body;
};

void AST::debug_print(size_t indentation) const {
	#define CASE_UNARY(kind) case AST_Kind::kind: {\
		const AST_Unary *self = dynamic_cast<const AST_Unary *>(this);\
		internal_verify(self, "Failed to cast to `const AST_Unary *`");\
		printf("`%s`:\n", debug_str(AST_Kind::kind).c_str());\
		print_unary(self, indentation);\
	} break
	
	#define CASE_BINARY(kind) case AST_Kind::kind: {\
		const AST_Binary *self = dynamic_cast<const AST_Binary *>(this);\
		internal_verify(self, "Faield to cast to `const AST_Binary *`");\
		printf("`%s`:\n", debug_str(AST_Kind::kind).c_str());\
		print_binary(self, indentation);\
	} break

	#define CASE_BLOCK(kind) case AST_Kind::kind: {\
		const AST_Block *self = dynamic_cast<const AST_Block *>(this);\
		internal_verify(self, "Failed to cast to `const AST_Block *`");\
		printf("`%s`:\n", debug_str(AST_Kind::kind).c_str());\
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
				static_cast<int>(self->symbol.size), self->symbol.chars
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
		case AST_Kind::Literal_String: {
			const AST_Literal *self = dynamic_cast<const AST_Literal *>(this);
			internal_verify(self, "Failed to cast to `const AST_Literal *`");

			printf("`Literal_String`:\n");
			print_base_members(indentation);

			printf("%*svalue: %.*s\n",
				static_cast<int>(Print_Indentation_Size * (indentation + 1)), "",
				static_cast<int>(self->as.string.size), self->as.string.chars
			);
		} break;

		CASE_UNARY(Unary_Not);
		CASE_UNARY(Unary_Negate);
		
		CASE_BINARY(Binary_Variable_Declaration);
		CASE_BINARY(Binary_Assignment);
		CASE_BINARY(Binary_While);
		CASE_BINARY(Binary_Add);
		CASE_BINARY(Binary_Subtract);
		CASE_BINARY(Binary_Multiply);
		CASE_BINARY(Binary_Divide);
		CASE_BINARY(Binary_And);
		CASE_BINARY(Binary_Or);
		CASE_BINARY(Binary_EQ);
		CASE_BINARY(Binary_NE);

		CASE_BLOCK(Block);
		CASE_BLOCK(Block_Comma);

		case AST_Kind::Variable_Instantiation:
		case AST_Kind::Constant_Instantiation: {
			const AST_Variable_Instantiation *self = dynamic_cast<const AST_Variable_Instantiation *>(this);
			internal_verify(self, "Failed to cast to `const AST_Literal_Instantiation *`");

			printf("`%s`:\n", debug_str(kind).c_str());
			print_base_members(indentation);

			print_member("symbol", indentation, self->symbol);
			if (self->specified_type_signature) print_member("type", indentation, self->specified_type_signature);
			print_member("initializer", indentation, self->initializer);
		} break;
		case AST_Kind::Function_Declaration: {
			const AST_Function_Declaration *self = dynamic_cast<const AST_Function_Declaration *>(this);
			internal_verify(self, "Failed to cast to `const AST_Function_Declaration *`");

			printf("`%s`:\n", debug_str(kind).c_str());
			print_base_members(indentation);

			print_member("parameters", indentation, self->parameters);
			if (self->return_type_signature) print_member("return", indentation, self->return_type_signature);
			print_member("body", indentation, self->body);
		} break;
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
	Unary,      // ! ~
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
	Delimeter_Comma,
	Delimeter_Left_Parenthesis,
	Delimeter_Right_Parenthesis,
	Delimeter_Left_Curly,
	Delimeter_Right_Curly,

	// punctuation
	Punctuation_Bang,
	Punctuation_Bang_Equal,
	Punctuation_Equal,
	Punctuation_Equal_Equal,
	Punctuation_Colon,
	Punctuation_Plus,
	Punctuation_Dash,
	Punctuation_Star,
	Punctuation_Slash,
	Punctuation_Ampersand_Ampersand,
	Punctuation_Pipe_Pipe,
	Punctuation_Right_Thin_Arrow,

	// keywords
	Keyword_If,
	Keyword_Else,
	Keyword_While,
	Keyword_Fn,
};

std::string debug_str(Token_Kind kind) {
	#define CASE(kind) case Token_Kind::kind: return #kind

	switch (kind) {
		CASE(Eof);

		// literals
		CASE(Literal_Null);
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
		CASE(Delimeter_Comma);
		CASE(Delimeter_Left_Parenthesis);
		CASE(Delimeter_Right_Parenthesis);
		CASE(Delimeter_Left_Curly);
		CASE(Delimeter_Right_Curly);

		// punctuation
		CASE(Punctuation_Bang);
		CASE(Punctuation_Bang_Equal);
		CASE(Punctuation_Equal);
		CASE(Punctuation_Equal_Equal);
		CASE(Punctuation_Colon);
		CASE(Punctuation_Plus);
		CASE(Punctuation_Dash);
		CASE(Punctuation_Star);
		CASE(Punctuation_Slash);
		CASE(Punctuation_Ampersand_Ampersand);
		CASE(Punctuation_Pipe_Pipe);
		CASE(Punctuation_Right_Thin_Arrow);

		// keywords
		CASE(Keyword_If);
		CASE(Keyword_Else);
		CASE(Keyword_While);
		CASE(Keyword_Fn);

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
			CASE(Delimeter_Comma, None);
			CASE(Delimeter_Left_Parenthesis, Call);
			CASE(Delimeter_Right_Parenthesis, None);
			CASE(Delimeter_Left_Curly, None);
			CASE(Delimeter_Right_Curly, None);

			// punctuation
			CASE(Punctuation_Bang, Unary);
			CASE(Punctuation_Bang_Equal, Equality);
			CASE(Punctuation_Equal, Assignment);
			CASE(Punctuation_Equal_Equal, Equality);
			CASE(Punctuation_Colon, Colon);
			CASE(Punctuation_Plus, Term);
			CASE(Punctuation_Dash, Term);
			CASE(Punctuation_Star, Factor);
			CASE(Punctuation_Slash, Factor);
			CASE(Punctuation_Ampersand_Ampersand, And);
			CASE(Punctuation_Pipe_Pipe, Or);

			// keywords
			CASE(Keyword_If, None);
			CASE(Keyword_Else, None);
			CASE(Keyword_While, None);
			CASE(Keyword_Fn, None);

			default:
				internal_error("Unhandled Token_Kind: %s!", debug_str(kind).c_str());
		}

		#undef CASE
	}

	void debug_print(int indentation = 0) const {
		printf("%*sToken::%s:\n", static_cast<int>(Print_Indentation_Size * indentation), "", debug_str(kind).c_str());

		// @TODO:
		// print data
		//
		switch (kind) {
			case Token_Kind::Literal_Boolean:
				printf("%*sdata: %s\n", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", data.boolean ? "true" : "false");
				break;
			case Token_Kind::Literal_Character:
				printf("%*sdata: %c\n", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", data.character);
				break;
			case Token_Kind::Literal_Integer:
				printf("%*sdata: %lld\n", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", data.integer);
				break;
			case Token_Kind::Literal_Floating_Point:
				printf("%*sdata: '%f'\n", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", data.floating_point);
				break;
			case Token_Kind::Literal_String:
				printf("%*sdata: \"%.*s\"\n", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", static_cast<int>(data.string.size), data.string.chars);
				break;
			case Token_Kind::Symbol_Identifier:
				printf("%*sdata: \"%.*s\"\n", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", static_cast<int>(data.string.size), data.string.chars);
				break;

			default:
				break;
		}

		printf("%*slocation: %s\n", static_cast<int>(Print_Indentation_Size * (indentation + 1)), "", location.debug_str().c_str());
	}

	std::string display_str() const {
		#define CASE(kind, display) case Token_Kind::kind: return display

		switch (kind) {
			CASE(Eof, "EOF");

			// literals
			CASE(Literal_Null, "null");
			CASE(Literal_Boolean, data.boolean ? "true" : "false");
			CASE(Literal_Character, std::to_string(data.character));
			CASE(Literal_Integer, std::to_string(data.integer));
			CASE(Literal_Floating_Point, std::to_string(data.floating_point));
			CASE(Literal_String, data.string.str());
		
			// symbols
			CASE(Symbol_Identifier, data.string.str());
		
			// delimeters
			CASE(Delimeter_Newline, "new-line");
			CASE(Delimeter_Semicolon, ";");
			CASE(Delimeter_Comma, ",");
			CASE(Delimeter_Left_Parenthesis, "(");
			CASE(Delimeter_Right_Parenthesis, ")");
			CASE(Delimeter_Left_Curly, "{");
			CASE(Delimeter_Right_Curly, "}");
		
			// punctuation
			CASE(Punctuation_Bang, "!");
			CASE(Punctuation_Bang_Equal, "!=");
			CASE(Punctuation_Equal, "=");
			CASE(Punctuation_Equal_Equal, "==");
			CASE(Punctuation_Colon, ":");
			CASE(Punctuation_Plus, "+");
			CASE(Punctuation_Dash, "-");
			CASE(Punctuation_Star, "*");
			CASE(Punctuation_Slash, "/");
			CASE(Punctuation_Ampersand_Ampersand, "&&");
			CASE(Punctuation_Pipe_Pipe, "||");
			CASE(Punctuation_Right_Thin_Arrow, "->");
		
			// keywords
			CASE(Keyword_If, "if");
			CASE(Keyword_Else, "else");
			CASE(Keyword_While, "while");
			CASE(Keyword_Fn, "fn");

			default:
				internal_error("Unhandled Token_Kind: %s!\n", debug_str(kind).c_str());
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
		if (source.size <= 0) {
			return '\0';
		}
		return source.chars[skip];
	}

	// @TODO:
	// :HandleUTF8
	//
	char32_t next_char() {
		if (source.size <= 0) {
			return '\0';
		}

		char32_t next_char = peek_char();
		source.advance();
		coloumn++;
		return next_char;
	}

	bool next_char_if_eq(char32_t &actual, char32_t expected) {
		if (actual == expected) {
			actual = next_char();
			return true;
		}
		return false;
	}

	bool match_char(char32_t expected, char32_t *opt_out_c = nullptr) {
		if (peek_char() == expected) {
			char32_t c = next_char();
			if (opt_out_c) *opt_out_c = c;
			return true;
		}
		return false;
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
			data
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
			if (c == '/' && peek_char(1) == '/') {
				while (c != '\n') {
					next_char();
					c = peek_char();
				}
			}

			if (c == '\n' && previous_token.kind == Token_Kind::Delimeter_Newline) {
				while (c == '\n') {
					next_char();
					c = peek_char();
					line++;
					coloumn = 0;
				}
			} else {
				next_char();
				c = peek_char();
			}
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
		Code_Location token_location = current_location();

		if (c == '\0') {
			previous_token = make_token(Token_Kind::Eof);
		} else if (next_char_if_eq(c, '\n')) {
			previous_token = make_token(Token_Kind::Delimeter_Newline);
			line++;
			coloumn = 0;
		} else if (next_char_if_eq(c, '\'')) {
			previous_token = try_(next_character_token());
		} else if (next_char_if_eq(c, '\"')) {
			previous_token = next_string_token();
		} else if (isdigit(c) || (c == '.' && isdigit(peek_char(1)))) {
			previous_token = next_number_token();
		} else if (isalpha(c) || (c == '_' && is_identifier_character(peek_char(1)))) {
			previous_token = next_keyword_or_identifier_token();
		} else {
			next_char();
			previous_token = try_(next_punctuation_token(c));
		}

		previous_token.location = token_location;

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
		auto string = String { 0, source.chars };

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

		// @TODO:
		// :HandleUTF8
		//
		char *start = source.chars;
		size_t size = 0;

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
		auto word = String { 0, source.chars };

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
		} else if (word == "while") {
			token = make_token(Token_Kind::Keyword_While);
		} else if (word == "fn") {
			token = make_token(Token_Kind::Keyword_Fn);
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
			case ',': {
				token = make_token(Token_Kind::Delimeter_Comma);
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
			case '!': {
				if (match_char('=')) {
					token = make_token(Token_Kind::Punctuation_Bang_Equal);
				} else {
					token = make_token(Token_Kind::Punctuation_Bang);
				}
			} break;
			case '=': {
				if (match_char('=')) {
					token = make_token(Token_Kind::Punctuation_Equal_Equal);
				} else {
					token = make_token(Token_Kind::Punctuation_Equal);
				}
			} break;
			case ':': {
				token = make_token(Token_Kind::Punctuation_Colon);
			} break; 
			case '+': {
				token = make_token(Token_Kind::Punctuation_Plus);
			} break;
			case '-': {
				if (match_char('>')) {
					token = make_token(Token_Kind::Punctuation_Right_Thin_Arrow);
				} else {
					token = make_token(Token_Kind::Punctuation_Dash);
				}
			} break;
			case '*': {
				token = make_token(Token_Kind::Punctuation_Star);
			} break;
			case '/': {
				token = make_token(Token_Kind::Punctuation_Slash);
			} break;
			case '&': {
				if (match_char('&')) {
					token = make_token(Token_Kind::Punctuation_Ampersand_Ampersand);
				} else {
					todo("Implement `&` tokenization.");
				}
			} break;
			case '|': {
				if (match_char('|')) {
					token = make_token(Token_Kind::Punctuation_Pipe_Pipe);
				} else {
					todo("Implement `|` tokenization.");
				}
			} break;

			default:
				error(current_location(), "Unknown operator `%c`.", c);
		}

		return token;
	}
};

struct Parser {
	bool error;
	Tokenizer tokenizer;

	bool check(Token_Kind kind) {
		// @NOTE: 
		// Maybe `check()` and `match()` should return `Result<bool>`s or something
		//
		auto peeked = tokenizer.peek().unwrap();
		return peeked.kind == kind;
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
		} else if (check(Token_Kind::Keyword_While)) {
			node = try_(parse_while_statement());
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

	Result<AST_Binary *> parse_while_statement() {
		auto location = try_(skip_expect(Token_Kind::Keyword_While, "Expected `while` statement!")).location;
		skip_newlines();

		auto condition = try_(parse_expression());
		auto body = try_(parse_block());

		AST_Binary *node = new AST_Binary;
		node->kind = AST_Kind::Binary_While;
		node->location = location;
		node->lhs = condition;
		node->rhs = body;

		return node;
	}

	Result<AST *> parse_expression_or_assignment() {
		return parse_precedence(Token_Precedence::Assignment);
	}

	Result<AST *> parse_expression() {
		auto expression = try_(parse_expression_or_assignment());

		verify(expression->kind != AST_Kind::Binary_Assignment, expression->location, "Cannot assign in expression context.");
		verify(expression->kind != AST_Kind::Variable_Instantiation, expression->location, "Cannot instantiate new variables in expression context.");
		verify(expression->kind != AST_Kind::Constant_Instantiation, expression->location, "Cannot instantiate new constants in expression context.");
		verify(expression->kind != AST_Kind::Binary_Variable_Declaration, expression->location, "Cannot declare new variables in expression context.");
		
		return expression;
	}

	Result<AST *> parse_precedence(Token_Precedence precedence) {
		Token token = try_(tokenizer.next());
		verify(token.kind != Token_Kind::Eof, token.location, "Unexpected end of file!");

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

		Code_Location location = token.location;

		switch (token.kind) {
			case Token_Kind::Symbol_Identifier: {
				AST_Symbol *identifier = new AST_Symbol;
				identifier->kind = AST_Kind::Symbol_Identifier;
				identifier->location = location;

				// @TODO:
				// :HandleStringData
				//
				identifier->symbol = token.data.string;

				node = identifier;
			} break;
			case Token_Kind::Literal_Null: {
				AST_Literal *literal = new AST_Literal;
				literal->kind = AST_Kind::Literal_Null;
				literal->location = location;

				node = literal;
			} break;
			case Token_Kind::Literal_Boolean: {
				AST_Literal *literal = new AST_Literal;
				literal->kind = AST_Kind::Literal_Boolean;
				literal->location = location;
				literal->as.boolean = token.data.boolean;

				node = literal;
			} break;
			case Token_Kind::Literal_Character: {
				AST_Literal *literal = new AST_Literal;
				literal->kind = AST_Kind::Literal_Character;
				literal->location = location;
				literal->as.character = token.data.character;

				node = literal;
			} break;
			case Token_Kind::Literal_Integer: {
				AST_Literal *literal = new AST_Literal;
				literal->kind = AST_Kind::Literal_Integer;
				literal->location = location;
				literal->as.integer = token.data.integer;

				node = literal;
			} break;
			case Token_Kind::Literal_Floating_Point: {
				AST_Literal *literal = new AST_Literal;
				literal->kind = AST_Kind::Literal_Floating_Point;
				literal->location = location;
				literal->as.floating_point = token.data.floating_point;

				node = literal;
			} break;
			case Token_Kind::Literal_String: {
				AST_Literal *literal = new AST_Literal;
				literal->kind = AST_Kind::Literal_String;
				literal->location = location;

				// @TODO:
				// :HandleStringData
				//
				literal->as.string = token.data.string;

				node = literal;
			} break;
			case Token_Kind::Punctuation_Bang:
				node = try_(parse_unary(AST_Kind::Unary_Not, location));
				break;
			case Token_Kind::Punctuation_Dash:
				if (check(Token_Kind::Literal_Integer)) {
					Token literal_token = try_(tokenizer.next());
					AST_Literal *literal = dynamic_cast<AST_Literal *>(try_(parse_prefix(literal_token)));
					internal_verify(literal, "Failed to cast to `AST_Literal *`");

					literal->as.integer = -literal->as.integer;
					literal->location = location;

					node = literal;
				} else if (check(Token_Kind::Literal_Floating_Point)) {
					Token literal_token = try_(tokenizer.next());
					AST_Literal *literal = dynamic_cast<AST_Literal *>(try_(parse_prefix(literal_token)));
					internal_verify(literal, "Failed to cast to `AST_Literal *`");

					literal->as.floating_point = -literal->as.floating_point;
					literal->location = location;

					node = literal;
				} else {
					node = try_(parse_unary(AST_Kind::Unary_Negate, location));
				}
				break;
			
			case Token_Kind::Keyword_Fn:
				node = try_(parse_function(token));
				break;

			default:
				error(location, "`%s` is not a prefix operation!", token.display_str().c_str());
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
			case Token_Kind::Punctuation_Bang_Equal:
				node = try_(parse_binary(AST_Kind::Binary_NE, precedence, previous, location));
				break;
			case Token_Kind::Punctuation_Equal_Equal:
				node = try_(parse_binary(AST_Kind::Binary_EQ, precedence, previous, location));
				break;
			case Token_Kind::Punctuation_Equal:
				node = try_(parse_binary(AST_Kind::Binary_Assignment, precedence, previous, location));
				break;
			case Token_Kind::Punctuation_Plus:
				node = try_(parse_binary(AST_Kind::Binary_Add, precedence, previous, location));
				break;
			case Token_Kind::Punctuation_Dash:
				node = try_(parse_binary(AST_Kind::Binary_Subtract, precedence, previous, location));
				break;
			case Token_Kind::Punctuation_Star:
				node = try_(parse_binary(AST_Kind::Binary_Multiply, precedence, previous, location));
				break;
			case Token_Kind::Punctuation_Slash:
				node = try_(parse_binary(AST_Kind::Binary_Divide, precedence, previous, location));
				break;
			case Token_Kind::Punctuation_Ampersand_Ampersand:
				node = try_(parse_binary(AST_Kind::Binary_And, precedence, previous, location));
				break;
			case Token_Kind::Punctuation_Pipe_Pipe:
				node = try_(parse_binary(AST_Kind::Binary_Or, precedence, previous, location));
				break;

			default:
				error(location, "`%s` is not an infix operation!", token.display_str().c_str());
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

		try_(skip_expect(Token_Kind::Delimeter_Right_Curly, "Expected `}` to terminate block!"));
		block->location = location;

		return block;
	}

	Result<AST *> parse_colon(AST *previous, Code_Location location) {
		AST *node = nullptr;

		if (match(Token_Kind::Punctuation_Equal)) {
			auto initializer = try_(parse_expression());

			AST_Variable_Instantiation *inst = new AST_Variable_Instantiation;
			inst->kind = AST_Kind::Variable_Instantiation;
			inst->location = location;

			AST_Symbol *symbol = dynamic_cast<AST_Symbol *>(previous);
			verify(symbol, previous->location, "Expected a symbol on the left hand side of variable instantiation.");
			inst->symbol = symbol;

			inst->initializer = initializer;

			inst->specified_type_signature = nullptr;

			node = inst;
		} else if (match(Token_Kind::Punctuation_Colon)) {
			auto initializer = try_(parse_expression());

			AST_Variable_Instantiation *inst = new AST_Variable_Instantiation;
			inst->kind = AST_Kind::Constant_Instantiation;
			inst->location = location;

			AST_Symbol *symbol = dynamic_cast<AST_Symbol *>(previous);
			verify(symbol, previous->location, "Expected a symbol on the left hand side of constant declaration.");
			inst->symbol = symbol;

			inst->initializer = initializer;

			inst->specified_type_signature = nullptr;

			node = inst;
		} else {
			todo("Variable declarations not yet implemented. previous.location = %s", tokenizer.current_location().debug_str().c_str());
		}

		return node;
	}

	Result<AST *> parse_function(Token fn_token) {
		try_(skip_expect(Token_Kind::Delimeter_Left_Parenthesis, "Expected `(` after `fn` keyword."));

		// @NOTE:
		// @TODO:
		// Should this be factored out into some sort of `parse_comma_separated_expressions()`?
		//
		AST_Block *parameters = new AST_Block;
		parameters->kind = AST_Kind::Block_Comma;
		parameters->location = tokenizer.current_location();

		do {
			if (skip_check(Token_Kind::Delimeter_Right_Parenthesis)) break;
			Token param_token = try_(skip_expect(Token_Kind::Symbol_Identifier, "Expected either `)` or parameter name."));
			
			Code_Location param_location = try_(skip_expect(Token_Kind::Punctuation_Colon, "Expected `:` after parameter name.")).location;

			// @TODO:
			// :ImplementParseTypeSignature
			//
			Token type_token = try_(skip_expect(Token_Kind::Symbol_Identifier, "Expected type name for parameter."));

			AST_Symbol *param_ident = new AST_Symbol;
			param_ident->kind = AST_Kind::Symbol_Identifier;
			param_ident->location = param_token.location;
			// @TODO:
			// :HandleStringData
			//
			param_ident->symbol = param_token.data.string;

			AST_Symbol *type_ident = new AST_Symbol;
			type_ident->kind = AST_Kind::Symbol_Identifier;
			type_ident->location = type_token.location;
			// @TODO:
			// :HandleStringData
			//
			type_ident->symbol = type_token.data.string;

			AST_Binary *param_node = new AST_Binary;
			param_node->kind = AST_Kind::Binary_Variable_Declaration;
			param_node->location = param_location;
			param_node->lhs = param_ident;
			param_node->rhs = type_ident;

			parameters->nodes.push_back(param_node);
		} while (skip_match(Token_Kind::Delimeter_Comma) || check(Token_Kind::Eof));

		try_(expect(Token_Kind::Delimeter_Right_Parenthesis, "Expected `)` to terminate parameter list."));

		AST *return_type = nullptr;
		if (skip_match(Token_Kind::Punctuation_Right_Thin_Arrow)) {
			// @TODO:
			// :ImplementParseTypeSignature
			//
			Token type_token = try_(skip_expect(Token_Kind::Symbol_Identifier, "Expected type name for parameter."));
			AST_Symbol *type_ident = new AST_Symbol;
			type_ident->kind = AST_Kind::Symbol_Identifier;
			type_ident->location = type_token.location;
			// @TODO:
			// :HandleStringData
			//
			type_ident->symbol = type_token.data.string;

			return_type = type_ident;
		}

		AST_Block *body = try_(parse_block());

		AST_Function_Declaration *decl = new AST_Function_Declaration;
		decl->kind = AST_Kind::Function_Declaration;
		decl->location = fn_token.location;
		decl->parameters = parameters;
		decl->return_type_signature = return_type;
		decl->body = body;

		return decl;
	}
};

AST_Block *parse(String source, const char *filename) {
	Parser p;
	p.error = false;
	p.tokenizer.source = source;
	p.tokenizer.filename = filename;

	AST_Block *ast = new AST_Block;
	ast->kind = AST_Kind::Block;
	ast->location = p.tokenizer.current_location();

	while (true) {
		p.skip_newlines();
		if (p.check(Token_Kind::Eof)) break;

		auto result = p.parse_declaration();
		if (result.is_err()) {
			p.error = true;
			std::cerr << result.err();
			continue;
		}

		ast->nodes.push_back(result.ok());
	}

	return p.error ? nullptr : ast;
}

//
//
// Typechecking
//
//

struct Typechecker {
	//
	// Child Data Structures
	//
	struct Binding {
		enum {
			Variable,
			Type,
			Function,
			Module,
		} kind;

		struct Function_Binding {
			PID pid;
			::Type type;
		};

		union {
			::Type ty;
			Function_Binding fn;
			// ::Module *mod;
		};

		static Binding variable(::Type type) {
			Binding b;
			b.kind = Variable;
			b.ty = type;
			return b;
		}

		static Binding type(::Type type) {
			Binding b;
			b.kind = Type;
			b.ty = type;
			return b;
		}

		static Binding function(PID pid, ::Type fn_type) {
			Binding b;
			b.kind = Function;
			b.fn.pid = pid;
			b.fn.type = fn_type;
			return b;
		}

		/* @TODO:
			for when we implement modules
		static Binding module(::Module *module) {
			return Binding {
				.kind = Module,
				.mod = module
			};
		}
		*/
	};

	struct Scope {
		std::unordered_map<std::string, Binding> bindings;
	};

	//
	// Fields
	//
	// Interpreter *interp;
	// Module *module;
	Scope *global_scope;
	// Function_Definition *function;
	Typechecker *parent;
	// bool has_return;
	std::forward_list<Scope> scopes;

	//
	// Constructor B.S
	//
	Typechecker() = default;

	/*
	Typechecker(Interpreter *interp, Module *module) {
		this->interp = interp;
		this->module = module;
		this->function = nullptr;
		this->parent = nullptr;
		this->has_return = false;
		begin_scope(); // global scope
		global_scope = &current_scope();
	}
	*/

	/*
	Typechecker(Typer &t, Function_Definition *function) {
		this->interp = t.interp;
		this->module = t.module;
		this->global_scope = t.global_scope;
		this->function = function;
		this->parent = &t;
		this->has_return = false;
	}
	*/

	Scope &current_scope() {
		internal_verify(!scopes.empty(), "No scopes in `scopes` field of Typechecker!");
		return scopes.front();
	}

	void begin_scope() {
		scopes.push_front(Scope{});
	}

	void end_scope() {
		internal_verify(!scopes.empty(), "No sopes in `scopes` field of Typechecker to pop!");
		scopes.pop_front();
	}

	std::optional<Binding> find_binding_by_id(const std::string &id, bool checking_through_parent = false) {
		for (Scope &scope : scopes) {
			auto it = scope.bindings.find(id);
			if (it == scope.bindings.end()) continue;
			if (checking_through_parent && it->second.kind == Binding::Variable) continue;
			return it->second;
		}

		if (parent) {
			auto optional_binding = parent->find_binding_by_id(id, true);
			if (optional_binding.has_value()) {
				return *optional_binding;
			}
		}

		if (!checking_through_parent) {
			auto it = global_scope->bindings.find(id);
			if (it != global_scope->bindings.end()) {
				return it->second;
			}
		}

		return {};
	}

	Result<void> put_binding(Code_Location location, const std::string &id, Binding binding) {
		Scope &scope = current_scope();

		auto it = scope.bindings.find(id);
		verify(it == scope.bindings.end(), location, "Redefinition of `%s`", id.c_str());

		scope.bindings[id] = binding;

		return {};
	}

	Result<void> bind_variable(Code_Location location, const std::string &id, Type type) {
		return put_binding(location, id, Binding::variable(type));
	}

	// Result<void> bind_type(Code_Location location, const std::string &id, Type type) {
	// 	internal_verify(type.kind == Type_Kind::Type, "Attempted to bind a type name to something other than a type! `%s` to `%s`", id.c_str(), type.debug_str().c_str());
	// 	return put_binding(location, id, Binding::type(type));
	// }

	// Result<void> bind_function(Code_Location location, const std::string &id, PID pid, Type type) {
	// 	internal_verify(type.kind == Type_Kind::Function, "Attempted to bind a function name to something other than a function-type! `%s` to `%s`", id.c_str(), type.debug_str().c_str());
	// 	return put_binding(location, id, Binding::function(pid, type));
	// }

	/*
	Result<void> bind_module(Code_Location location, const std::string &id, Module *module) {
		return put_binding(location, id, Binding::module(module));
	}
	*/

	Result<AST *> typecheck(AST *node) {
		AST *typechecked_node = nullptr;

		switch (node->kind) {
			case AST_Kind::Symbol_Identifier: {
				AST_Symbol *symbol = dynamic_cast<AST_Symbol *>(node);
				internal_verify(symbol, "Failed to cast to `AST_Symbol *`");

				auto opt_binding = find_binding_by_id(symbol->symbol.str());
				verify(opt_binding.has_value(), "Unresolved identifier `%.*s`!", symbol->symbol.size, symbol->symbol.chars);
				Binding binding = *opt_binding;

				if (binding.kind != Binding::Variable) {
					todo("Implement non-variable binding typechecking!");
				}

				symbol->type = binding.ty;
				typechecked_node = symbol;
			} break;

			case AST_Kind::Literal_Null: {
				node->type = Type { Type_Kind::Null };
				typechecked_node = node;
			} break;
			case AST_Kind::Literal_Boolean: {
				node->type = Type::Boolean();
				typechecked_node = node;
			} break;
			case AST_Kind::Literal_Character: {
				node->type = Type::Character();
				typechecked_node = node;
			} break;
			case AST_Kind::Literal_Integer: {
				AST_Literal *literal = dynamic_cast<AST_Literal *>(node);
				internal_verify(literal, "Failed to cast to `AST_Literal *`");

				Size literal_size = minimum_required_size_for_literal(literal->as.integer);
				node->type = Type::Integer(literal_size);
				typechecked_node = node;
			} break;
			case AST_Kind::Literal_Floating_Point: {
				AST_Literal *literal = dynamic_cast<AST_Literal *>(node);
				internal_verify(literal, "Failed to cast to `AST_Literal *`");

				Size literal_size = minimum_required_size_for_literal(literal->as.floating_point);
				node->type = Type::Floating_Point(literal_size);
				typechecked_node = node;
			} break;
			case AST_Kind::Literal_String: {
				node->type = Type::String();
				typechecked_node = node;
			} break;

			case AST_Kind::Unary_Not: {
				AST_Unary *unary = dynamic_cast<AST_Unary *>(node);
				internal_verify(unary, "Failed to cast to `AST_Unary *`");

				unary->sub = try_(typecheck(unary->sub));
				verify(unary->sub->type->kind == Type_Kind::Boolean, unary->sub->location, "Type mismatch! `!` expects `%s` but was given `%s`", Type::Boolean().display_str().c_str(), unary->sub->type->display_str().c_str());

				unary->type = Type::Boolean();
				typechecked_node = unary;
			} break;
			case AST_Kind::Unary_Negate: {
				AST_Unary *unary = dynamic_cast<AST_Unary *>(node);
				internal_verify(unary, "Failed to cast to `AST_Unary *`");

				unary->sub = try_(typecheck(unary->sub));
				verify(
					unary->sub->type->kind == Type_Kind::Integer || unary->sub->type->kind == Type_Kind::Floating_Point,
					unary->sub->location,
					"Type mismatch! `-` expects its argument to be a numeric value but was given `%s`",
					unary->sub->type->display_str().c_str()
				);

				unary->type = unary->sub->type;
				typechecked_node = unary;
			} break;

			case AST_Kind::Binary_Variable_Declaration: {
				todo("Not yet implemented!");
			} break;
			case AST_Kind::Binary_Assignment: {
				AST_Binary *binary = dynamic_cast<AST_Binary*>(node);
				internal_verify(binary, "Failed to cast to `AST_Binary *`");

				binary->lhs = try_(typecheck(binary->lhs));
				binary->rhs = try_(typecheck(binary->rhs));

				// @TODO:
				// :TypeEquality
				//
				verify(binary->lhs->type->kind == binary->rhs->type->kind, binary->rhs->location, "Type mismatch! Cannot assign `%s` to `%s`", binary->rhs->type->display_str().c_str(), binary->lhs->type->display_str().c_str());

				binary->type = Type { Type_Kind::No_Type };
				typechecked_node = binary;
			} break;
			case AST_Kind::Binary_While: {
				AST_Binary *binary = dynamic_cast<AST_Binary *>(node);
				internal_verify(binary, "Failed to cast to `AST_Binary *`");

				binary->lhs = try_(typecheck(binary->lhs));
				verify(binary->lhs->type->kind == Type_Kind::Boolean, binary->lhs->location, "Type mismatch! Expected boolean expression as condition to `while` statement but found `%s`.", binary->lhs->type->display_str().c_str());

				binary->rhs = try_(typecheck(binary->rhs));

				binary->type = Type { Type_Kind::No_Type };
				typechecked_node = binary;
			} break;
			case AST_Kind::Binary_Add: {
				AST_Binary *binary = dynamic_cast<AST_Binary*>(node);
				internal_verify(binary, "Failed to cast to `AST_Binary *`");

				binary->lhs = try_(typecheck(binary->lhs));
				binary->rhs = try_(typecheck(binary->rhs));

				verify(
					binary->lhs->type->kind == Type_Kind::Integer || binary->lhs->type->kind ==Type_Kind::Floating_Point,
					binary->lhs->location,
					"Type mismatch! `+` expects its arguments to be numeric values but was given `%s`",
					binary->lhs->type->display_str().c_str()
				);
				verify(
					binary->rhs->type->kind == Type_Kind::Integer || binary->rhs->type->kind ==Type_Kind::Floating_Point,
					binary->rhs->location,
					"Type mismatch! `+` expects its arguments to be numeric values but was given `%s`",
					binary->rhs->type->display_str().c_str()
				);
				verify(
					binary->lhs->type->kind == binary->rhs->type->kind,
					binary->location,
					"Type mismatch! `+` expects its arguments to be the same type. `%s` vs. `%s`",
					binary->lhs->type->display_str().c_str(),
					binary->rhs->type->display_str().c_str()
				);

				binary->type = binary->lhs->type;
				typechecked_node = binary;
			} break;
			case AST_Kind::Binary_Subtract: {
				AST_Binary *binary = dynamic_cast<AST_Binary*>(node);
				internal_verify(binary, "Failed to cast to `AST_Binary *`");

				binary->lhs = try_(typecheck(binary->lhs));
				binary->rhs = try_(typecheck(binary->rhs));

				verify(
					binary->lhs->type->kind == Type_Kind::Integer || binary->lhs->type->kind ==Type_Kind::Floating_Point,
					binary->lhs->location,
					"Type mismatch! `-` expects its arguments to be numeric values but was given `%s`",
					binary->lhs->type->display_str().c_str()
				);
				verify(
					binary->rhs->type->kind == Type_Kind::Integer || binary->rhs->type->kind ==Type_Kind::Floating_Point,
					binary->rhs->location,
					"Type mismatch! `-` expects its arguments to be numeric values but was given `%s`",
					binary->rhs->type->display_str().c_str()
				);
				verify(
					binary->lhs->type->kind == binary->rhs->type->kind,
					binary->location,
					"Type mismatch! `-` expects its arguments to be the same type. `%s` vs. `%s`",
					binary->lhs->type->display_str().c_str(),
					binary->rhs->type->display_str().c_str()
				);

				binary->type = binary->lhs->type;
				typechecked_node = binary;
			} break;
			case AST_Kind::Binary_Multiply: {
				AST_Binary *binary = dynamic_cast<AST_Binary*>(node);
				internal_verify(binary, "Failed to cast to `AST_Binary *`");

				binary->lhs = try_(typecheck(binary->lhs));
				binary->rhs = try_(typecheck(binary->rhs));

				verify(
					binary->lhs->type->kind == Type_Kind::Integer || binary->lhs->type->kind ==Type_Kind::Floating_Point,
					binary->lhs->location,
					"Type mismatch! `*` expects its arguments to be numeric values but was given `%s`",
					binary->lhs->type->display_str().c_str()
				);
				verify(
					binary->rhs->type->kind == Type_Kind::Integer || binary->rhs->type->kind ==Type_Kind::Floating_Point,
					binary->rhs->location,
					"Type mismatch! `*` expects its arguments to be numeric values but was given `%s`",
					binary->rhs->type->display_str().c_str()
				);
				verify(
					binary->lhs->type->kind == binary->rhs->type->kind,
					binary->location,
					"Type mismatch! `*` expects its arguments to be the same type. `%s` vs. `%s`",
					binary->lhs->type->display_str().c_str(),
					binary->rhs->type->display_str().c_str()
				);

				binary->type = binary->lhs->type;
				typechecked_node = binary;
			} break;
			case AST_Kind::Binary_Divide: {
				AST_Binary *binary = dynamic_cast<AST_Binary*>(node);
				internal_verify(binary, "Failed to cast to `AST_Binary *`");

				binary->lhs = try_(typecheck(binary->lhs));
				binary->rhs = try_(typecheck(binary->rhs));

				verify(
					binary->lhs->type->kind == Type_Kind::Integer || binary->lhs->type->kind ==Type_Kind::Floating_Point,
					binary->lhs->location,
					"Type mismatch! `/` expects its arguments to be numeric values but was given `%s`",
					binary->lhs->type->display_str().c_str()
				);
				verify(
					binary->rhs->type->kind == Type_Kind::Integer || binary->rhs->type->kind ==Type_Kind::Floating_Point,
					binary->rhs->location,
					"Type mismatch! `/` expects its arguments to be numeric values but was given `%s`",
					binary->rhs->type->display_str().c_str()
				);
				verify(
					binary->lhs->type->kind == binary->rhs->type->kind,
					binary->location,
					"Type mismatch! `/` expects its arguments to be the same type. `%s` vs. `%s`",
					binary->lhs->type->display_str().c_str(),
					binary->rhs->type->display_str().c_str()
				);

				binary->type = binary->lhs->type;
				typechecked_node = binary;
			} break;
			case AST_Kind::Binary_And: {
				AST_Binary *binary = dynamic_cast<AST_Binary*>(node);
				internal_verify(binary, "Failed to cast to `AST_Binary *`");

				binary->lhs = try_(typecheck(binary->lhs));
				binary->rhs = try_(typecheck(binary->rhs));

				verify(
					binary->lhs->type->kind == Type_Kind::Boolean,
					binary->lhs->location,
					"Type mismatch! `&&` expects its arguments to be numeric values but was given `%s`",
					binary->lhs->type->display_str().c_str()
				);
				verify(
					binary->rhs->type->kind == Type_Kind::Boolean,
					binary->rhs->location,
					"Type mismatch! `&&` expects its arguments to be numeric values but was given `%s`",
					binary->rhs->type->display_str().c_str()
				);
				verify(
					binary->lhs->type->kind == binary->rhs->type->kind,
					binary->location,
					"Type mismatch! `&&` expects its arguments to be `%s`",
					Type::Boolean().display_str().c_str()
				);

				binary->type = binary->lhs->type;
				typechecked_node = binary;
			} break;
			case AST_Kind::Binary_Or: {
				AST_Binary *binary = dynamic_cast<AST_Binary*>(node);
				internal_verify(binary, "Failed to cast to `AST_Binary *`");

				binary->lhs = try_(typecheck(binary->lhs));
				binary->rhs = try_(typecheck(binary->rhs));

				verify(
					binary->lhs->type->kind == Type_Kind::Boolean,
					binary->lhs->location,
					"Type mismatch! `||` expects its arguments to be numeric values but was given `%s`",
					binary->lhs->type->display_str().c_str()
				);
				verify(
					binary->rhs->type->kind == Type_Kind::Boolean,
					binary->rhs->location,
					"Type mismatch! `||` expects its arguments to be numeric values but was given `%s`",
					binary->rhs->type->display_str().c_str()
				);
				verify(
					binary->lhs->type->kind == binary->rhs->type->kind,
					binary->location,
					"Type mismatch! `||` expects its arguments to be `%s`",
					Type::Boolean().display_str().c_str()
				);

				binary->type = binary->lhs->type;
				typechecked_node = binary;
			} break;
			case AST_Kind::Binary_EQ: {
				AST_Binary *binary = dynamic_cast<AST_Binary *>(node);
				internal_verify(binary, "Failed to cast to `AST_Binary *`");

				binary->lhs = try_(typecheck(binary->lhs));
				binary->rhs = try_(typecheck(binary->rhs));

				// @TODO:
				// :TypeEquality
				//
				verify(
					binary->lhs->type->kind == binary->rhs->type->kind,
					binary->location,
					"Type mismatch! `==` expects its arguments to be the same type! `%s` vs. `%s`.",
					binary->lhs->type->display_str().c_str(),
					binary->rhs->type->display_str().c_str()
				);

				binary->type = Type::Boolean();
				typechecked_node = binary;
			} break;
			case AST_Kind::Binary_NE: {
				AST_Binary *binary = dynamic_cast<AST_Binary *>(node);
				internal_verify(binary, "Failed to cast to `AST_Binary *`");

				binary->lhs = try_(typecheck(binary->lhs));
				binary->rhs = try_(typecheck(binary->rhs));

				// @TODO:
				// :TypeEquality
				//
				verify(
					binary->lhs->type->kind == binary->rhs->type->kind,
					binary->location,
					"Type mismatch! `!=` expects its arguments to be the same type! `%s` vs. `%s`.",
					binary->lhs->type->display_str().c_str(),
					binary->rhs->type->display_str().c_str()
				);

				binary->type = Type::Boolean();
				typechecked_node = binary;
			} break;

			case AST_Kind::Block: {
				AST_Block *block = dynamic_cast<AST_Block *>(node);
				internal_verify(block, "Failed to cast to `AST_Block *`");

				begin_scope();
				for (size_t i = 0; i < block->nodes.size(); i++) {
					block->nodes[i] = try_(typecheck(block->nodes[i]));
				}
				end_scope();

				block->type = Type { Type_Kind::No_Type };
				typechecked_node = block;
			} break;

			case AST_Kind::Variable_Instantiation: {
				AST_Variable_Instantiation *inst = dynamic_cast<AST_Variable_Instantiation *>(node);
				internal_verify(inst, "Failed to cast to `AST_Variable_Instantiation *`");

				AST_Symbol *symbol = inst->symbol;
				symbol->type = Type { Type_Kind::No_Type };

				Type inst_type;
				if (inst->specified_type_signature) {
					todo("Implement typechecking for var-insts with specified_type_signature.");
				} else {
					inst->initializer = try_(typecheck(inst->initializer));
					inst_type = inst->initializer->type.value();
				}

				bind_variable(inst->location, symbol->symbol.str(), inst_type);

				inst->type = Type { Type_Kind::No_Type };
				typechecked_node = inst;
			} break;
			case AST_Kind::Constant_Instantiation: {
				todo("Implement typechecking constant declaration!");
			} break;

			case AST_Kind::If: {
				AST_If *if_ = dynamic_cast<AST_If *>(node);
				internal_verify(if_, "Failed to cast to `AST_If *`");

				if_->condition = try_(typecheck(if_->condition));
				verify(
					if_->condition->type->kind == Type_Kind::Boolean, 
					if_->condition->location, 
					"Type mismatch! Expected boolean expression as conditional of `if` statement but expression evaluates to `%s`", 
					if_->condition->type->display_str().c_str()
				);

				if_->then_block = try_(typecheck(if_->then_block));
				if (if_->else_block) if_->else_block = try_(typecheck(if_->else_block));

				if_->type = Type { Type_Kind::No_Type };
				typechecked_node = if_;
			} break;

			default:
				internal_error("Unhandled AST_Kind: %s!", debug_str(node->kind).c_str());
		}

		return typechecked_node;
	}
};

Result<AST_Block *> typecheck(AST_Block *ast) {
	auto t = Typechecker {};

	// @HACK:
	// This won't be needed when we pass an `Interpreter` or something.
	//
	t.parent = nullptr;
	t.begin_scope();
	t.global_scope = &t.current_scope();
	
	for (size_t i = 0; i < ast->nodes.size(); i++) {
		ast->nodes[i] = try_(t.typecheck(ast->nodes[i]));
	}

	return ast;
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
	file.read(source.chars, size);
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
	AST_Block *ast = parse(source, filename);
	if (!ast) return EXIT_FAILURE;

	ast->debug_print();

	ast = dynamic_cast<AST_Block *>(typecheck(ast).unwrap());
	internal_verify(ast, "`typecheck()` didn't return an `AST_Block`");

	ast->debug_print();

	source.free();
	return 0;
}
