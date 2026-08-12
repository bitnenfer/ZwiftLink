#pragma once

#include <stdint.h>

#define MEM_KiB(n) ((n) * 1024)
#define MEM_MiB(n) (MEM_KiB((n)) * 1024)
#define MEM_GiB(n) (MEM_MiB((n)) * 1024)

struct LinearAllocator
{
	LinearAllocator(size_t sizeInBytes);
	~LinearAllocator();

	void* allocate(size_t size);
	void reset() { offset = 0; }

private:
	size_t capacity = 0;
	size_t offset = 0;
	uint8_t* memory = nullptr;
};

namespace mem
{
	inline void* offsetPtr(void* p, intptr_t o) { return (void*)((intptr_t)p + o); }
	inline void* alignPtr(void* p, size_t a) { return (void*)(((uintptr_t)(p)+((uintptr_t)(a)-1LL)) & ~((uintptr_t)(a)-1LL)); }
	inline size_t alignSize(size_t v, size_t a) { return ((v)+((a)-1LL)) & ~((a)-1LL); }
}

template<size_t capacity>
struct TStackLinearAllocator
{
	void* allocate(size_t size)
	{
		void* ptr = mem::alignPtr((void*)(memory + offset), 8);
		uintptr_t diff = (uintptr_t)ptr - (uintptr_t)(memory + offset);
		offset += size + diff;
		DBG_ASSERT(offset <= capacity, "Exhausted memory");
		return ptr;
	}
	void reset() { offset = 0; }

private:
	size_t offset = 0;
	uint8_t memory[capacity];
};