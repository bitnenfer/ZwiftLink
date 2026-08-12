#pragma once

#include "debug.h"
#include <stdint.h>
#include <type_traits>
#include <initializer_list>
#include <new>

template<typename T, typename TSize = uint64_t>
struct TArray {
	TArray(TSize fillCount, const T& element) : data(nullptr), num(0), capacity(0)
	{
		resize(fillCount);
		fill(fillCount, element);
	}

	TArray() : data(nullptr), num(0), capacity(0) {}

	~TArray()
	{
		clear();
	}

	TArray(const TArray<T, TSize>& copyRef) : data(nullptr), num(0), capacity(0) 
	{
		if (copyRef.num > 0 && copyRef.data != nullptr) {
			resize(copyRef.num);
			copy(data, copyRef.data, copyRef.num);
			num = copyRef.num;
		}
	}

	TArray(TArray<T, TSize>&& moveRef) noexcept : data(moveRef.data), num(moveRef.num), capacity(moveRef.capacity)
	{
		if (&moveRef == this)
		{
			return;
		}
		data = moveRef.data;
		num = moveRef.num;
		capacity = moveRef.capacity;
		moveRef.data = nullptr;
		moveRef.num = 0;
		moveRef.capacity = 0;
	}

	TArray(std::initializer_list<T> initList)
		: data(nullptr), num(0), capacity(0) {
		resize(static_cast<TSize>(initList.size()));
		TSize i = 0;
		for (const auto& elem : initList) 
		{
			data[i++] = elem;
		}
		num = static_cast<TSize>(initList.size());
	}

	TArray<T, TSize>& operator=(const TArray<T, TSize>& copyRef) 
	{
		if (data != nullptr) 
		{
			clear();
		}
		if (copyRef.num > 0 && copyRef.data != nullptr) 
		{
			resize(copyRef.num);
			copy(copyRef.data, copyRef.num);
			num = copyRef.num;
		}
		return *this;
	}

	TArray<T, TSize>& operator=(TArray<T, TSize>&& moveRef) noexcept 
	{
		if (&moveRef == this)
		{
			return *this;
		}
		clear();
		data = moveRef.data;
		num = moveRef.num;
		capacity = moveRef.capacity;
		moveRef.data = nullptr;
		moveRef.num = 0;
		moveRef.capacity = 0;
		return *this;
	}

	TArray<T, TSize>& operator=(std::initializer_list<T> initList) 
	{
		clear();
		resize(static_cast<TSize>(initList.size()));
		TSize i = 0;
		for (const auto& elem : initList) 
		{
			data[i++] = elem;
		}
		num = static_cast<TSize>(initList.size());
		return *this;
	}

	bool canAccess(TSize index)
	{
		return index < num;
	}

	void reset() 
	{
		destroyRange(0, num);
		num = 0;
	}

	void clear() 
	{
		destroyRange(0, num);
		num = 0;
		capacity = 0;
		free(data);
		data = nullptr;
	}

	void insertAt(TSize index, const T& value)
	{
		DBG_ASSERT(index <= num, "insertAt: index out of range");
		checkResize();

		for (TSize i = num; i > index; --i)
		{
			new (&data[i]) T(std::move(data[i - 1]));
			data[i - 1].~T();
		}

		new (&data[index]) T(value);
		++num;
	}

	void insertAt(TSize index, T&& value)
	{
		DBG_ASSERT(index <= num, "insertAt: index out of range");
		checkResize();

		for (TSize i = num; i > index; --i)
		{
			new (&data[i]) T(std::move(data[i - 1]));
			data[i - 1].~T();
		}

		new (&data[index]) T(std::move(value));
		++num;
	}

	void fill(TSize count, const T& element)
	{
		for (TSize index = 0; index < count; ++index)
		{
			add(element);
		}
	}

	template<typename... TArgs>
	T& emplace(TArgs... args) 
	{
		checkResize();
		T* ptr = new (&data[num++]) T(std::forward<TArgs>(args)...);
		return *ptr;
	}

	void add(const T& element) 
	{
		checkResize();
		new (&data[num++]) T(element);
	}

	void add(T&& element) 
	{
		checkResize();
		new (&data[num++]) T(std::move(element));
	}

	void add(const T* elements, TSize count)
	{
		for (TSize index = 0; index < count; ++index)
		{
			add(elements[index]);
		}
	}

	void addUnique(const T& element)
	{
		for (TSize index = 0; index < num; ++index)
		{
			if (data[index] == element)
			{
				return;
			}
		}
		add(element);
	}

