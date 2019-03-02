/** 
 * The classes in this file are so incestous that we just declare
 * every class before any implementation.
 */

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

/** Reference
 * Represents a reference type in the language --- should be kept as
 * tight as possible, because the size of this is passed around in
 * every kind of value.
 */
struct Reference {
	Obj_Type type;
	void * ptr;
	char * to_string();
	void mark_for_gc();
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
		value.string = string;
		return value;
	}
	void mark_for_gc();
	char * to_string();
};

/*
 * Objects
 */

struct Obj_Tuple {
	size_t length;
	Value * elements;

	char * to_string();
	void mark_for_gc();
};

/*
 * Value
 */

void Value::mark_for_gc()
{
	// GC is only necessary for dynamically allocated values,
	// which will ALWAYS be accessed through a reference
	if (type == VALUE_REFERENCE) {
		reference.mark_for_gc();
	}
}

char * Value::to_string() {
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

/*
 * Reference
 */

char * Reference::to_string()
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

void Reference::mark_for_gc()
{
	switch (type) {
	case OBJ_TUPLE: {
		Collection::mark_ptr(ptr);
		((Obj_Tuple*) ptr)->mark_for_gc();
	} break;
	default:
		fatal_internal("Incomplete switch at Reference::mark_for_gc()");
	}
}

/*
 * Obj_Tuple
 */

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

void Obj_Tuple::mark_for_gc()
{
	Collection::mark_ptr(elements);
	for (int i = 0; i < length; i++) {
		elements[i].mark_for_gc();
	}
}
