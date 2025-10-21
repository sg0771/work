

#ifndef f_VD2_SYSTEM_ATOMIC_H
#define f_VD2_SYSTEM_ATOMIC_H

#include "system_vdtypes.h"

#ifdef VD_COMPILER_MSVC
	// Intrinsics available in VC6.0
	extern "C" long __cdecl _InterlockedDecrement(volatile long *p);
	extern "C" long __cdecl _InterlockedIncrement(volatile long *p);
	extern "C" long __cdecl _InterlockedCompareExchange(volatile long *p, long n, long p_compare);
	extern "C" long __cdecl _InterlockedExchange(volatile long *p, long n);
	extern "C" long __cdecl _InterlockedExchangeAdd(volatile long *p, long n);

	#pragma intrinsic(_InterlockedDecrement)
	#pragma intrinsic(_InterlockedIncrement)
	#pragma intrinsic(_InterlockedCompareExchange)
	#pragma intrinsic(_InterlockedExchange)
	#pragma intrinsic(_InterlockedExchangeAdd)

	// Intrinsics available in VC7.1. Note that the compiler is smart enough to
	// use straight LOCK AND/OR/XOR if the return value is not needed; otherwise
	// it uses a LOCK CMPXCHG loop.
	#if _MSC_VER >= 1310
		extern "C" long __cdecl _InterlockedAnd(volatile long *p, long n);
		extern "C" long __cdecl _InterlockedOr(volatile long *p, long n);
		extern "C" long __cdecl _InterlockedXor(volatile long *p, long n);

		#pragma intrinsic(_InterlockedAnd)
		#pragma intrinsic(_InterlockedOr)
		#pragma intrinsic(_InterlockedXor)
	#endif

	// Intrinsics available with AMD64
	#ifdef VD_CPU_AMD64
		extern "C" void *__cdecl _InterlockedExchangePointer(void *volatile *pp, void *p);
		#pragma intrinsic(_InterlockedExchangePointer)
		extern "C" void *__cdecl _InterlockedCompareExchangePointer(void *volatile *pp, void *p, void *compare);
		#pragma intrinsic(_InterlockedCompareExchangePointer)
	#endif
#endif

inline void *VDAtomicCompareExchangePointer(void *volatile *pp, void *p, void *compare) {
#if defined(VD_COMPILER_MSVC)
	#ifdef _M_AMD64
		return _InterlockedCompareExchangePointer(pp, p, compare);
	#else
		return (void *)(sintptr)_InterlockedCompareExchange((volatile long *)(volatile sintptr *)pp, (long)(sintptr)p, (long)(sintptr)compare);
	#endif
#elif defined(VD_COMPILER_GCC)
	return __sync_val_compare_and_swap(pp, compare, p);
#endif
}

///////////////////////////////////////////////////////////////////////////
/// \class VDAtomicInt
/// \brief Wrapped integer supporting thread-safe atomic operations.
///
/// VDAtomicInt allows integer values shared between threads to be
/// modified with several common operations in a lock-less manner and
/// without the need for explicit barriers. This is particularly useful
/// for thread-safe reference counting.
///
class VDAtomicInt {
protected:
	volatile int n;

public:
	VDAtomicInt() {}
	VDAtomicInt(int v) : n(v) {}

	bool operator!() const { return !n; }
	bool operator!=(volatile int v) const  { return n!=v; }
	bool operator==(volatile int v) const { return n==v; }
	bool operator<=(volatile int v) const { return n<=v; }
	bool operator>=(volatile int v) const { return n>=v; }
	bool operator<(volatile int v) const { return n<v; }
	bool operator>(volatile int v) const { return n>v; }

	///////////////////////////////

	/// Atomically exchanges a value with an integer in memory.
	static inline int staticExchange(volatile int *dst, int v) {
		#if defined(VD_COMPILER_MSVC)
			return (int)_InterlockedExchange((volatile long *)dst, v);
		#elif defined(VD_COMPILER_GCC)
			return __sync_lock_test_and_set((int *)&dst, v);
		#endif
	}

	/// Atomically adds one to an integer in memory.
	static inline void staticIncrement(volatile int *dst) {
		#if defined(VD_COMPILER_MSVC)
			_InterlockedExchangeAdd((volatile long *)dst, 1);
		#elif defined(VD_COMPILER_GCC)
			__sync_fetch_and_add(&dst, 1);
		#endif
	}

