#pragma once

#include <stdint.h>
#include "string.h"

namespace hash
{
	uint64_t murmurHash(const void* key, uint64_t keyLength, uint64_t seed);

	template<typename T> inline uint64_t hash(const T& data)
	{
		return murmurHash(&data, sizeof(data), 0);
	}

	template<> inline uint64_t hash<String>(const String& str)
	{
		return murmurHash(*str, str.length(), 0);
	}

	template<> inline uint64_t hash<WString>(const WString& str)
	{
		return murmurHash(*str, str.length() * sizeof(wchar_t), 0);
	}

}