enum Token_Type {
	TOKEN_EOF = 256,

	// Reserved words
	TOKEN_PLACEHOLDER,
	
	TOKEN_SYMBOL,
	TOKEN_INTEGER_LITERAL,
};

#define RESERVED_WORDS_BEGIN (TOKEN_PLACEHOLDER)
#define RESERVED_WORDS_END   (TOKEN_SYMBOL)
#define RESERVED_WORDS_COUNT (RESERVED_WORDS_END - RESERVED_WORDS_BEGIN)

struct Token {
	Token_Type type;
	union {
		int integer;
		char * symbol;
	} values;
	static Token eof()
	{
		Token token;
		token.type = TOKEN_EOF;
		return token;
	}
	static Token with_type(Token_Type type)
	{
		Token token;
		token.type = type;
		return token;
	}
	static char * type_to_string(Token_Type type);
	char * to_string();
};

char * Token::type_to_string(Token_Type type)
{
	if (type < 256) {
		return Token::with_type(type).to_string();
	}
	switch (type) {
	case TOKEN_EOF:
		return strdup("EOF");
	case TOKEN_PLACEHOLDER:
		return strdup("_");
	case TOKEN_SYMBOL:
		return strdup("<symbol>");
	case TOKEN_INTEGER_LITERAL:
		return strdup("<integer>");
	default:
		fatal("Token::type_to_string() switch incomplete");
	}
}

char * Token::to_string()
{
	if (type >= 0 && type < 256) {
		char buf[2];
		buf[0] = type;
		buf[1] = '\0';
		return strdup(buf);
	}
	switch (type) {
	case TOKEN_SYMBOL: {
		return strdup(values.symbol);
	}
	case TOKEN_INTEGER_LITERAL: {
		return itoa(values.integer);
	}
	default:
		return Token::type_to_string(type);
	}
}

static const char * reserved_words[RESERVED_WORDS_COUNT] = {
	"_",
};

struct Lexer {
	const char * source;
	size_t source_length;
	size_t cursor;
	Lexer(const char * source);
	char next();
	char peek();
	void advance();
	Token next_token();
	Token_Type read_double_token(char left, char right, Token_Type double_type);
};

Lexer::Lexer(const char * source)
{
	this->source = source;
	source_length = strlen(source);
	cursor = 0;
}

char Lexer::next()
{
	char c = peek();
	advance();
	return c;
}

char Lexer::peek()
{
	if (cursor >= source_length) {
		return '\0';
	}
	return source[cursor];
}

void Lexer::advance()
{
	cursor++;
}

Token Lexer::next_token()
{
 reset:
	if (peek() == '\0') {
		return Token::eof();
	}
	
	if (isspace(peek())) {
		advance();
		goto reset;
	}

	if (isalpha(peek()) || peek() == '_') {
		char buf[512];
		size_t i = 0;
		while (isalnum(peek()) || peek() == '_') {
			buf[i++] = next();
		}
		buf[i++] = '\0';

		for (int i = 0; i < RESERVED_WORDS_COUNT; i++) {
			if (strcmp(buf, reserved_words[i]) == 0) {
				return Token::with_type((Token_Type) (RESERVED_WORDS_BEGIN + i));
			}
		}
		
		Token token;
		token.type = TOKEN_SYMBOL;
		token.values.symbol = strdup(buf);
		return token;
	}

	if (isdigit(peek())) {
		char buf[512];
		size_t i = 0;
		while (isdigit(peek())) {
			buf[i++] = next();
		}
		buf[i++] = '\0';
		Token token;
		token.type = TOKEN_INTEGER_LITERAL;
		token.values.integer = strtol(buf, NULL, 10);
		return token;
	}
	
	switch (peek()) {
	case '(':
	case ')':
	case ',':
		return Token::with_type((Token_Type) next());
		/*
	case ';':
		return Token::with_type(read_double_token(';', ';', TOKEN_DOUBLE_SEMICOLON));*/
	}
	fatal("Misplaced character %c (%d)", peek(), peek());
}

Token_Type Lexer::read_double_token(char left, char right, Token_Type double_type)
{
	assert(peek() == left);
	advance();
	if (peek() == right) {
		advance();
		return double_type;
	} else {
		return (Token_Type) left;
	}
}
