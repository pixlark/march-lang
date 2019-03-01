#include <assert.h>
#include <ctype.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "list.h" // Necessary evil

// Unity build
#include "utility.cc"
#include "error.cc"
#include "string_builder.cc"
#include "lexer.cc"
#include "parser.cc"
#include "value.cc"

enum Instr_Type {
	INSTR_POP_AND_OUTPUT,
	INSTR_PUSH,
	INSTR_MAKE_TUPLE,
};

struct Instr {
	Instr_Type type;
	Value argument;
	static Instr with_type(Instr_Type type)
	{
		return (Instr) { type };
	}
	static Instr with_type_and_arg(Instr_Type type,
								   Value argument)
	{
		return (Instr) { type, argument };
	}
};

struct Compiler {
	List<Instr> source;
	void alloc()
	{
		source.alloc();
	}
	void compile_expr(Expr * expr)
	{
		switch (expr->type) {
		case EXPR_INTEGER: {
			source.push(Instr::with_type_and_arg(INSTR_PUSH,
												 Value::make_integer(expr->integer)));
		} break;
		case EXPR_TUPLE: {
			for (int i = 0; i < expr->tuple.size; i++) {
				compile_expr(expr->tuple[i]);
			}
			source.push(Instr::with_type_and_arg(INSTR_MAKE_TUPLE,
												 Value::make_integer(expr->tuple.size)));
		} break;
		}
	}
};

struct VM {
	Instr * program;
	size_t program_length;
	size_t program_counter;
	bool halted;

	List<Value> op_stack;
	static VM create(Instr * program, size_t program_length)
	{
		VM vm;
		vm.program = program;
		vm.program_length = program_length;
		vm.program_counter = 0;
		vm.halted = false;
		vm.op_stack.alloc();
		return vm;
	}
	void step()
	{
		if (program_counter >= program_length) {
			halted = true;
			return;
		}
		Instr instr = program[program_counter++];
		switch (instr.type) {
		case INSTR_POP_AND_OUTPUT: {
			Value v = op_stack.pop();
			char * s = v.to_string();
			printf("%s\n", s);
			free(s);
		} break;
		case INSTR_PUSH: {
			op_stack.push(instr.argument);
		} break;
		case INSTR_MAKE_TUPLE: {
			assert(instr.argument.type == VALUE_INTEGER);
			MTuple tuple;
			tuple.length = instr.argument.integer;
			tuple.elements = (Value*) malloc(sizeof(Value) * tuple.length);
			for (int i = tuple.length - 1; i >= 0; i--) {
				tuple.elements[i] = op_stack.pop();
			}
			Value v = Value::with_type(VALUE_TUPLE);
			v.tuple = tuple;
			op_stack.push(v);
		} break;
		default:
			fatal_internal("Incomplete switch in VM::step()");
			break;
		}
	}
};

int main(int argc, char ** argv)
{
	if (argc != 2) {
		printf("Provide one source file.\n");
		return 1;
	}

	const char * source = load_string_from_file(argv[1]);
	if (!source) {
		printf("File does not exist.\n");
		return 1;
	}
	
	Lexer    lexer(source);
	Parser   parser(&lexer);
	
	while (!parser.at_end()) {
		Expr * expr = parser.parse_expression();
		/*
		char * s = expr->to_string();
		printf("%s\n", s);
		free(s);
		*/
		Compiler compiler;
		compiler.alloc();
		compiler.compile_expr(expr);
		VM vm = VM::create(compiler.source.arr, compiler.source.size);
		while (!vm.halted) {
			vm.step();
		}
		printf("%s\n", vm.op_stack.pop().to_string());
	}
	
	/*
	Instr program[] = {
		Instr::with_type_and_arg(INSTR_PUSH, Value::make_integer(12)),
		Instr::with_type_and_arg(INSTR_PUSH, Value::make_integer(13)),
		Instr::with_type_and_arg(INSTR_MAKE_TUPLE, Value::make_integer(2)),
		Instr::with_type(INSTR_POP_AND_OUTPUT),
	};
	auto vm = VM::create(program, sizeof(program) / sizeof(Instr));
	while (!vm.halted) {
		vm.step();
		}*/
}
