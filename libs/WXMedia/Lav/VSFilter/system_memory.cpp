
#include "system.h"
#include <malloc.h>
#include <windows.h>


#ifdef _DEBUG
VDAssertResult VDAssert(const char* exp, const char* file, int line) {
	return kVDAssertIgnore;
}
VDAssertResult VDAssertPtr(const char* exp, const char* file, int line) {
	return kVDAssertIgnore;
}
void VDDebugPrint(const char* format, ...) {

}
#endif

#if defined(VD_COMPILER_MSVC) && defined(_WIN32)
#include <excpt.h>

#define vd_seh_guard_try		__try
#define vd_seh_guard_except		__except(GetExceptionCode() == STATUS_ACCESS_VIOLATION ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
#else
#define vd_seh_guard_try		if (true)
#define vd_seh_guard_except		else
#endif

vdsafedelete_t vdsafedelete;

template class vdspan<char>;
template class vdspan<uint8>;
template class vdspan<uint16>;
template class vdspan<uint32>;
template class vdspan<uint64>;
template class vdspan<sint8>;
template class vdspan<sint16>;
template class vdspan<sint32>;
template class vdspan<sint64>;
template class vdspan<float>;
template class vdspan<double>;
template class vdspan<wchar_t>;

void VDNORETURN vdallocator_base::throw_oom(size_t n, size_t elsize) {
	//size_t nbytes = ~(size_t)0;

	//if (n <= nbytes / elsize)
	//	nbytes = n * elsize;

	//throw MyMemoryError(nbytes);
}



void *VDAlignedMalloc(size_t n, unsigned alignment) {
	return av_malloc(n);
}

void VDAlignedFree(void *p) {
	av_free(p);
}


void VDSwapMemoryScalar(void *p0, void *p1, size_t bytes) {
	uint32 *dst0 = (uint32 *)p0;
	uint32 *dst1 = (uint32 *)p1;

	while(bytes >= 4) {
		uint32 a = *dst0;
		uint32 b = *dst1;

		*dst0++ = b;
		*dst1++ = a;

		bytes -= 4;
	}

	char *dstb0 = (char *)dst0;
	char *dstb1 = (char *)dst1;

	while(bytes--) {
		char a = *dstb0;
		char b = *dstb1;

		*dstb0++ = b;
		*dstb1++ = a;
	}
}

#if defined(VD_CPU_AMD64) || defined(VD_CPU_X86)
	void VDSwapMemorySSE(void *p0, void *p1, size_t bytes) {
		if (((uint32)(size_t)p0 | (uint32)(size_t)p1) & 15)
			return VDSwapMemoryScalar(p0, p1, bytes);

		__m128 *pv0 = (__m128 *)p0;
		__m128 *pv1 = (__m128 *)p1;

		size_t veccount = bytes >> 4;
		if (veccount) {
			do {
				__m128 v0 = *pv0;
				__m128 v1 = *pv1;

				*pv0++ = v1;
				*pv1++ = v0;
			} while(--veccount);
		}

		uint32 left = bytes & 15;
		if (left) {
			uint8 *pb0 = (uint8 *)pv0;
			uint8 *pb1 = (uint8 *)pv1;
			do {
				uint8 b0 = *pb0;
				uint8 b1 = *pb1;

				*pb0++ = b1;
				*pb1++ = b0;
			} while(--left);
		}
	}
#endif

void (__cdecl *VDSwapMemory)(void *p0, void *p1, size_t bytes) = VDSwapMemoryScalar;


namespace {
	uintptr VDGetSystemPageSizeW32() {
		SYSTEM_INFO sysInfo;
		GetSystemInfo(&sysInfo);

		return sysInfo.dwPageSize;
	}

	uintptr VDGetSystemPageSize() {
		static uintptr pageSize = VDGetSystemPageSizeW32();

		return pageSize;
	}
}

bool VDIsValidReadRegion(const void *p0, size_t bytes) {
	if (!bytes)
		return true;

	if (!p0)
		return false;

	uintptr pageSize = VDGetSystemPageSize();
	uintptr p = (uintptr)p0;
	uintptr pLimit = p + (bytes-1);

	vd_seh_guard_try {
		for(;;) {
			*(volatile char *)p;

			if (pLimit - p < pageSize)
				break;

			p += pageSize;
		}
	} vd_seh_guard_except {
		return false;
	}

	return true;
}


void VDMemset8(void *dst, uint8 value, size_t count) {
	if (count) {
		uint8 *dst2 = (uint8 *)dst;

		do {
			*dst2++ = value;
		} while(--count);
	}
}

void VDMemset16(void *dst, uint16 value, size_t count) {
	if (count) {
		uint16 *dst2 = (uint16 *)dst;

		do {
			*dst2++ = value;
		} while(--count);
	}
}


