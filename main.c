#define MALLOC_LOG 1
#include <stdio.h>
#include <strings.h>
#include "slotallocator.h"

SlotAllocator allocator;

void *talloc(size_t size)
{
	return slotalloc(&allocator, size);
}

char *tconcat(char *a, char *b)
{
	size_t strlen_a = strlen(a);
	size_t strlen_b = strlen(b);
	size_t total_len = strlen_a + strlen_b;
	char *temp_alloc = talloc(total_len + 1);
	memcpy(temp_alloc, a, strlen_a);
	memcpy(temp_alloc + strlen_a, b, strlen_b);
	temp_alloc[total_len] = 0;
	return temp_alloc;
}

void test()
{
	char *a = "a";
	char *b = "c";
	for (int i = 0; i < 1000000; i++)
	{
		a = tconcat(a, tconcat(tconcat(b, "c"), "d"));
		b = tconcat(b, a);
		if (strlen(a) > 1024) a[1024] = 0;
		if (strlen(b) > 1024) b[1024] = 0;
	}
	printf("Result was %s\n", a);
}

int main()
{
	size_t page_size = 4096;
	size_t page_count = 4;
	slotinit(&allocator, malloc(page_count * page_size), page_size, page_count);
	test();
	return 0;
}
