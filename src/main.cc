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
#include "string-builder.cc"
#include "intern.cc"
#include "lexer.cc"
#include "parser.cc"
#include "ast-deallocation.cc"
#include "value.cc"
#include "symbol-table.cc"

enum Instr_Type {
	INSTR_POP_AND_OUTPUT,
	INSTR_POP_AND_LOOKUP,
	INSTR_PUSH,
	INSTR_MAKE_TUPLE,
	INSTR_BIND,
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
	void dealloc()
	{
		source.dealloc();
	}
	void compile_expr(Expr * expr)
	{
		switch (expr->type) {
		case EXPR_VARIABLE: {
			source.push(Instr::with_type_and_arg(INSTR_PUSH,
												 Value::make_string_from_intern(expr->variable)));
			source.push(Instr::with_type(INSTR_POP_AND_LOOKUP));
		} break;
		case EXPR_INTEGER: {
			source.push(Instr::with_type_and_arg(INSTR_PUSH,
												 Value::make_integer(expr->integer)));
		} break;
		case EXPR_STRING: {
			source.push(Instr::with_type_and_arg(INSTR_PUSH,
												 Value::make_string_from_intern(expr->string)));
		} break;
		case EXPR_TUPLE: {
			for (int i = 0; i < expr->tuple.size; i++) {
				compile_expr(expr->tuple[i]);
			}
			source.push(Instr::with_type_and_arg(INSTR_MAKE_TUPLE,
												 Value::make_integer(expr->tuple.size)));
		} break;
		case EXPR_LET: {
			compile_expr(expr->let.right);
			source.push(Instr::with_type_and_arg(INSTR_BIND,
												 Value::make_string_from_intern(expr->let.symbol)));
		} break;
		default:
			fatal_internal("Switch in Compiler::compile_expr() incomplete");
			break;
		}
	}
};

struct VM {
	Instr * program;
	size_t program_length;
	size_t program_counter;
	bool halted;

	Symbol_Table global_table;
	List<Value> op_stack;
	static VM create()
	{
		VM vm;
		vm.program = NULL;
		vm.program_length = 0;
		vm.program_counter = 0;
		vm.halted = true;
		vm.global_table.alloc();
		vm.op_stack.alloc();
		return vm;
	}
	// NOTE: Does no deep freeing
	void destroy()
	{
		global_table.dealloc();
		op_stack.dealloc();
	}
	void prime(Instr * program, size_t program_length)
	{
		assert(op_stack.size == 0); // Everything should be empty between statements
		this->program = program;
		this->program_length = program_length;
		program_counter = 0;
		halted = false;
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
		case INSTR_POP_AND_LOOKUP: {
			Value v = op_stack.pop();
			assert(v.type == VALUE_STRING);
			// TODO(pixlark): Dangerous! We have no assertion that the
			// string we pop is interned! How do we solve this?
			int index = global_table.find(v.string);
			if (index == -1) {
				fatal("Tried to lookup nonexistent variable %s", v.string);
			}
			op_stack.push(global_table.values[index]);
		} break;
		case INSTR_PUSH: {
			op_stack.push(instr.argument);
		} break;
		case INSTR_MAKE_TUPLE: {
			assert(instr.argument.type == VALUE_INTEGER);
			assert(instr.argument.integer >= 0);
			
			Obj_Tuple * tuple = (Obj_Tuple*) malloc(sizeof(Obj_Tuple)); // @GC
			tuple->length = instr.argument.integer;
			tuple->elements = (Value*) malloc(sizeof(Value) * tuple->length); // @GC
			for (int i = tuple->length - 1; i >= 0; i--) {
				tuple->elements[i] = op_stack.pop();
			}
			
			Value v = Value::with_type(VALUE_REFERENCE);
			v.reference = Reference::to(tuple, OBJ_TUPLE);
			op_stack.push(v);
		} break;
		case INSTR_BIND: {
			assert(instr.argument.type == VALUE_STRING);
			const char * symbol = instr.argument.string;
			if (global_table.find(symbol) != -1) {
				fatal("Tried to declare variable %s which is already bound", symbol);
			}
			// Don't pop because it's an expression - the return value of let is what gets let
			Value to_bind = op_stack[op_stack.size - 1];
			global_table.set(symbol, to_bind);
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

	Intern::init();
	Lexer    lexer(source);
	Parser   parser(&lexer);
	VM vm = VM::create();
	
	while (!parser.at_end()) {
		Expr * expr = parser.parse_expr();

		/*
		char * s = expr->to_string();
		printf("%s\n", s);
		free(s);*/

		Compiler compiler;
		compiler.alloc();
		compiler.compile_expr(expr);
		compiler.source.push(Instr::with_type(INSTR_POP_AND_OUTPUT));
		vm.prime(compiler.source.arr, compiler.source.size);
		while (!vm.halted) {
			vm.step();
		}
		compiler.dealloc();

		expr->deep_free();
		free(expr);
	}

	vm.destroy();
	free((void*) source);
	Intern::destroy_everything();
}
