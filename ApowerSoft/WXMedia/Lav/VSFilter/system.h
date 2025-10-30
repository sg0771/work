
#pragma once

#include <windows.h>
#include <process.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <ctype.h>

#include "system_vdtypes.h"
#include "system_atomic.h"
#include "system_cpuaccel.h"
#include "system_halffloat.h"
#include "system_math.h"
#include "system_memory.h"
#include "system_vdstl.h"
#include "system_vectors.h"

#include <WXBase.h>

#pragma push_macro("_interlockedbittestandset")
#pragma push_macro("_interlockedbittestandreset")
#pragma push_macro("_interlockedbittestandset64")
#pragma push_macro("_interlockedbittestandreset64")

#define _interlockedbittestandset _interlockedbittestandset_vc
#define _interlockedbittestandreset _interlockedbittestandreset_vc
#define _interlockedbittestandset64 _interlockedbittestandset64_vc
#define _interlockedbittestandreset64 _interlockedbittestandreset64_vc

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <emmintrin.h>
#endif

#pragma pop_macro("_interlockedbittestandreset64")
#pragma pop_macro("_interlockedbittestandset64")
#pragma pop_macro("_interlockedbittestandreset")
#pragma pop_macro("_interlockedbittestandset")

#include <stdlib.h>

#pragma warning(push)
#pragma warning(disable: 4284)		// operator-> must return pointer to UDT

template<class T> class vdautoptr {
protected:
	T* ptr;

public:
	explicit vdautoptr(T* p = 0) : ptr(p) {}
	~vdautoptr() { delete ptr; }

	vdautoptr<T>& operator=(T* src) { delete ptr; ptr = src; return *this; }

	operator T* () const { return ptr; }
	T& operator*() const { return *ptr; }
	T* operator->() const { return ptr; }

	T** operator~() {
		if (ptr) {
			delete ptr;
			ptr = NULL;
		}

		return &ptr;
	}

	void from(vdautoptr<T>& src) { delete ptr; ptr = src.ptr; src.ptr = 0; }
	T* get() const { return ptr; }
	T* release() { T* v = ptr; ptr = NULL; return v; }

	void reset() {
		if (ptr) {
			delete ptr;
			ptr = NULL;
		}
	}

	void swap(vdautoptr<T>& other) {
		T* p = other.ptr;
		other.ptr = ptr;
		ptr = p;
	}
};

struct vdsafedelete_t {};
extern vdsafedelete_t vdsafedelete;

template<class T>
inline vdsafedelete_t& operator<<=(vdsafedelete_t& x, T*& p) {
	if (p) {
		delete p;
		p = 0;
	}

	return x;
}

template<class T>
inline vdsafedelete_t& operator,(vdsafedelete_t& x, T*& p) {
	if (p) {
		delete p;
		p = 0;
	}

	return x;
}

template<class T, size_t N>
inline vdsafedelete_t& operator<<=(vdsafedelete_t& x, T* (&p)[N]) {
	for (size_t i = 0; i < N; ++i) {
		if (p[i]) {
			delete p[i];
			p[i] = 0;
		}
	}

	return x;
}

template<class T, size_t N>
inline vdsafedelete_t& operator,(vdsafedelete_t& x, T* (&p)[N]) {
	for (size_t i = 0; i < N; ++i) {
		if (p[i]) {
			delete p[i];
			p[i] = 0;
		}
	}

	return x;
}

#pragma warning(pop)