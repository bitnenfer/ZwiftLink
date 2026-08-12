#pragma once

#include "array.h"

template<typename TChar = char>
struct TString
{
	static constexpr size_t npos = static_cast<size_t>(~0);

	TString() 
	{
		chars.add(TChar(0));
	}
	TString(size_t count, TChar ch)
	{
		chars.fill(count, ch);
		chars.add(TChar(0));
	}
	TString(const TString<TChar>& copyRef)
	{
		chars = copyRef.chars;
	}
	TString(TString<TChar>&& moveRef) noexcept
	{
		chars = std::move(moveRef.chars);
	}
	TString(const TChar* cstr)
	{
		if (cstr != nullptr)
		{
			while (*cstr != TChar(0))
			{
				chars.add(*cstr);
				++cstr;
			}
		}
		chars.add(TChar(0));
	}
	TString(const TChar& singleChar)
	{
		this->operator+=(singleChar);
	}
	TString& operator=(const TChar* cstr)
	{
		chars.reset();
		if (cstr != nullptr)
		{
			while (*cstr != TChar(0))
			{
				chars.add(*cstr);
				++cstr;
			}
		}
		chars.add(TChar(0));
		return *this;
	}
	TString& operator=(const TString<TChar>& copyRef)
	{
		chars = copyRef.chars;
		return *this;
	}
	TString& operator=(TString<TChar>&& moveRef) noexcept
	{
		chars = std::move(moveRef.chars);
		return *this;
	}
	void fill(size_t count, const TChar& c)
	{
		chars.clear();
		chars.fill(count, c);
		chars.add(TChar(0));
	}
	const TChar* rawStr() const
	{
		if (chars.getNum() == 0)
		{
			return nullptr;
		}
		return &chars[0];
	}
	TString substr(size_t pos, size_t count) const
	{
		TString out;
		out.chars.clear();

		const size_t len = length();
		if (pos >= len)
		{
			// empty
			out.chars.add(TChar(0));
			return out;
		}

		size_t maxCount = len - pos;
		if (count == npos || count > maxCount)
			count = maxCount;

		for (size_t i = 0; i < count; ++i)
			out.chars.add(chars[pos + i]);

		out.chars.add(TChar(0));
		return out;
	}

	TString substr(size_t pos) const
	{
		return substr(pos, npos);
	}
	size_t length() const
	{
		return (size_t)chars.getNum() - 1;
	}
	size_t byteLength() const
	{
		return chars.getNumInByteSize();
	}
	size_t size() const
	{
		return chars.getNum();
	}
	TString& operator+=(const TString<TChar>& other)
	{
		if (chars.getNum() > 0)
		{
			chars.remove(chars.getNum() - 1);
		}
		for (size_t index = 0; index < other.length(); ++index)
		{
			chars.add(other.chars[index]);
		}
		chars.add(TChar(0));
		return *this;
	}
	TString& operator+=(const TChar* cstr)
	{
		if (chars.getNum() > 0)
		{
			chars.remove(chars.getNum() - 1);
		}
		if (cstr != nullptr)
		{
			while (*cstr != TChar(0))
			{
				chars.add(*cstr);
				++cstr;
			}
		}
		chars.add(TChar(0));
		return *this;
	}
	TString& operator+=(TChar c)
	{
		if (chars.getNum() > 0)
		{
			chars.remove(chars.getNum() - 1);
		}
		chars.add(c);
		chars.add(TChar(0));
		return *this;
	}
	bool operator==(const TString<TChar>& other) const
	{
		if (length() != other.length())
		{
			return false;
		}
		for (size_t index = 0; index < length(); ++index)
		{
			if (chars[index] != other.chars[index])
			{
				return false;
			}
		}
		return true;
	}
	bool operator!=(const TString<TChar>& other) const
	{
		return !(*this == other);
	}
	bool operator==(const TChar* cstr) const
	{
		if (cstr == nullptr)
		{
			return length() == 0;
		}
		size_t index = 0;
		while (index < length() && cstr[index] != TChar(0))
		{
			if (chars[index] != cstr[index])
			{
				return false;
			}
			++index;
		}
		return index == length() && cstr[index] == TChar(0);
	}
	bool operator!=(const TChar* cstr) const
	{
		return !(*this == cstr);
	}
	size_t find(const TString<TChar>& substring, size_t startIndex = 0) const
	{
		if (substring.length() == 0 || substring.length() > length() || startIndex >= length())
		{
			return npos;
		}
		for (size_t i = startIndex; i <= length() - substring.length(); ++i)
		{
			bool match = true;
			for (size_t j = 0; j < substring.length(); ++j)
			{
				if (chars[i + j] != substring.chars[j])
				{
					match = false;
					break;
				}
			}
			if (match)
			{
				return i;
			}
		}
		return npos;
	}
	size_t find(const TChar* cstr, size_t startIndex = 0) const
	{
		if (cstr == nullptr)
		{
			return npos;
		}
		TString<TChar> substring(cstr);
		return find(substring, startIndex);
	}
	const TChar* operator*() const
	{
		return rawStr();
	}
	const TChar& operator[](size_t index) const
	{
		return chars[index];
	}
	operator const TChar*()
	{
		return rawStr();
	}

	TChar* begin() { return chars.begin(); }
	TChar* end() { return chars.end(); }
	const TChar* begin() const { return chars.begin(); }
	const TChar* end()   const { return chars.end(); }

private:
	TArray<TChar, size_t> chars;
};

using String = TString<char>;
using WString = TString<wchar_t>;

inline String operator+(const String& a, const String& b)
{
	String result = a;
	result += b;
	return result;
}

inline WString operator+(const WString& a, const WString& b)
{
	WString result = a;
	result += b;
	return result;
}

namespace str
{
	String format(const char* fmt, ...);
	String formatv(const char* fmt, va_list args);
	WString format(const wchar_t* fmt, ...);
	WString formatv(const wchar_t* fmt, va_list args);

	inline String toString(int8_t value) { return format("%d", value); }
	inline String toString(uint8_t value) { return format("%u", value); }
	inline String toString(int32_t value) { return format("%d", value); }
	inline String toString(uint32_t value) { return format("%u", value); }
	inline String toString(int64_t value) { return format("%lld", value); }
	inline String toString(uint64_t value) { return format("%llu", value); }
	inline String toString(float value) { return format("%f", value); }
	inline String toString(double value) { return format("%lf", value); }
	inline String toString(const WString& wstr)
	{
		String str;
		for (size_t index = 0; index < wstr.length(); ++index)
		{
			str += (char)wstr[index];
		}
		return str;
	}

	inline WString toStringW(int8_t value) { return format(L"%d", value); }
	inline WString toStringW(uint8_t value) { return format(L"%u", value); }
	inline WString toStringW(int32_t value) { return format(L"%d", value); }
	inline WString toStringW(uint32_t value) { return format(L"%u", value); }
	inline WString toStringW(int64_t value) { return format(L"%lld", value); }
	inline WString toStringW(uint64_t value) { return format(L"%llu", value); }
	inline WString toStringW(float value) { return format(L"%f", value); }
	inline WString toStringW(double value) { return format(L"%lf", value); }
	inline WString toStringW(const String& str)
	{
		WString wstr;
		for (size_t index = 0; index < str.length(); ++index)
		{
			wstr += (wchar_t)str[index];
		}
		return wstr;
	}
}
