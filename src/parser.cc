enum Expr_Type {
	EXPR_TYPEOF,
	EXPR_VARIABLE,
	EXPR_INTEGER,
	EXPR_STRING,
	EXPR_TUPLE,
	EXPR_PRODUCT,
};

struct Expr {
	Expr_Type type;
	union {
		int integer;
		const char * variable;
		const char * string;
		List<Expr*> tuple;
		struct {
			List<const char *> symbols;
			List<Expr*> annotations;
		} product;
		struct {
			Expr * expr;
		} type_of;
	};
	static Expr * with_type(Expr_Type type)
	{
		Expr * expr = (Expr*) malloc(sizeof(Expr));
		expr->type = type;
		return expr;
	}
	char * to_string()
	{
		String_Builder builder;
		switch (type) {
		case EXPR_INTEGER: {
			char * s = itoa(integer);
			builder.append(s);
			free(s);
		} break;
		case EXPR_TUPLE: {
			builder.append("(");
			for (int i = 0; i < tuple.size; i++) {
				char * s = tuple[i]->to_string();
				builder.append(s);
				free(s);
				if (i < tuple.size - 1) builder.append(", ");
			}
			builder.append(")");
		} break;
		default:
			fatal_internal("Switch in Expr::to_string() incomplete");
		}
		return builder.final_string();
	}
	void deep_free();
};

enum Stmt_Type {
	STMT_LET,
	STMT_ASSIGN,
	STMT_PRINT,
	STMT_EXPR,
};

struct Stmt {
	Stmt_Type type;
	union {
		struct {
			const char * symbol;
			bool infer;
			Expr * annotation;
			Expr * right;
		} let;
		struct {
			Expr * left;
			Expr * right;
		} assign;
		struct {
			Expr * expr;
		} print;
		Expr * expr;
	};
	static Stmt * with_type(Stmt_Type type)
	{
		Stmt * stmt = (Stmt*) malloc(sizeof(Stmt));
		stmt->type = type;
		return stmt;
	}
	void deep_free();
};

struct Parser {
	Lexer * lexer;
	Token peek;
	Parser(Lexer * lexer);
	bool is(Token_Type type);
	bool at_end();
	Token next();
	Token expect(Token_Type type);
	Token weak_expect(Token_Type type);
	void advance();
	bool match(Token_Type type);
	Expr * parse_atom();
	Expr * parse_tuple();
	Expr * parse_structured();
	Expr * parse_type();
	Expr * parse_expr();
	Stmt * parse_stmt();
};

Parser::Parser(Lexer * lexer)
{
	this->lexer = lexer;
	this->peek = lexer->next_token();
}

bool Parser::is(Token_Type type)
{
	return peek.type == type;
}

bool Parser::at_end()
{
	return peek.type == TOKEN_EOF;
}

Token Parser::next()
{
	Token t = peek;
	advance();
	return t;
}

Token Parser::expect(Token_Type type)
{
	if (!is(type)) {
		fatal("Expected %s, got %s",
			  Token::type_to_string(type),
			  peek.to_string());
	}
	return next();
}

Token Parser::weak_expect(Token_Type type)
{
	if (!is(type)) {
		fatal("Expected %s, got %s",
			  Token::type_to_string(type),
			  peek.to_string());
	}
	return peek;
}

void Parser::advance()
{
	this->peek = lexer->next_token();
}

bool Parser::match(Token_Type type)
{
	if (is(type)) {
		advance();
		return true;
	}
	return false;
}

// The tightest unit of expression --- things like literals and variables
Expr * Parser::parse_atom()
{
	if (is(TOKEN_SYMBOL)) {
		Expr * expr = Expr::with_type(EXPR_VARIABLE);
		expr->variable = next().values.symbol;
		return expr;
	} else if (is(TOKEN_INTEGER_LITERAL)) {
		Expr * expr = Expr::with_type(EXPR_INTEGER);
		Token tok = next();
		expr->integer = tok.values.integer;
		return expr;
	} else if (is(TOKEN_STRING_LITERAL)) {
		Expr * expr = Expr::with_type(EXPR_STRING);
		Token tok = next();
		expr->string = tok.values.string;
	} else {
		fatal("Unexpected %s in expression", peek.to_string());
	}
}

