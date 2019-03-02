// Primitive types
enum Value_Type {
	VALUE_INTEGER,
	VALUE_REFERENCE,
};

// Reference types
enum Obj_Type {
	OBJ_TUPLE,
};

const char * obj_type_to_string(Obj_Type type)
{
	switch (type) {
	case OBJ_TUPLE:
		return "tuple";
	default:
		fatal_internal("switch in obj_type_to_string() incomplete");
	}
}

struct Value;

struct Obj_Tuple {
	size_t length;
	Value * elements;
	char * to_string();
};

struct Reference {
	Obj_Type type;
	void * ptr;
	char * to_string()
	{
		switch (type) {
		case OBJ_TUPLE: {
			return ((Obj_Tuple*) ptr)->to_string();
		} break;
		default: {
			String_Builder builder;
			builder.append("<");
			builder.append(obj_type_to_string(type));
			builder.append(" at ");
			char buf[100]; // Far too big for a pointer string to exceed
			sprintf(buf, "%p", ptr);
			builder.append(buf);
			builder.append(">");
			return builder.final_string();
		} break;
		}
	}
	static Reference to(void * ptr, Obj_Type type)
	{
		return (Reference) { type, ptr };
	}
};

struct Value {
	Value_Type type;
	union {
		int integer;
		Reference reference;
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
		case VALUE_REFERENCE: {
			char * s = reference.to_string();
			builder.append(s);
			free(s);
		} break;
		default:
			fatal_internal("Incomplete switch in Value::to_string()");
			break;
		}
		return builder.final_string();
	}
};

char * Obj_Tuple::to_string()
{
	String_Builder builder;
	builder.append("(");
	for (int i = 0; i < length; i++) {
		char * s = elements[i].to_string();
		builder.append(s);
		free(s);
		if (i < length - 1) builder.append(", "); 
	}
	builder.append(")");
	return builder.final_string();
}
