#include <stdexcept>
#include <new> // std::bad_alloc

#include <stdlib.h>
#include "MemMan.h"
#include "slog/slog.h"

void* XMalloc(size_t const size)
{
	void* const p = malloc(size);
	if (!p) throw std::bad_alloc();
	return p;
}

void* XRealloc(void* const ptr, size_t const size)
{
	void* const p = realloc(ptr, size);
	if (!p) throw std::bad_alloc();
	return p;
}
