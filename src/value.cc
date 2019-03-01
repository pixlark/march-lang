enum Value_Type {
	VALUE_INTEGER,
	VALUE_TUPLE,
};

struct Value;

struct MTuple {
	size_t length;
	Value * elements;
};
	
struct Value {
	Value_Type type;
	union {
		int    integer;
		MTuple tuple;
	};
	static Value with_type(Value_Type type)
	{
		return (Value) { type };
	}
	static Value make_integer(int integer)
	{
		return (Value) { VALUE_INTEGER, integer };
	}
	char * to_string()
	{
		String_Builder builder;
		switch (type) {
		case VALUE_INTEGER: {
			char * s = itoa(integer);
			builder.append(s);
			free(s);
		} break;
		case VALUE_TUPLE: {
			builder.append("(");
			for (int i = 0; i < tuple.length; i++) {
				char * s = tuple.elements[i].to_string();
				builder.append(s);
				free(s);
				if (i < tuple.length - 1) builder.append(", "); 
			}
			builder.append(")");
		} break;
		default:
			fatal_internal("Incomplete switch in Value::to_string()");
			break;
		}
		return builder.final_string();
	}
};
