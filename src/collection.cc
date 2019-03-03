namespace Collection {
	List<uint8_t*> ptrs;
	void init()
	{
		ptrs.alloc();
	}
	void * alloc(size_t size)
	{
		uint8_t * raw_ptr = (uint8_t*) malloc(size + 1);
		*raw_ptr = 0;
		ptrs.push(raw_ptr);
		return (void*) (raw_ptr + 1);
	}
	void unmark_all()
	{
		for (int i = 0; i < ptrs.size; i++) {
			*(ptrs[i]) = 0;
		}
	}
	void mark_ptr(void * external_ptr)
	{
		uint8_t * raw_ptr = ((uint8_t*) external_ptr) - 1;
		*raw_ptr = 1;
	}
	void collect_unmarked()
	{
		List<uint8_t*> new_ptrs;
		new_ptrs.alloc();
		for (int i = 0; i < ptrs.size; i++) {
			if (*(ptrs[i])) {
				new_ptrs.push(ptrs[i]);
			}
		}
		ptrs.dealloc();
		ptrs = new_ptrs;
	}
	void destroy_everything()
	{
		for (int i = 0; i < ptrs.size; i++) {
			free(ptrs[i]);
		}
		ptrs.dealloc();
	}
}
