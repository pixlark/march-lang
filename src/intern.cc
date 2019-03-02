namespace Intern {
	List<const char*> interns;
	void init()
	{
		interns.alloc();
	}
	void destroy_everything()
	{
		for (int i = 0; i < interns.size; i++) {
			free((void*) interns[i]);
		}
		interns.dealloc();
	}
	const char * intern(const char * s)
	{
		for (int i = 0; i < interns.size; i++) {
			if (strcmp(s, interns[i]) == 0) {
				return interns[i];
			}
		}
		interns.push(strdup(s));
		return interns[interns.size - 1];
	}
}