	/// Atomically subtracts one from an integer in memory.
	static inline void staticDecrement(volatile int *dst) {
		#if defined(VD_COMPILER_MSVC)
			_InterlockedExchangeAdd((volatile long *)dst, -1);
		#elif defined(VD_COMPILER_GCC)
			__sync_fetch_and_sub(&dst, 1);
		#endif
	}

	/// Atomically subtracts one from an integer in memory and returns
	/// true if the result is zero.
	static inline bool staticDecrementTestZero(volatile int *dst) {
		#if defined(VD_COMPILER_MSVC)
			return 1 == _InterlockedExchangeAdd((volatile long *)dst, -1);
		#elif defined(VD_COMPILER_GCC)
			return 1 == __sync_fetch_and_sub((int *)&dst, 1);
		#endif
	}

	/// Atomically adds a value to an integer in memory and returns the
	/// result.
	static inline int staticAdd(volatile int *dst, int v) {
		#if defined(VD_COMPILER_MSVC)
			return (int)_InterlockedExchangeAdd((volatile long *)dst, v) + v;
		#elif defined(VD_COMPILER_GCC)
			return __sync_fetch_and_add((int *)&dst, v) + v;
		#endif
	}

	/// Atomically adds a value to an integer in memory and returns the
	/// old result (post-add).
	static inline int staticExchangeAdd(volatile int *dst, int v) {
		#if defined(VD_COMPILER_MSVC)
			return _InterlockedExchangeAdd((volatile long *)dst, v);
		#elif defined(VD_COMPILER_GCC)
			return __sync_fetch_and_add((int *)&dst, v);
		#endif
	}

	/// Atomically compares an integer in memory to a compare value and
	/// swaps the memory location with a second value if the compare
	/// succeeds. The return value is the memory value prior to the swap.
	static inline int staticCompareExchange(volatile int *dst, int v, int compare) {
		#if defined(VD_COMPILER_MSVC)
			return _InterlockedCompareExchange((volatile long *)dst, v, compare);
		#elif defined(VD_COMPILER_GCC)
			return __sync_val_compare_and_swap((int *)&dst, compare, v);
		#endif
	}

	///////////////////////////////

	int operator=(int v) { return n = v; }

	int operator++()		{ return staticAdd(&n, 1); }
	int operator--()		{ return staticAdd(&n, -1); }
	int operator++(int)		{ return staticExchangeAdd(&n, 1); }
	int operator--(int)		{ return staticExchangeAdd(&n, -1); }
	int operator+=(int v)	{ return staticAdd(&n, v); }
	int operator-=(int v)	{ return staticAdd(&n, -v); }

#if _MSC_VER >= 1310

	void operator&=(int v)	{ _InterlockedAnd((volatile long *)&n, v); }	///< Atomic bitwise AND.
	void operator|=(int v)	{ _InterlockedOr((volatile long *)&n, v); }		///< Atomic bitwise OR.
	void operator^=(int v)	{ _InterlockedXor((volatile long *)&n, v); }	///< Atomic bitwise XOR.

#elif defined(VD_COMPILER_GCC)

	void operator&=(int v) {
		__sync_fetch_and_and(&n, v);
	}

	void operator|=(int v) {
		__sync_fetch_and_or(&n, v);
	}

	void operator^=(int v) {
		__sync_fetch_and_xor(&n, v);
	}	

#else
	/// Atomic bitwise AND.
	void operator&=(int v) {
		__asm mov eax,v
		__asm mov ecx,this
		__asm lock and dword ptr [ecx],eax
	}

	/// Atomic bitwise OR.
	void operator|=(int v) {
		__asm mov eax,v
		__asm mov ecx,this
		__asm lock or dword ptr [ecx],eax
	}

	/// Atomic bitwise XOR.
	void operator^=(int v) {
		__asm mov eax,v
		__asm mov ecx,this
		__asm lock xor dword ptr [ecx],eax
	}
#endif

	operator int() const {
		return n;
	}

	/// Atomic exchange.
	int xchg(int v) {
		return staticExchange(&n, v);
	}

	/// Compare/exchange (486+).
	int compareExchange(int newValue, int oldValue) {
		return staticCompareExchange(&n, newValue, oldValue);
	}

	// 486 only, but much nicer.  They return the actual result.