	void addUnique(T&& element)
	{
		for (TSize index = 0; index < num; ++index)
		{
			if (data[index] == element)
			{
				return;
			}
		}
		add(element);
	}

	bool remove(TSize index)
	{
		if (index < num)
		{
			destroyRange(index, index + 1);
			const TSize tail = num - index - 1;
			if (tail > 0)
			{
				move(&data[index], &data[index + 1], tail);
			}
			num--;
			return true;
		}
		return false;
	}

	void pop()
	{
		if (num > 0)
		{
			remove(num - 1);
		}
	}

	void resize(TSize newCapacity)
	{
		if (newCapacity == capacity) return;
		DBG_ASSERT(newCapacity > capacity, "New capacity must be larger than capacity. To clear the use the clear function.");
		if (data && newCapacity > capacity)
		{
			T* newData = (T*)malloc((size_t)newCapacity * sizeof(T));
			for (TSize i = 0; i < num; ++i)
				new (&newData[i]) T(std::move(data[i]));
			destroyRange(0, num);
			free(data);
			data = newData;
			capacity = newCapacity;
		}
		else if (!data && newCapacity > 0)
		{
			data = (T*)malloc((size_t)newCapacity * sizeof(T));
			capacity = newCapacity;
		}
		else if (data && newCapacity == 0)
		{
			clear();
		}
	}

	void checkResize() 
	{
		if (num + 1 >= capacity) 
		{
			resize(capacity > 0 ? capacity * 2 : 16);
		}
	}

	T* getData() { return data; }
	const T* getData() const { return data; }

	TSize getNum() const { return num; }
	TSize size() const { return num; }
	TSize getCapacity() const { return capacity; }
	size_t getNumInByteSize() const { return sizeof(T) * num; }
	size_t getCapacityInByteSize() const { return sizeof(T) * capacity; }

	T& last() { return data[num - 1]; }
	const T& last() const { return data[num - 1]; }

	const T& operator[](TSize index) const { return data[index]; }
	T& operator[](TSize index) { return data[index]; }
	T* operator*() const { return data; }
	const T* operator*() { return data; }

	void copy(const T* src, TSize count) 
	{
		for (TSize index = 0; index < count; index++) 
		{
			new (&data[index]) T(src[index]);
		}
	}

	void move(T* src, TSize count)
	{
		for (TSize index = 0; index < count; index++) 
		{
			new (&data[index]) T(std::move(src[index]));
		}
	}

	T* begin() { return data; }
	T* end() { return data ? data + num : nullptr; }
	const T* begin() const { return data; }
	const T* end()   const { return data ? data + num : nullptr; }

private:
	T* data;
	TSize num;
	TSize capacity;

	void destroyRange(TSize from, TSize to) 
	{
		if (data == nullptr) return;
		for (TSize index = from; index < to; index++)
		{
			data[index].~T();
		}
	}

	void copy(T* dst, const T* src, TSize count) 
	{
		for (TSize index = 0; index < count; index++) 
		{
			new (&dst[index]) T(src[index]);
		}
	}

	void move(T* dst, T* src, TSize count) 
	{
		for (TSize index = 0; index < count; index++) 
		{
			new (&dst[index]) T(std::move(src[index]));
		}
	}
};

template<typename T, size_t MAX_COUNT, typename TSize = uint64_t>
struct TFixedArray {

	TFixedArray() : num(0) {}

	~TFixedArray()
	{

	}

	void reset() {
		num = 0;
	}

	template<typename... TArgs>
	T& emplace(TArgs... args) {
		DBG_ASSERT(num < MAX_COUNT, "Exceeded the limit of the fixed array %u", MAX_COUNT);
		T* ptr = new (&data[num++]) T(std::forward(args)...);
		return *ptr;
	}

	void add(const T& element) {
		DBG_ASSERT(num < MAX_COUNT, "Exceeded the limit of the fixed array %u", MAX_COUNT);
		data[num++] = element;
	}
	void remove(TSize index) {
		DBG_ASSERT(index < num, "Index out of bounds");
		for (TSize start = index; start < num - 1; ++start) {
			data[start] = data[start + 1];
		}
		num--;
	}
	T* getData() { return data; }
	const T* getData() const { return data; }
	TSize getNum() const { return num; }
	TSize getCapacity() const { return (TSize)MAX_COUNT; }
	const T& operator[](TSize index) const { return data[index]; }
	T& operator[](TSize index) { return data[index]; }

private:
	T data[MAX_COUNT];
	TSize num;
};
