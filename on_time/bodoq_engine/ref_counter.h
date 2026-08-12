#pragma once

#include "win32.h"

template<typename T>
struct TNonAtomicRefCounter
{
	TNonAtomicRefCounter() = default;

	TNonAtomicRefCounter(T initValue) :
		counter(initValue)
	{
	}
	TNonAtomicRefCounter(TNonAtomicRefCounter<T>&& other) 
	{
		counter = other.counter;
		other.reset();
	}

	TNonAtomicRefCounter<T>& operator=(TNonAtomicRefCounter<T>&& other) noexcept
	{
		counter = other.counter;
		other.reset();
		return *this;
	}
	void reset() { counter = T(0); }
	void inc() { counter++; }
	bool dec() { return --counter == 0; }
	T getCount() const { return counter; }

private:
	TNonAtomicRefCounter(const TNonAtomicRefCounter<T>& other) = delete;
	TNonAtomicRefCounter<T>& operator=(const TNonAtomicRefCounter<T>&) = delete;
	T counter{ 0 };
};

template<typename T>
struct TAtomicRefCounter
{
	TAtomicRefCounter() = default;

	TAtomicRefCounter(T initValue) :
		counter(initValue)
	{
	}

	TAtomicRefCounter(TAtomicRefCounter<T>&& other)
	{
		InterlockedExchange(&counter, other.counter);
		other.reset();
	}

	TAtomicRefCounter<T>& operator=(TAtomicRefCounter<T>&& other) noexcept
	{
		InterlockedExchange(&counter, other.counter);
		other.reset();
		return *this;
	}
	void reset()
	{
		InterlockedExchange(&counter, T(0));
	}
	void inc() 
	{ 
		InterlockedIncrement(&counter);
	}
	bool dec() 
	{ 
		return InterlockedDecrement(&counter) == 0;
	}
	T getCount() const { return counter; }
private:
	TAtomicRefCounter(const TAtomicRefCounter<T>& other) = delete;
	TAtomicRefCounter<T>& operator=(const TAtomicRefCounter<T>&) = delete;
	T counter{ 0 };
};