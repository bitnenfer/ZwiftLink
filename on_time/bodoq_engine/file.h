#pragma once

#include "memory.h"
#include "string.h"

namespace file
{
	void* readFile(const String& filePath, size_t& outFileSize);
	bool writeFile(const String& filePath, const void* data, size_t dataSize);
}