void VDMemset32(void *dst, uint32 value, size_t count) {
	if (count) {
		uint32 *dst2 = (uint32 *)dst;

		do {
			*dst2++ = value;
		} while(--count);
	}
}


void VDMemset128(void *dst, const void *src0, size_t count) {
	if (count) {
		const uint32 *src = (const uint32 *)src0;
		uint32 a0 = src[0];
		uint32 a1 = src[1];
		uint32 a2 = src[2];
		uint32 a3 = src[3];

		uint32 *dst2 = (uint32 *)dst;

		do {
			dst2[0] = a0;
			dst2[1] = a1;
			dst2[2] = a2;
			dst2[3] = a3;
			dst2 += 4;
		} while(--count);
	}
}


void VDMemset8Rect(void *dst, ptrdiff_t pitch, uint8 value, size_t w, size_t h) {
	if (w>0 && h>0) {
		do {
			memset(dst, value, w);
			dst = (char *)dst + pitch;
		} while(--h);
	}
}



#if defined(_WIN32) && defined(VD_CPU_X86) && defined(VD_COMPILER_MSVC)
	extern "C" void __cdecl VDFastMemcpyPartialScalarAligned8(void *dst, const void *src, size_t bytes);
	extern "C" void __cdecl VDFastMemcpyPartialMMX(void *dst, const void *src, size_t bytes);
	extern "C" void __cdecl VDFastMemcpyPartialMMX2(void *dst, const void *src, size_t bytes);

	void VDFastMemcpyPartialScalar(void *dst, const void *src, size_t bytes) {
		if (!(((int)dst | (int)src | bytes) & 7))
			VDFastMemcpyPartialScalarAligned8(dst, src, bytes);
		else
			memcpy(dst, src, bytes);
	}

	void VDFastMemcpyFinishScalar() {
	}

	void __cdecl VDFastMemcpyFinishMMX() {
		_mm_empty();
	}

	void __cdecl VDFastMemcpyFinishMMX2() {
		_mm_empty();
		_mm_sfence();
	}

	void (__cdecl *VDFastMemcpyPartial)(void *dst, const void *src, size_t bytes) = VDFastMemcpyPartialScalar;
	void (__cdecl *VDFastMemcpyFinish)() = VDFastMemcpyFinishScalar;

	void VDFastMemcpyAutodetect() {
		long exts = CPUGetEnabledExtensions();

		if (exts & CPUF_SUPPORTS_SSE) {
			VDFastMemcpyPartial = VDFastMemcpyPartialMMX2;
			VDFastMemcpyFinish	= VDFastMemcpyFinishMMX2;
			VDSwapMemory		= VDSwapMemorySSE;
		} else if (exts & CPUF_SUPPORTS_INTEGER_SSE) {
			VDFastMemcpyPartial = VDFastMemcpyPartialMMX2;
			VDFastMemcpyFinish	= VDFastMemcpyFinishMMX2;
			VDSwapMemory		= VDSwapMemoryScalar;
		} else if (exts & CPUF_SUPPORTS_MMX) {
			VDFastMemcpyPartial = VDFastMemcpyPartialMMX;
			VDFastMemcpyFinish	= VDFastMemcpyFinishMMX;
			VDSwapMemory		= VDSwapMemoryScalar;
		} else {
			VDFastMemcpyPartial = VDFastMemcpyPartialScalar;
			VDFastMemcpyFinish	= VDFastMemcpyFinishScalar;
			VDSwapMemory		= VDSwapMemoryScalar;
		}
	}

#else
	void VDFastMemcpyPartial(void *dst, const void *src, size_t bytes) {
		memcpy(dst, src, bytes);
	}

	void VDFastMemcpyFinish() {
	}

	void VDFastMemcpyAutodetect() {
	}
#endif

void VDMemcpyRect(void *dst, ptrdiff_t dststride, const void *src, ptrdiff_t srcstride, size_t w, size_t h) {
	if (w <= 0 || h <= 0)
		return;

	if (w == srcstride && w == dststride)
		VDFastMemcpyPartial(dst, src, w*h);
	// MPC custom code (begin)
	else if (w == -srcstride && w == -dststride)
		VDFastMemcpyPartial((char *)dst + dststride * (h - 1), (char *)src + srcstride * (h - 1), w*h);
	// MPC custom code (end)
	else {
		char *dst2 = (char *)dst;
		const char *src2 = (const char *)src;

		do {
			VDFastMemcpyPartial(dst2, src2, w);
			dst2 += dststride;
			src2 += srcstride;
		} while(--h);
	}
	VDFastMemcpyFinish();
}

