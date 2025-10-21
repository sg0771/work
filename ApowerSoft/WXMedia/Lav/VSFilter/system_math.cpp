
#include "system.h"
#include <math.h>

sint32 VDRoundToInt32(double x) {
	return (sint32)floor(x + 0.5);
}

#if defined(VD_CPU_X86) && defined(VD_COMPILER_MSVC)
	sint64 __declspec(naked) __stdcall VDFractionScale64(uint64 a, uint32 b, uint32 c, uint32& remainder) {
		__asm {
			push	edi
			push	ebx
			mov		edi, [esp+12+8]			;edi = b
			mov		eax, [esp+4+8]			;eax = a[lo]
			mul		edi						;edx:eax = a[lo]*b
			mov		ecx, eax				;ecx = (a*b)[lo]
			mov		eax, [esp+8+8]			;eax = a[hi]
			mov		ebx, edx				;ebx = (a*b)[mid]
			mul		edi						;edx:eax = a[hi]*b
			add		eax, ebx
			mov		ebx, [esp+16+8]			;ebx = c
			adc		edx, 0
			div		ebx						;eax = (a*b)/c [hi], edx = (a[hi]*b)%c
			mov		edi, eax				;edi = (a[hi]*b)/c
			mov		eax, ecx				;eax = (a*b)[lo]
			mov		ecx, [esp+20+8]
			div		ebx						;eax = (a*b)/c [lo], edx = (a*b)%c
			mov		[ecx], edx
			mov		edx, edi
			pop		ebx
			pop		edi
			ret		20
		}
	}

	uint64 __declspec(naked) __stdcall VDUMulDiv64x32(uint64 a, uint32 b, uint32 c) {
		__asm {
			mov		eax, [esp+4]			;eax = a0
			mul		dword ptr [esp+12]		;edx:eax = a0*b
			mov		dword ptr [esp+4], eax	;tmp = a0*b[0:31]
			mov		ecx, edx				;ecx = a0*b[32:63]
			mov		eax, [esp+8]			;eax = a1
			mul		dword ptr [esp+12]		;edx:eax = a1*b
			add		eax, ecx				;edx:eax += a0*b[32:95]
			adc		edx, 0					;(cont.)
			cmp		edx, [esp+16]			;test if a*b[64:95] >= c; equiv to a*b >= (c<<64)
			jae		invalid					;abort if so (overflow)
			div		dword ptr [esp+16]		;edx,eax = ((a*b)[32:95]/c, (a*b)[32:95]%c)
			mov		ecx, eax
			mov		eax, [esp+4]
			div		dword ptr [esp+16]
			mov		edx, ecx
			ret		16
invalid:
			mov		eax, -1					;return FFFFFFFFFFFFFFFF
			mov		edx, -1
			ret		16
		}
	}
#elif !defined(VD_CPU_AMD64)
	sint64 VDFractionScale64(uint64 a, uint32 b, uint32 c, uint32& remainder) {
		uint32 a0 = (uint32)a;
		uint32 a1 = (uint32)(a >> 32);

		uint64 m0 = (uint64)a0*b;
		uint64 m1 = (uint64)a1*b;

		// collect all multiplier terms
		uint32 s0  = (uint32)m0;
		uint32 s1a = (uint32)(m0 >> 32);
		uint32 s1b = (uint32)m1;
		uint32 s2  = (uint32)(m1 >> 32);

		// form 96-bit intermediate product
		uint32 acc0 = s0;
		uint32 acc1 = s1a + s1b;
		uint32 acc2 = s2 + (acc1 < s1b);

		// check for overflow (or divide by zero)
		if (acc2 >= c)
			return 0xFFFFFFFFFFFFFFFFULL;

		// do divide
		uint64 div1 = ((uint64)acc2 << 32) + acc1;
		uint64 q1 = div1 / c;
		uint64 div0 = ((div1 % c) << 32) + acc0;
		uint32 q0 = (uint32)(div0 / c);

		remainder = (uint32)(div0 % c);

		return (q1 << 32) + q0;
	}

	uint64 VDUMulDiv64x32(uint64 a, uint32 b, uint32 c) {
		uint32 r;

		return VDFractionScale64(a, b, c, r);
	}
#endif


