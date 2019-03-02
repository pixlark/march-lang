/** Symbol_Table
 * A structure that binds symbols to Values.
 */
struct Symbol_Table {
	List<const char*> symbols;
	List<Value>       values;
	void alloc()
	{
		symbols.alloc();
		values.alloc();
	}
	void dealloc()
	{
		symbols.dealloc();
		values.dealloc();
	}
	int find(const char * symbol)
	{
		for (int i = 0; i < symbols.size; i++) {
			if (symbols[i] == symbol) { // This only works on string
										// that have been interned!
										// i.e. Symbols and string
										// literals. Constructed
										// strings have no guarantee
										// to dereference corrrectly!
				return i;
			}
		}
		return -1;
	}
	void set(const char * symbol, Value value)
	{
		int index = find(symbol);
		if (index == -1) {
			symbols.push(symbol);
			values.push(value);
		} else {
			values[index] = value;
		}
	}
};