Expr * Parser::parse_tuple()
{
	if (match((Token_Type) '(')) {
		// Hack for empty tuple
		if (match((Token_Type) ')')) {
			Expr * expr = Expr::with_type(EXPR_TUPLE);
			expr->tuple.alloc();
			return expr;
		}
		// Lookahead and backtrack
		Expr * left = parse_expr();
		if (match((Token_Type) ',')) {
			// Tuple
			Expr * expr = Expr::with_type(EXPR_TUPLE);
			expr->tuple.alloc();
			expr->tuple.push(left);
			while (true) {
				if (match((Token_Type) ')')) break;
				expr->tuple.push(parse_expr());
				if (!match((Token_Type) ',')) {
					expect((Token_Type) ')');
					break;
				}
			}
			return expr;
		} else {
			// Just a parenthesized expr
			expect((Token_Type) ')');
			return left;
		}
	} else {
		return parse_atom();
	}
}

Expr * Parser::parse_structured()
{
	if (match(TOKEN_TYPEOF)) {
		Expr * expr = Expr::with_type(EXPR_TYPEOF);
		expr->type_of.expr = parse_expr();
		return expr;
	}
	/*
	if (match(TOKEN_PRODUCT)) {
		expect((Token_Type) '{');
		Expr * expr = Expr::with_type(EXPR_PRODUCT);
		expr->product.symbols.alloc();
		expr->product.annotations.alloc();
		while (true) {
			if (match((Token_Type) '}')) break;
			weak_expect(TOKEN_SYMBOL);
			expr->product.symbols.push(next().values.symbol);
			expect((Token_Type) ':');
			weak_expect(TOKEN_SYMBOL);
			expr->product.annotations.push(parse_type());
			if (!match((Token_Type) ',')) {
				expect((Token_Type) '}');
				break;
			}
		}
		return expr;
		}*/
	return parse_tuple();
}

Expr * Parser::parse_expr()
{
	return parse_structured();
}

Stmt * Parser::parse_stmt()
{
	if (match(TOKEN_LET)) {
		Stmt * stmt = Stmt::with_type(STMT_LET);
		weak_expect(TOKEN_SYMBOL);
		stmt->let.symbol = next().values.symbol;
		expect((Token_Type) ':');
		stmt->let.infer = true;
		if (!is((Token_Type) '=')) {
			stmt->let.annotation = parse_expr();
			stmt->let.infer = false;
		}
		expect((Token_Type) '=');
		stmt->let.right = parse_expr();
		expect((Token_Type) ';');
		return stmt;
	} else if (match(TOKEN_PRINT)) {
		Stmt * stmt = Stmt::with_type(STMT_PRINT);
		stmt->print.expr = parse_expr();
		expect((Token_Type) ';');
		return stmt;
	} else {
		Expr * left = parse_expr();
		if (match((Token_Type) '=')) {
			Stmt * stmt = Stmt::with_type(STMT_ASSIGN);
			stmt->assign.left = left;
			stmt->assign.right = parse_expr();
			expect((Token_Type) ';');
			return stmt;
		} else {
			Stmt * stmt = Stmt::with_type(STMT_EXPR);
			stmt->expr = left;
			expect((Token_Type) ';');
			return stmt;
		}
	}
}

