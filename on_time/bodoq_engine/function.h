#pragma once

template<typename> struct TFunction;

template<typename TReturn, typename...TArgs>
struct TFunction<TReturn(TArgs...)>
{
	TFunction() = default;

	template<typename Fn>
	TFunction(Fn function)
	{
		fnObj = new Fn(function);
		fnCall = [](void* ptr, TArgs... args) -> TReturn { return (*static_cast<Fn*>(ptr))(args...); };
		fnDestroy = [](void* ptr) { delete static_cast<Fn*>(ptr); };
		fnCopy = [](void* ptr) -> void* { return new Fn(*static_cast<Fn*>(ptr)); };
	}
	TFunction(TFunction&& otherMove)
	{
		fnObj = otherMove.fnObj;
		fnCall = otherMove.fnCall;
		fnDestroy = otherMove.fnDestroy;
		fnCopy = otherMove.fnCopy;
		otherMove.fnObj = nullptr;
		otherMove.fnCall = nullptr;
		otherMove.fnDestroy = nullptr;
		otherMove.fnCopy = nullptr;
	}
	TFunction(const TFunction& otherCopy)
	{
		fnObj = nullptr;
		fnCall = otherCopy.fnCall;
		fnDestroy = otherCopy.fnDestroy;
		fnCopy = otherCopy.fnCopy;
		if (fnCopy)
		{
			fnObj = fnCopy(otherCopy.fnObj);
		}
	}
	TFunction& operator=(TFunction&& otherMove)
	{
		if (this != &otherMove)
		{
			reset();
			fnObj = otherMove.fnObj;
			fnCall = otherMove.fnCall;
			fnDestroy = otherMove.fnDestroy;
			fnCopy = otherMove.fnCopy;
			otherMove.fnObj = nullptr;
			otherMove.fnCall = nullptr;
			otherMove.fnDestroy = nullptr;
			otherMove.fnCopy = nullptr;
		}
		return *this;
	}
	TFunction& operator=(const TFunction& otherCopy)
	{
		if (this != &otherCopy)
		{
			reset();
			fnObj = nullptr;
			fnCall = otherCopy.fnCall;
			fnDestroy = otherCopy.fnDestroy;
			fnCopy = otherCopy.fnCopy;
			if (fnCopy)
			{
				fnObj = fnCopy(otherCopy.fnObj);
			}
		}
		return *this;
	}
	~TFunction() { reset(); }
	TReturn operator()(TArgs... args) { return fnCall(fnObj, args...); }
	void reset()
	{
		if (fnDestroy) fnDestroy(fnObj);
		fnObj = nullptr;
		fnCall = nullptr;
		fnDestroy = nullptr;
	}
	operator bool() const { return fnCall != nullptr; }

private:
	void* fnObj = nullptr;
	TReturn (*fnCall)(void*, TArgs...) = nullptr;
	void (*fnDestroy)(void*) = nullptr;
	void* (*fnCopy)(void*) = nullptr;
};
