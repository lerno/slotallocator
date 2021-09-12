

/*
Copyright (c) 2021 Christoffer Lernö

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <assert.h>
#include <stdlib.h>

typedef struct SlotAllocator_
{
	void *pages;
	size_t page_size;
	size_t page_count;
	size_t bitmask;
	size_t current_page;
} SlotAllocator;

void* slotinit(SlotAllocator *allocator, void *memory, size_t page_size, size_t page_count)
{
	assert(memory && "Initialized without memory");
	assert(!allocator->pages && "Allocator already initialized");
	assert(page_size > 0 && "Page size must be > 0");
	assert(page_count > 0 && "Page count must be > 0");
	assert(!(page_count & (page_count - 1)) && "Page count must be power of 2");
	allocator->pages = memory;
	allocator->page_size = page_size;
	allocator->page_count = page_count;
	allocator->bitmask = page_count - 1;
	allocator->current_page = 0;
}

void* slotalloc(SlotAllocator *allocator, size_t size)
{
	// Find the start of the current page.
	void* active_page = (char *)allocator->pages + allocator->current_page * allocator->page_size;
	// The first bytes of the page is holding an optional pointer address.
	void** page_pointer = (void**)active_page;

	// If the pointer exists, free this pointer and clear the optional pointer address.
	// this frees the spilled malloc.
	// As an optimization we may choose to reuse it of the next allocated
	// memory will also spill. For simplicity we ignore that.
	if (*page_pointer)
	{
#if MALLOC_LOG
		printf("Freeing previous malloc: %p\n", *page_pointer);
#endif
		free(*page_pointer);
		*page_pointer = NULL;
	}

	// Calculate any spill.
	size_t max_allocation_size = allocator->page_size - sizeof(page_pointer);
	void *mem;
	if (size > max_allocation_size)
	{
		// We need to fallback to malloc.
#if MALLOC_LOG
		printf("Fallback to malloc: %zu\n", size);
#endif
		mem = malloc(size);
		// This may fail, in this case return NULL.
		if (!mem) return NULL;
		// Successful allocation, we store the address of the malloc.
		*page_pointer = mem;
	}
	else
	{
		// In the normal case, we return the part of the pointer after the page.
		// note that if we want an aligned allocation we can support this here
		// by adjusting max allocation size and this offset.
		mem = &page_pointer[1];
	}

	// Move the current page forward
	allocator->current_page = (allocator->current_page + 1) & allocator->bitmask;

	return mem;
}
