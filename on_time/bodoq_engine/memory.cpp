#include "memory.h"
#include "debug.h"

LinearAllocator::LinearAllocator(size_t sizeInBytes)
{
	memory = new uint8_t[sizeInBytes];
	capacity = sizeInBytes;
	offset = 0;
}

LinearAllocator::~LinearAllocator()
{
	delete[] memory;
}

void* LinearAllocator::allocate(size_t size)
{
	void* ptr = mem::alignPtr((void*)(memory + offset), 8);
	uintptr_t diff = (uintptr_t)ptr - (uintptr_t)(memory + offset);
	offset += size + diff;
	DBG_ASSERT(offset <= capacity, "Exhausted memory");
	return ptr;
}
