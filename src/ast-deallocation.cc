void Expr::deep_free()
{
	switch (type) {
	case EXPR_VARIABLE:
		// Variable symbols are interned globally, shouldn't be freed (right?)
		break;
	case EXPR_INTEGER:
		// Primitive
		break;
	case EXPR_STRING:
		// Literal strings are interned globally, shouldn't be freed (right?)
		break;
	case EXPR_TUPLE: {
		for (int i = 0; i < tuple.size; i++) {
			tuple[i]->deep_free();
			free(tuple[i]);
		}
		tuple.dealloc();
	} break;
	case EXPR_TYPEOF: {
		type_of.expr->deep_free();
		free(type_of.expr);
	} break;
	default:
		fatal("Switch in Expr::deep_free() incomplete");
	}
}

void Stmt::deep_free()
{
	switch (type) {
	case STMT_LET: {
		let.right->deep_free();
		free(let.right);
	} break;
	case STMT_ASSIGN: {
		assign.left->deep_free();
		free(assign.left);
		assign.right->deep_free();
		free(assign.right);
	} break;
	case STMT_PRINT: {
		print.expr->deep_free();
		free(print.expr);
	} break;
	case STMT_EXPR: {
		expr->deep_free();
		free(expr);
	} break;
	default:
		fatal("Switch in Stmt::deep_free() incomplete");
	}
}