/*
enum Expr_Type {
	EXPR_NIL,
	EXPR_INTEGER,
	EXPR_TUPLE,
	EXPR_VARIABLE,
	EXPR_UNARY,
	EXPR_BINARY,
	EXPR_FUNCALL,
	EXPR_IF_THEN_ELSE,
};

enum Unary_Op {
	UNARY_MINUS,
};

enum Binary_Op {
	BINARY_PLUS,
	BINARY_MINUS,
	BINARY_MULTIPLY,
	BINARY_DIVIDE,
};

struct Expr {
	Expr_Type type;
	union {
		int integer;
		List<Expr*> tuple;
		const char * variable;
		struct {
			Unary_Op op;
			Expr * expr;
		} unary;
		struct {
			Binary_Op op;
			Expr * left;
			Expr * right;
		} binary;
		struct {
			const char * symbol;
			List<Expr*> arguments;
		} funcall;
		struct {
			Expr * _if;
			Expr * _then;
			Expr * _else;
		} if_then_else;
	};
	static Expr * with_type(Expr_Type type)
	{
		Expr * expr = (Expr*) malloc(sizeof(Expr));
		expr->type = type;
		return expr;
	}
	char * to_string()
	{
		switch (type) {
		case EXPR_INTEGER:
			return itoa(integer);
		case EXPR_VARIABLE:
			return strdup(variable);
		case EXPR_UNARY: {
			String_Builder builder;
			builder.append("(");
			switch (unary.op) {
			case UNARY_MINUS:
				builder.append("-");
				break;
			default:
				fatal_internal("Expr::to_string for unary expressions is incomplete");
			}
			char * expr_s = unary.expr->to_string();
			builder.append(expr_s);
			free(expr_s);
			builder.append(")");
			return builder.final_string();
		}
		case EXPR_BINARY: {
			String_Builder builder;
			builder.append("(");
			{
				char * left_s = binary.left->to_string();
				builder.append(left_s);
				free(left_s);
			}
			switch (binary.op) {
			case BINARY_PLUS:
				builder.append("+");
				break;
			case BINARY_MINUS:
				builder.append("-");
				break;
			case BINARY_MULTIPLY:
				builder.append("*");
				break;
			case BINARY_DIVIDE:
				builder.append("/");
				break;								
			}
			{
				char * right_s = binary.right->to_string();
				builder.append(right_s);
				free(right_s);
			}
			builder.append(")");
			return builder.final_string();
		}
		case EXPR_FUNCALL: {
			String_Builder builder;
			builder.append("(");
			builder.append(funcall.symbol);
			builder.append(": (");
			for (int i = 0; i < funcall.arguments.size; i++) {
				char * s = funcall.arguments[i]->to_string();
				builder.append(s);
				free(s);
				if (i < funcall.arguments.size - 1) {
					builder.append(" . ");
				}
			}
			builder.append("))");
			return builder.final_string();
		}
		default: {
			fatal_internal("Expr::to_string ran on expression without procedure");
		}
		}
	}
};

struct Job_Spec {
	const char * left;
	Expr * right;
	char * to_string()
	{
		String_Builder builder;
		builder.append("[");
		if (left) {
			builder.append(left);
		} else {
			builder.append("_");
		}
		builder.append(" <- ");
		{
			char * right_s = right->to_string();
			builder.append(right_s);
			free(right_s);
		}
		builder.append("]");
		return builder.final_string();
	}
};

struct Parser {
	Lexer * lexer;
	Token peek;
	Parser(Lexer * lexer);
	bool is(Token_Type type);
	bool at_end();
	Token next();
	Token expect(Token_Type type);
	Token weak_expect(Token_Type type);
	void advance();
	Expr * parse_atom();
	Expr * parse_tuple();
	Expr * parse_expression();
	Expr * parse_expr_09();
	Expr * parse_expr_10();
	Expr * parse_expr_11();
	Expr * parse_expr_12();
	Expr * parse_expr_13();
	Job_Spec * parse_job_spec();
	List<Job_Spec*> parse_frame_spec();
};

Parser::Parser(Lexer * lexer)
{
	this->lexer = lexer;
	this->peek = lexer->next_token();
}

bool Parser::is(Token_Type type)
{
	return peek.type == type;
}

bool Parser::at_end()
{
	return peek.type == TOKEN_EOF;
}

Token Parser::next()
{
	Token t = peek;
	advance();
	return t;
}

Token Parser::expect(Token_Type type)
{
	if (!is(type)) {
		fatal("Expected %s, got %s",
			  Token::type_to_string(type),
			  peek.to_string());
	}
	return next();
}

Token Parser::weak_expect(Token_Type type)
{
	if (!is(type)) {
		fatal("Expected %s, got %s",
			  Token::type_to_string(type),
			  peek.to_string());
	}
	return peek;
}

void Parser::advance()
{
	this->peek = lexer->next_token();
}

Expr * Parser::parse_expression()
{
	return parse_expr_13();
}

Expr * Parser::parse_atom()
{
	switch (peek.type) {
	case TOKEN_NIL: {
		advance();
		return Expr::with_type(EXPR_NIL);
	}
	case TOKEN_INTEGER_LITERAL: {
		Expr * expr = Expr::with_type(EXPR_INTEGER);
		expr->integer = expect(TOKEN_INTEGER_LITERAL).values.integer;
		return expr;
	}
	case '(': {
		advance();
		Expr * expr = parse_expression();
		expect((Token_Type) ')');
		return expr;
	}
	case '[': {
		return parse_tuple();
	}
	default: {
		fatal("Expected some kind of atom, got %s", peek.to_string());
	}
	}	
}

Expr * Parser::parse_tuple()
{
	List<Expr*> tuple;
	tuple.alloc();
	expect((Token_Type) '[');
	while (true) {
		if (is((Token_Type) ']')) {
			advance();
			break;
		}
		tuple.push(parse_expression());
	}
	Expr * expr = Expr::with_type(EXPR_TUPLE);
	expr->tuple = tuple;
	return expr;
}

Expr * Parser::parse_expr_09()
{
	if (is(TOKEN_IF)) {
		advance();
		Expr * expr = Expr::with_type(EXPR_IF_THEN_ELSE);
		expr->if_then_else._if = parse_expression();
		expect(TOKEN_THEN);
		expr->if_then_else._then = parse_expression();
		if (is(TOKEN_ELSE)) {
			advance();
			expr->if_then_else._else = parse_expression();
		} else {
			expr->if_then_else._else = NULL;
		}
	} else {
		return parse_atom();
	}
}

Expr * Parser::parse_expr_10()
{
	if (is(TOKEN_SYMBOL)) {
		Token symbol_tok = next();
		if (is((Token_Type) ':')) {
			// Function call
			advance();
			Expr * expr = Expr::with_type(EXPR_FUNCALL);
			expr->funcall.symbol = symbol_tok.values.symbol;
			expr->funcall.arguments = parse_tuple()->tuple; // @temporary
			return expr;
		} else {
			// Variable
			Expr * expr = Expr::with_type(EXPR_VARIABLE);
			expr->variable = symbol_tok.values.symbol;
			return expr;
		}
	} else {
		return parse_expr_09();
	}
}

Expr * Parser::parse_expr_11()
{
	if (is((Token_Type) '-')) {
		Expr * expr = Expr::with_type(EXPR_UNARY);
		expr->unary.op = UNARY_MINUS;
		advance();
		expr->unary.expr = parse_expr_11();
		return expr;
	} else {
		return parse_expr_10();
	}
}

Expr * Parser::parse_expr_12()
{
	Expr * left = parse_expr_11();
	while (is((Token_Type) '*') || is((Token_Type) '/')) {
		Expr * expr = Expr::with_type(EXPR_BINARY);
		switch (peek.type) {
		case '*':
			expr->binary.op = BINARY_MULTIPLY;
			break;
		case '/':
			expr->binary.op = BINARY_DIVIDE;
			break;
		}
		advance();
		expr->binary.left = left;
		expr->binary.right = parse_expr_12();
		left = expr;
	}
	return left;
}

Expr * Parser::parse_expr_13()
{
	Expr * left = parse_expr_12();
	while (is((Token_Type) '+') || is((Token_Type) '-')) {
		Expr * expr = Expr::with_type(EXPR_BINARY);
		switch (peek.type) {
		case '+':
			expr->binary.op = BINARY_PLUS;
			break;
		case '-':
			expr->binary.op = BINARY_MINUS;
			break;
		}
		advance();
		expr->binary.left = left;
		expr->binary.right = parse_expr_13();
		left = expr;
	}
	return left;
}

Job_Spec * Parser::parse_job_spec()
{
	const char * symbol;
	if (is(TOKEN_PLACEHOLDER)) {
		symbol = NULL;
		advance();
	} else {
		weak_expect(TOKEN_SYMBOL);
		symbol = peek.values.symbol;
		advance();
	}
	expect(TOKEN_LEFT_ARROW);
	Expr * right = parse_expression();
	
	Job_Spec * spec = (Job_Spec*) malloc(sizeof(Job_Spec));
	spec->left = symbol;
	spec->right = right;
	return spec;
}

List<Job_Spec*> Parser::parse_frame_spec()
{
	List<Job_Spec*> frame_spec;
	frame_spec.alloc();
	while (true) {
		frame_spec.push(parse_job_spec());
		if (is((Token_Type) ';')) {
			advance();
			break;
		} else if (!is((Token_Type) ',')) {
			fatal("Expected , or ;, got %s", peek.to_string());
		}
		advance();
	}
	return frame_spec;
}
*/
