#ifndef _MEMMAN_H
#define _MEMMAN_H

#include <stdlib.h>
#include <string.h>

#define MemAlloc(size)        XMalloc((size))
#define MemFree(ptr)          free((ptr))
#define MemRealloc(ptr, size) XRealloc((ptr), (size))

void* XMalloc(size_t size);
void* XRealloc(void* ptr, size_t size);

static inline void* MallocZ(const size_t n)
{
	void* const p = XMalloc(n);
	memset(p, 0, n);
	return p;
}

template<typename T> static inline void FreeNull(T*& r) throw()
{
	T* const p = r;
	if (!p) return;
	r = 0;
	free(p);
}

#define MALLOC(type)             (type*)XMalloc(sizeof(type))
#define MALLOCE(type, member, n) (type*)XMalloc(sizeof(type) + sizeof(*((type*)0)->member) * (n))
#define MALLOCN(type, count)     (type*)XMalloc(sizeof(type) * (count))
#define MALLOCNZ(type, count)    (type*)MallocZ(sizeof(type) * (count))
#define MALLOCZ(type)            (type*)MallocZ(sizeof(type))
#define REALLOC(ptr, type, count) (type*)MemRealloc(ptr, sizeof(type) * (count))

#endif /* _MEMMAN_H */
