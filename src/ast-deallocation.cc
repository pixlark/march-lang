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
	case EXPR_LET: {
		let.right->deep_free();
		free(let.right);
	} break;
	default:
		fatal("Switch in Expr::deep_free() incomplete");
	}
}