	int inc()			{ return operator++(); }				///< Atomic increment.
	int dec()			{ return operator--(); }				///< Atomic decrement.
	int add(int v)		{ return operator+=(v); }				///< Atomic add.

	// These return the result before the operation, which is more inline with
	// what XADD allows us to do.

	int postinc()		{ return operator++(0); }				///< Atomic post-increment.
	int postdec()		{ return operator--(0); }				///< Atomic post-decrement.
	int postadd(int v)	{ return staticExchangeAdd(&n, v); }	///< Atomic post-add.

};

///////////////////////////////////////////////////////////////////////////

class VDAtomicFloat {
protected:
	volatile float n;

public:
	VDAtomicFloat() {}
	VDAtomicFloat(float v) : n(v) {}

	bool operator!=(float v) const  { return n!=v; }
	bool operator==(float v) const { return n==v; }
	bool operator<=(float v) const { return n<=v; }
	bool operator>=(float v) const { return n>=v; }
	bool operator<(float v) const { return n<v; }
	bool operator>(float v) const { return n>v; }

	float operator=(float v) { return n = v; }

	operator float() const {
		return n;
	}

	/// Atomic exchange.
	float xchg(float v) {
		union { int i; float f; } converter = {VDAtomicInt::staticExchange((volatile int *)&n, *(const int *)&v)};

		return converter.f;
	}
};

///////////////////////////////////////////////////////////////////////////

class VDAtomicBool {
protected:
	volatile char n;

public:
	VDAtomicBool() {}
	VDAtomicBool(bool v) : n(v) {}

	bool operator!=(bool v) const { return (n != 0) != v; }
	bool operator==(bool v) const { return (n != 0) == v; }

	bool operator=(bool v) { return n = v; }

	operator bool() const {
		return n != 0;
	}

	/// Atomic exchange.
	bool xchg(bool v) {
		const uint32 mask = ((uint32)0xFF << (int)((size_t)&n & 3));
		const int andval = (int)~mask; 
		const int orval = (int)(mask & 0x01010101);
		volatile int *p = (volatile int *)((uintptr)&n & ~(uintptr)3);

		for(;;) {
			const uint32 prevval = *p;
			const uint32 newval = (prevval & andval) + orval;

			if (prevval == VDAtomicInt::staticCompareExchange(p, newval, prevval))
				return (prevval & mask) != 0;
		}
	}
};

///////////////////////////////////////////////////////////////////////////
/// \class VDAtomicPtr
/// \brief Wrapped pointer supporting thread-safe atomic operations.
///
/// VDAtomicPtr allows a shared pointer to be safely manipulated by
/// multiple threads without locks. Note that atomicity is only guaranteed
/// for the pointer itself, so any operations on the object must be dealt
/// with in other manners, such as an inner lock or other atomic
/// operations. An atomic pointer can serve as a single entry queue.
///
template<typename T>
class VDAtomicPtr {
protected:
	T *volatile ptr;

public:
	VDAtomicPtr() {}
	VDAtomicPtr(T *p) : ptr(p) { }

	operator T*() const { return ptr; }
	T* operator->() const { return ptr; }

	T* operator=(T* p) {
		return ptr = p;
	}

	/// Atomic pointer exchange.
	T *xchg(T* p) {
		#if defined(VD_COMPILER_MSVC)
			#ifdef _M_AMD64
				return ptr == p ? p : (T *)_InterlockedExchangePointer((void *volatile *)&ptr, p);
			#else
				return ptr == p ? p : (T *)_InterlockedExchange((volatile long *)&ptr, (long)p);
			#endif
		#elif defined(VD_COMPILER_GCC)
			return __sync_lock_test_and_set(&ptr, p);
		#endif
	}

	T *compareExchange(T *newValue, T *oldValue) {
		#if defined(VD_COMPILER_MSVC)
			#ifdef _M_AMD64
				return (T *)_InterlockedCompareExchangePointer((void *volatile *)&ptr, (void *)newValue, (void *)oldValue);
			#else
				return (T *)_InterlockedCompareExchange((volatile long *)&ptr, (long)(size_t)newValue, (long)(size_t)oldValue);
			#endif
		#elif defined(VD_COMPILER_GCC)
			return __sync_val_compare_and_swap(&ptr, oldValue, newValue);
		#endif
	}
};

#endif
