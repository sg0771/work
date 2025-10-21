#ifndef f_VD2_SYSTEM_MATH_H
#define f_VD2_SYSTEM_MATH_H

#include <math.h>
#include "system_vdtypes.h"

// Constants
namespace nsVDMath {
	static const float	kfPi = 3.1415926535897932384626433832795f;
	static const double	krPi = 3.1415926535897932384626433832795;
	static const float	kfTwoPi = 6.283185307179586476925286766559f;
	static const double	krTwoPi = 6.283185307179586476925286766559;
	static const float	kfLn2 = 0.69314718055994530941723212145818f;
	static const double	krLn2 = 0.69314718055994530941723212145818;
	static const float	kfLn10 = 2.3025850929940456840179914546844f;
	static const double	krLn10 = 2.3025850929940456840179914546844;
	static const float	kfOneOverLn10 = 0.43429448190325182765112891891661f;
	static const double	krOneOverLn10 = 0.43429448190325182765112891891661;
};


sint32 VDRoundToInt32(double x);


inline sint32 VDRoundToIntFast(float x) {
	union {
		float f;
		sint32 i;
	} u = {x + 12582912.0f};		// 2^22+2^23

	return (sint32)u.i - 0x4B400000;
}


#if !defined(VD_CPU_X86) || !defined(VD_COMPILER_MSVC)
	inline sint32 VDFloorToInt(double x) {
		return (sint32)floor(x);
	}

	inline sint64 VDFloorToInt64(double x) {
		return (sint64)floor(x);
	}
#else
	#pragma warning(push)
	#pragma warning(disable: 4035)		// warning C4035: 'VDFloorToInt' : no return value
	inline sint32 VDFloorToInt(double x) {
		sint32 temp;

		__asm {
			fld x
			fist temp
			fild temp
			mov eax, temp
			fsub
			fstp temp
			cmp	temp, 80000001h
			adc eax, -1
		}
	}
	inline sint64 VDFloorToInt64(double x) {
		sint64 temp;
		sint32 temp2;

		__asm {
			fld x
			fld st(0)
			fistp qword ptr temp
			fild qword ptr temp
			mov eax, dword ptr temp
			mov edx, dword ptr temp+4
			fsub
			fstp dword ptr temp2
			cmp	dword ptr temp2, 80000001h
			adc eax, -1
			adc edx, -1
		}
	}
	#pragma warning(pop)
#endif

#if !defined(VD_CPU_X86) || !defined(VD_COMPILER_MSVC)
	inline sint32 VDCeilToInt(double x) {
		return (sint32)ceil(x);
	}

#else
	#pragma warning(push)
	#pragma warning(disable: 4035)		// warning C4035: 'VDCeilToInt' : no return value
	inline sint32 VDCeilToInt(double x) {
		sint32 temp;

		__asm {
			fld x
			fist temp
			fild temp
			mov eax, temp
			fsubr
			fstp temp
			cmp	temp, 80000001h
			sbb eax, -1
		}
	}


	#pragma warning(pop)
#endif


/// Convert a value from [0..1] to [0..255] with clamping.
inline uint8 VDClampedRoundFixedToUint8Fast(float x) {
	union {
		float f;
		sint32 i;
	} u = {x * 255.0f + 12582912.0f};		// 2^22+2^23

	sint32 v = (sint32)u.i - 0x4B400000;

	if ((uint32)v >= 0x100)
		v = ~v >> 31;

	return (uint8)v;
}

#ifdef _M_IX86
	sint64 __stdcall VDFractionScale64(uint64 a, uint32 b, uint32 c, uint32& remainder);
	uint64 __stdcall VDUMulDiv64x32(uint64 a, uint32 b, uint32 c);
#else
	extern "C" sint64 VDFractionScale64(uint64 a, uint64 b, uint64 c, uint32& remainder);
	extern "C" uint64 VDUMulDiv64x32(uint64 a, uint32 b, uint32 c);
#endif


#endif
