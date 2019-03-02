// Primitive types
enum Value_Type {
	VALUE_INTEGER,
	VALUE_STRING,
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

/** Reference
 * Represents a reference type in the language --- should be kept as
 * tight as possible, because the size of this is passed around in
 * every kind of value.
 */
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

/** Value
 * Represents a pass-by-value value. This is *never* pointed to,
 * always passed around by value.
 */
struct Value {
	Value_Type type;
	union {
		int integer;
		const char * string; // Strings are immutable, so they don't
							 // need to be reference types
		Reference reference;
	};
	static Value with_type(Value_Type type)
	{
		return (Value) { type };
	}
	static Value make_integer(int integer)
	{
		Value value = Value::with_type(VALUE_INTEGER);
		value.integer = integer;
		return value;
	}
	/** make_string_from_intern 
	 * Constructs a string directly from the passed in const char
	 * *. This should be an interned string that lasts the entire
	 * lifetime of the VM, meaning either a symbol or a string
	 * literal.
	 */
	static Value make_string_from_intern(const char * string)
	{
		Value value = Value::with_type(VALUE_STRING);
		value.string = string; // @GC
		return value;
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
		case VALUE_STRING: {
			builder.append(string);
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
