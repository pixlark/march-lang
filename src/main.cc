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
#include "collection.cc"
#include "value.cc"
#include "symbol-table.cc"

enum Instr_Type {
	INSTR_POP_AND_DISCARD,
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
		default:
			fatal_internal("Switch in Compiler::compile_expr() incomplete");
			break;
		}
	}
	void compile_stmt(Stmt * stmt)
	{
		switch (stmt->type) {
		case STMT_LET: {
			compile_expr(stmt->let.right);
			source.push(Instr::with_type_and_arg(INSTR_BIND,
												 Value::make_string_from_intern(stmt->let.symbol)));
		} break;
		case STMT_PRINT: {
			compile_expr(stmt->print.expr);
			source.push(Instr::with_type(INSTR_POP_AND_OUTPUT));
		} break;
		case STMT_EXPR: {
			compile_expr(stmt->expr);
			source.push(Instr::with_type(INSTR_POP_AND_DISCARD));
		} break;
		default:
			fatal_internal("Switch in Compiler::compile_stmt() incomplete");
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
		case INSTR_POP_AND_DISCARD: {
			op_stack.pop();
		} break;
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
			
			Obj_Tuple * tuple = (Obj_Tuple*) Collection::alloc(sizeof(Obj_Tuple));
			tuple->length = instr.argument.integer;
			tuple->elements = (Value*) Collection::alloc(sizeof(Value) * tuple->length);
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
			Value to_bind = op_stack.pop();
			global_table.set(symbol, to_bind);
		} break;
		default:
			fatal_internal("Incomplete switch in VM::step()");
			break;
		}
	}
	void mark_all_bound_values()
	{
		for (int i = 0; i < global_table.values.size; i++) {
			global_table.values[i].mark_for_gc();
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
	Collection::init();
	Lexer lexer(source);
	Parser parser(&lexer);
	VM vm = VM::create();
	
	while (!parser.at_end()) {
		// Get AST
		Stmt * stmt = parser.parse_stmt();

		// Compile AST to bytecode
		Compiler compiler;
		compiler.alloc();
		compiler.compile_stmt(stmt);

		// Run bytecode
		vm.prime(compiler.source.arr, compiler.source.size);
		while (!vm.halted) {
			vm.step();
		}

		// Make sure we've progressed correctly
		assert(vm.op_stack.size == 0);
		
		// Free some stuff
		compiler.dealloc();
		stmt->deep_free();
		free(stmt);

		// Run garbage collector
		vm.mark_all_bound_values();
		size_t allocations_before = Collection::ptrs.size;
		Collection::collect_unmarked();
		printf("Collected %d references; from %d to %d\n",
			   allocations_before - Collection::ptrs.size,
			   allocations_before,  Collection::ptrs.size);
	}

	vm.destroy();
	Collection::destroy_everything();
	Intern::destroy_everything();
	free((void*) source);
}
