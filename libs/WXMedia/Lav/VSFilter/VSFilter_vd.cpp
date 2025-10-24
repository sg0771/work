
#include "VSFilterImpl.h"

#include <intrin.h>
#include <emmintrin.h>

#include "kasumi.h"
#include "kasumi_pixmap.h"
#include "Kasumi_pixmaputils.h"
#include "Kasumi_pixmapops.h"
#include "Kasumi_resample.h"
#include "vd.h"

extern bool VDPixmapBlt(const VDPixmap& dst, const VDPixmap& src);


void AvgLines8(BYTE* dst, DWORD h, DWORD pitch)
{
	if (h <= 1) {
		return;
	}

	BYTE* s = dst;
	BYTE* d = dst + (h - 2) * pitch;

	for (; s < d; s += pitch * 2) {
		BYTE* tmp = s;

#ifndef _WIN64
		if (!((DWORD)tmp & 0xf) && !((DWORD)pitch & 0xf)) {
			__asm {
				mov		esi, tmp
				mov		ebx, pitch

				mov		ecx, ebx
				shr		ecx, 4

				AvgLines8_sse2_loop:
				movdqa	xmm0, [esi]
					pavgb	xmm0, [esi + ebx * 2]
					movdqa[esi + ebx], xmm0
					add		esi, 16

					dec		ecx
					jnz		AvgLines8_sse2_loop

					mov		tmp, esi
			}

			for (ptrdiff_t i = pitch & 7; i--; tmp++) {
				tmp[pitch] = (tmp[0] + tmp[pitch << 1] + 1) >> 1;
			}
		}
		else {
			__asm {
				mov		esi, tmp
				mov		ebx, pitch

				mov		ecx, ebx
				shr		ecx, 3

				pxor	mm7, mm7
				AvgLines8_mmx_loop :
				movq	mm0, [esi]
					movq	mm1, mm0

					punpcklbw	mm0, mm7
					punpckhbw	mm1, mm7

					movq	mm2, [esi + ebx * 2]
					movq	mm3, mm2

					punpcklbw	mm2, mm7
					punpckhbw	mm3, mm7

					paddw	mm0, mm2
					psrlw	mm0, 1

					paddw	mm1, mm3
					psrlw	mm1, 1

					packuswb	mm0, mm1

					movq[esi + ebx], mm0

					lea		esi, [esi + 8]

					dec		ecx
					jnz		AvgLines8_mmx_loop

					mov		tmp, esi
			}

			for (ptrdiff_t i = pitch & 7; i--; tmp++) {
				tmp[pitch] = (tmp[0] + tmp[pitch << 1] + 1) >> 1;
			}
		}
#else
		{
			for (ptrdiff_t i = pitch; i--; tmp++) {
				tmp[pitch] = (tmp[0] + tmp[pitch << 1] + 1) >> 1;
			}
		}
#endif
	}

	if (!(h & 1) && h >= 2) {
		dst += (h - 2) * pitch;
		memcpy(dst + pitch, dst, pitch);
	}

#ifndef _WIN64
	__asm emms;
#endif
}

void AvgLines555(BYTE* dst, DWORD h, DWORD pitch)
{
	if (h <= 1) {
		return;
	}

	unsigned __int64 __0x03e003e003e003e0 = 0x03e003e003e003e0;
	unsigned __int64 __0x001f001f001f001f = 0x001f001f001f001f;

	BYTE* s = dst;
	BYTE* d = dst + (h - 2) * pitch;

	for (; s < d; s += pitch * 2) {
		BYTE* tmp = s;

#ifndef _WIN64
		__asm {
			mov		esi, tmp
			mov		ebx, pitch

			mov		ecx, ebx
			shr		ecx, 3

			movq	mm6, __0x03e003e003e003e0
			movq	mm7, __0x001f001f001f001f

			AvgLines555_loop :
			movq	mm0, [esi]
				movq	mm1, mm0
				movq	mm2, mm0

				psrlw	mm0, 10				// red1 bits: mm0 = 001f001f001f001f
				pand	mm1, mm6			// green1 bits: mm1 = 03e003e003e003e0
				pand	mm2, mm7			// blue1 bits: mm2 = 001f001f001f001f

				movq	mm3, [esi + ebx * 2]
				movq	mm4, mm3
				movq	mm5, mm3

				psrlw	mm3, 10				// red2 bits: mm3 = 001f001f001f001f
				pand	mm4, mm6			// green2 bits: mm4 = 03e003e003e003e0
				pand	mm5, mm7			// blue2 bits: mm5 = 001f001f001f001f

				paddw	mm0, mm3
				psrlw	mm0, 1				// (red1+red2)/2
				psllw	mm0, 10				// red bits at 7c007c007c007c00

				paddw	mm1, mm4
				psrlw	mm1, 1				// (green1+green2)/2
				pand	mm1, mm6			// green bits at 03e003e003e003e0

				paddw	mm2, mm5
				psrlw	mm2, 1				// (blue1+blue2)/2
				// blue bits at 001f001f001f001f (no need to pand, lower bits were discareded)

				por		mm0, mm1
				por		mm0, mm2

				movq[esi + ebx], mm0

				lea		esi, [esi + 8]

				dec		ecx
				jnz		AvgLines555_loop

				mov		tmp, esi
		}
#endif

		for (ptrdiff_t i = (pitch & 7) >> 1; i--; tmp++) {
			tmp[pitch] =
				((((*tmp & 0x7c00) + (tmp[pitch << 1] & 0x7c00)) >> 1) & 0x7c00) |
				((((*tmp & 0x03e0) + (tmp[pitch << 1] & 0x03e0)) >> 1) & 0x03e0) |
				((((*tmp & 0x001f) + (tmp[pitch << 1] & 0x001f)) >> 1) & 0x001f);
		}
	}

	if (!(h & 1) && h >= 2) {
		dst += (h - 2) * pitch;
		memcpy(dst + pitch, dst, pitch);
	}

#ifndef _WIN64
	__asm emms;
#endif
}

void AvgLines565(BYTE* dst, DWORD h, DWORD pitch)
{
	if (h <= 1) {
		return;
	}

	unsigned __int64 __0x07e007e007e007e0 = 0x07e007e007e007e0;
	unsigned __int64 __0x001f001f001f001f = 0x001f001f001f001f;

	BYTE* s = dst;
	BYTE* d = dst + (h - 2) * pitch;

	for (; s < d; s += pitch * 2) {
		WORD* tmp = (WORD*)s;

#ifndef _WIN64
		__asm {
			mov		esi, tmp
			mov		ebx, pitch

			mov		ecx, ebx
			shr		ecx, 3

			movq	mm6, __0x07e007e007e007e0
			movq	mm7, __0x001f001f001f001f

			AvgLines565_loop :
			movq	mm0, [esi]
				movq	mm1, mm0
				movq	mm2, mm0

				psrlw	mm0, 11				// red1 bits: mm0 = 001f001f001f001f
				pand	mm1, mm6			// green1 bits: mm1 = 07e007e007e007e0
				pand	mm2, mm7			// blue1 bits: mm2 = 001f001f001f001f

				movq	mm3, [esi + ebx * 2]
				movq	mm4, mm3
				movq	mm5, mm3

				psrlw	mm3, 11				// red2 bits: mm3 = 001f001f001f001f
				pand	mm4, mm6			// green2 bits: mm4 = 07e007e007e007e0
				pand	mm5, mm7			// blue2 bits: mm5 = 001f001f001f001f

				paddw	mm0, mm3
				psrlw	mm0, 1				// (red1+red2)/2
				psllw	mm0, 11				// red bits at f800f800f800f800

				paddw	mm1, mm4
				psrlw	mm1, 1				// (green1+green2)/2
				pand	mm1, mm6			// green bits at 03e003e003e003e0

				paddw	mm2, mm5
				psrlw	mm2, 1				// (blue1+blue2)/2
				// blue bits at 001f001f001f001f (no need to pand, lower bits were discareded)

				por		mm0, mm1
				por		mm0, mm2

				movq[esi + ebx], mm0

				lea		esi, [esi + 8]

				dec		ecx
				jnz		AvgLines565_loop

				mov		tmp, esi
		}
#else
		for (ptrdiff_t wd = (pitch >> 3); wd--; tmp++) {
			tmp[0] =
				((((*tmp & 0xf800) + (tmp[pitch << 1] & 0xf800)) >> 1) & 0xf800) |
				((((*tmp & 0x07e0) + (tmp[pitch << 1] & 0x07e0)) >> 1) & 0x07e0) |
				((((*tmp & 0x001f) + (tmp[pitch << 1] & 0x001f)) >> 1) & 0x001f);
		}
#endif

		for (ptrdiff_t i = (pitch & 7) >> 1; i--; tmp++) {
			tmp[pitch] =
				((((*tmp & 0xf800) + (tmp[pitch << 1] & 0xf800)) >> 1) & 0xf800) |
				((((*tmp & 0x07e0) + (tmp[pitch << 1] & 0x07e0)) >> 1) & 0x07e0) |
				((((*tmp & 0x001f) + (tmp[pitch << 1] & 0x001f)) >> 1) & 0x001f);
		}
	}

	if (!(h & 1) && h >= 2) {
		dst += (h - 2) * pitch;
		memcpy(dst + pitch, dst, pitch);
	}

#ifndef _WIN64
	__asm emms;
#endif
}


#include "vd.h"

#pragma warning(disable : 4799) // no emms... blahblahblah

#ifndef _WIN64
void __declspec(naked) yuvtoyuy2row_MMX(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width)
{
	__asm {
		push	ebp
		push	edi
		push	esi
		push	ebx

		mov		edi, [esp + 20] // dst
		mov		ebp, [esp + 24] // srcy
		mov		ebx, [esp + 28] // srcu
		mov		esi, [esp + 32] // srcv
		mov		ecx, [esp + 36] // width

		shr		ecx, 3

		yuvtoyuy2row_loop:

		movd		mm0, [ebx]
			punpcklbw	mm0, [esi]

			movq		mm1, [ebp]
			movq		mm2, mm1
			punpcklbw	mm1, mm0
			punpckhbw	mm2, mm0

			movq[edi], mm1
			movq[edi + 8], mm2

			add		ebp, 8
			add		ebx, 4
			add		esi, 4
			add		edi, 16

			dec		ecx
			jnz		yuvtoyuy2row_loop

			pop		ebx
			pop		esi
			pop		edi
			pop		ebp
			ret
	};
}


static const __int64 mask1 = 0x7f7f7f7f7f7f7f7fi64;
void __declspec(naked) yuvtoyuy2row_avg_MMX(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width, DWORD pitchuv)
{
	__asm {
		push	ebp
		push	edi
		push	esi
		push	ebx

		movq	mm7, mask1

		mov		edi, [esp + 20] // dst
		mov		ebp, [esp + 24] // srcy
		mov		ebx, [esp + 28] // srcu
		mov		esi, [esp + 32] // srcv
		mov		ecx, [esp + 36] // width
		mov		eax, [esp + 40] // pitchuv

		shr		ecx, 3

		yuvtoyuy2row_avg_loop:

		movd		mm0, [ebx]
			punpcklbw	mm0, [esi]
			movq		mm1, mm0

			movd		mm2, [ebx + eax]
			punpcklbw	mm2, [esi + eax]
			movq		mm3, mm2

			// (x+y)>>1 == (x&y)+((x^y)>>1)

			pand		mm0, mm2
			pxor		mm1, mm3
			psrlq		mm1, 1
			pand		mm1, mm7
			paddb		mm0, mm1

			movq		mm1, [ebp]
			movq		mm2, mm1
			punpcklbw	mm1, mm0
			punpckhbw	mm2, mm0

			movq[edi], mm1
			movq[edi + 8], mm2

			add		ebp, 8
			add		ebx, 4
			add		esi, 4
			add		edi, 16

			dec		ecx
			jnz		yuvtoyuy2row_avg_loop

			pop		ebx
			pop		esi
			pop		edi
			pop		ebp
			ret
	};
}

void __declspec(naked) yv12_yuy2_row_sse2() {
	__asm {
		// ebx - Y
		// edx - U
		// esi - V
		// edi - dest
		// ecx - halfwidth
		xor eax, eax

		one :
		movdqa		xmm0, [ebx + eax * 2]			// YYYYYYYY
			movdqa		xmm1, [ebx + eax * 2 + 16]	// YYYYYYYY

			movdqa		xmm2, [edx + eax]			// UUUUUUUU
			movdqa		xmm3, [esi + eax]			// VVVVVVVV

			movdqa		xmm4, xmm2
			movdqa		xmm5, xmm0
			movdqa		xmm6, xmm1
			punpcklbw	xmm2, xmm3					// VUVUVUVU
			punpckhbw	xmm4, xmm3					// VUVUVUVU

			punpcklbw	xmm0, xmm2					// VYUYVYUY
			punpcklbw	xmm1, xmm4
			punpckhbw	xmm5, xmm2
			punpckhbw	xmm6, xmm4

			movntdq[edi + eax * 4], xmm0
			movntdq[edi + eax * 4 + 16], xmm5
			movntdq[edi + eax * 4 + 32], xmm1
			movntdq[edi + eax * 4 + 48], xmm6

			add		eax, 16
			cmp		eax, ecx

			jb		one

			ret
	};
}

void __declspec(naked) yv12_yuy2_row_sse2_linear() {
	__asm {
		// ebx - Y
		// edx - U
		// esi - V
		// edi - dest
		// ecx - width
		// ebp - uv_stride
		xor eax, eax

		one :
		movdqa		xmm0, [ebx + eax * 2]			// YYYYYYYY
			movdqa		xmm1, [ebx + eax * 2 + 16]	// YYYYYYYY

			movdqa		xmm2, [edx]
			movdqa		xmm3, [esi]
			pavgb		xmm2, [edx + ebp]		// UUUUUUUU
			pavgb		xmm3, [esi + ebp]		// VVVVVVVV

			movdqa		xmm4, xmm2
			movdqa		xmm5, xmm0
			movdqa		xmm6, xmm1
			punpcklbw	xmm2, xmm3			// VUVUVUVU
			punpckhbw	xmm4, xmm3			// VUVUVUVU

			punpcklbw	xmm0, xmm2			// VYUYVYUY
			punpcklbw	xmm1, xmm4
			punpckhbw	xmm5, xmm2
			punpckhbw	xmm6, xmm4

			movntdq[edi + eax * 4], xmm0
			movntdq[edi + eax * 4 + 16], xmm5
			movntdq[edi + eax * 4 + 32], xmm1
			movntdq[edi + eax * 4 + 48], xmm6

			add		eax, 16
			add		edx, 16
			add		esi, 16
			cmp		eax, ecx

			jb		one

			ret
	};
}

void __declspec(naked) yv12_yuy2_row_sse2_linear_interlaced() {
	__asm {
		// ebx - Y
		// edx - U
		// esi - V
		// edi - dest
		// ecx - width
		// ebp - uv_stride
		xor eax, eax

		one :
		movdqa		xmm0, [ebx + eax * 2]			// YYYYYYYY
			movdqa		xmm1, [ebx + eax * 2 + 16]	// YYYYYYYY

			movdqa		xmm2, [edx]
			movdqa		xmm3, [esi]
			pavgb		xmm2, [edx + ebp * 2]			// UUUUUUUU
			pavgb		xmm3, [esi + ebp * 2]			// VVVVVVVV

			movdqa		xmm4, xmm2
			movdqa		xmm5, xmm0
			movdqa		xmm6, xmm1
			punpcklbw	xmm2, xmm3				// VUVUVUVU
			punpckhbw	xmm4, xmm3				// VUVUVUVU

			punpcklbw	xmm0, xmm2				// VYUYVYUY
			punpcklbw	xmm1, xmm4
			punpckhbw	xmm5, xmm2
			punpckhbw	xmm6, xmm4

			movntdq[edi + eax * 4], xmm0
			movntdq[edi + eax * 4 + 16], xmm5
			movntdq[edi + eax * 4 + 32], xmm1
			movntdq[edi + eax * 4 + 48], xmm6

			add		eax, 16
			add		edx, 16
			add		esi, 16
			cmp		eax, ecx

			jb		one

			ret
	};
}

void __declspec(naked) yv12_yuy2_sse2(const BYTE* Y, const BYTE* U, const BYTE* V,
	int halfstride, unsigned halfwidth, unsigned height,
	BYTE* YUY2, int d_stride)
{
	__asm {
		push	ebx
		push	esi
		push	edi
		push	ebp

		mov		ebx, [esp + 20] // Y
		mov		edx, [esp + 24] // U
		mov		esi, [esp + 28] // V
		mov		edi, [esp + 44] // D
		mov		ebp, [esp + 32] // uv_stride
		mov		ecx, [esp + 36] // uv_width

		mov		eax, ecx
		add		eax, 15
		and eax, 0xfffffff0
		sub[esp + 32], eax

		cmp		dword ptr[esp + 40], 2
		jbe		last2

		row :
		sub		dword ptr[esp + 40], 2
			call	yv12_yuy2_row_sse2

			lea		ebx, [ebx + ebp * 2]
			add		edi, [esp + 48]

			call	yv12_yuy2_row_sse2_linear

			add		edx, [esp + 32]
			add		esi, [esp + 32]

			lea		ebx, [ebx + ebp * 2]
			add		edi, [esp + 48]

			cmp		dword ptr[esp + 40], 2
			ja		row

			last2 :
		call	yv12_yuy2_row_sse2

			dec		dword ptr[esp + 40]
			jz		done

			lea		ebx, [ebx + ebp * 2]
			add		edi, [esp + 48]
			call	yv12_yuy2_row_sse2
			done :

		pop		ebp
			pop		edi
			pop		esi
			pop		ebx

			ret
	};
}

void __declspec(naked) yv12_yuy2_sse2_interlaced(const BYTE* Y, const BYTE* U, const BYTE* V,
	int halfstride, unsigned halfwidth, unsigned height,
	BYTE* YUY2, int d_stride)
{
	__asm {
		push	ebx
		push	esi
		push	edi
		push	ebp

		mov		ebx, [esp + 20] // Y
		mov		edx, [esp + 24] // U
		mov		esi, [esp + 28] // V
		mov		edi, [esp + 44] // D
		mov		ebp, [esp + 32] // uv_stride
		mov		ecx, [esp + 36] // uv_width

		mov		eax, ecx
		add		eax, 15
		and eax, 0xfffffff0
		sub[esp + 32], eax

		cmp		dword ptr[esp + 40], 4
		jbe		last4

		row :
		sub		dword ptr[esp + 40], 4
			call	yv12_yuy2_row_sse2						// first row, first field

			lea		ebx, [ebx + ebp * 2]
			add		edi, [esp + 48]

			add		edx, ebp
			add		esi, ebp

			call	yv12_yuy2_row_sse2						// first row, second field

			lea		ebx, [ebx + ebp * 2]
			add		edi, [esp + 48]

			sub		edx, ebp
			sub		esi, ebp

			call	yv12_yuy2_row_sse2_linear_interlaced	// second row, first field

			add		edx, [esp + 32]
			add		esi, [esp + 32]

			lea		ebx, [ebx + ebp * 2]
			add		edi, [esp + 48]

			call	yv12_yuy2_row_sse2_linear_interlaced	// second row, second field

			add		edx, [esp + 32]
			add		esi, [esp + 32]

			lea		ebx, [ebx + ebp * 2]
			add		edi, [esp + 48]

			cmp		dword ptr[esp + 40], 4
			ja		row

			last4 :
		call	yv12_yuy2_row_sse2

			lea		ebx, [ebx + ebp * 2]
			add		edi, [esp + 48]

			add		edx, ebp
			add		esi, ebp

			call	yv12_yuy2_row_sse2

			lea		ebx, [ebx + ebp * 2]
			add		edi, [esp + 48]

			sub		edx, ebp
			sub		esi, ebp

			call	yv12_yuy2_row_sse2

			lea		ebx, [ebx + ebp * 2]
			add		edi, [esp + 48]

			add		edx, ebp
			add		esi, ebp

			call	yv12_yuy2_row_sse2

			pop		ebp
			pop		edi
			pop		esi
			pop		ebx

			ret
	};
}
#endif


void Scale2x_YV(int w, int h, BYTE* d, int dpitch, BYTE* s, int spitch)
{
	BYTE* s1;
	BYTE* s2;
	BYTE* d1;

	for (s1 = s, s2 = s + h * spitch, d1 = d; s1 < s2; d1 += dpitch) { // TODO: replace this mess with mmx code
		BYTE* stmp = s1 + spitch;
		BYTE* dtmp = d1 + dpitch;

		for (BYTE* s3 = s1 + (w - 1); s1 < s3; s1 += 1, d1 += 2) {
			d1[0] = s1[0];
			d1[1] = (s1[0] + s1[1]) >> 1;
		}

		d1[0] = d1[1] = s1[0];

		s1 += 1;
		d1 += 2;

		s1 = stmp;
		d1 = dtmp;
	}

	AvgLines8(d, h * 2, dpitch);
}

void Scale2x_YUY2_SSE2(BYTE* s1, BYTE* d1, int w)
{
	const uint64_t __0xffffffff00000000 = 0xffffffff00000000;
	const uint64_t __0x00000000ffffffff = 0x00000000ffffffff;
	const uint64_t __0x00ff00ff00ff00ff = 0x00ff00ff00ff00ff;

	const __m128i mm4 = _mm_loadl_epi64((const __m128i*) & __0x00ff00ff00ff00ff);	//movq	mm0, __0x00ff00ff00ff00ff
	const __m128i mm5 = _mm_loadl_epi64((const __m128i*) & __0x00000000ffffffff);	//movq	mm0, __0x00000000ffffffff
	const __m128i mm6 = _mm_loadl_epi64((const __m128i*) & __0xffffffff00000000);	//movq	mm0, __0xffffffff00000000
	for (BYTE* s3 = s1 + ((w >> 1) - 1) * 4; s1 < s3; s1 += 4, d1 += 8) {
		__m128i mm0 = _mm_loadl_epi64((const __m128i*)(s1));	//movq	mm0, [esi]
		__m128i mm2 = _mm_move_epi64(mm0);						//movq	mm2, mm0
		mm0 = _mm_and_si128(mm0, mm4);							//pand	mm0, mm4	// mm0 = 00y400y300y200y1
		mm2 = _mm_srli_epi16(mm2, 8);							//psrlw	mm2, 8		// mm2 = 00u200v200u100v1
		__m128i mm1 = _mm_move_epi64(mm0);						//movq	mm1, mm0
		mm0 = _mm_and_si128(mm0, mm5);							//pand	mm0, mm5	// mm0 = 0000000000y200y1
		mm1 = _mm_slli_epi64(mm1, 16);							//psllq	mm1, 16
		mm1 = _mm_and_si128(mm1, mm6);							//pand	mm1, mm6	// mm1 = 00y300y200000000
		mm1 = _mm_or_si128(mm1, mm0);							//por	mm1, mm0	// mm1 = 00y300y200y200y1
		mm0 = _mm_unpacklo_epi8(mm0, mm0);						//punpcklwd mm0, mm0	// mm0 = 00y200y200y100y1
		mm0 = _mm_adds_epi16(mm0, mm1);							//paddw	mm0, mm1
		mm0 = _mm_srli_epi16(mm0, 1);							//psrlw	mm0, 1		// mm0 = (mm0 + mm1) / 2
		mm1 = _mm_move_epi64(mm2);								//movq	mm1, mm2
		mm1 = _mm_unpacklo_epi32(mm1, mm1);						//punpckldq	mm1, mm1 // mm1 = 00u100v100u100v1
		mm1 = _mm_adds_epi16(mm1, mm2);							//paddw	mm1, mm2
		mm1 = _mm_srli_epi16(mm1, 1);							//psrlw	mm1, 1		// mm1 = (mm1 + mm2) / 2
		mm1 = _mm_slli_epi64(mm1, 8);							//psllw	mm1, 8
		mm1 = _mm_or_si128(mm0, mm1);							//por		mm0, mm1	// mm0 = (v1+v2)/2|(y2+y3)/2|(u1+u2)/2|y2|v1|(y1+y2)/2|u1|y1
		_mm_storel_epi64((__m128i*)(d1), mm0);					//movq	[edi], mm0
	}

	*d1++ = s1[0];
	*d1++ = s1[1];
	*d1++ = (s1[0] + s1[2]) >> 1;
	*d1++ = s1[3];

	*d1++ = s1[2];
	*d1++ = s1[1];
	*d1++ = s1[2];
	*d1++ = s1[3];

	s1 += 4;
}

/*
void Scale2x_YUY2_c( BYTE* s1, BYTE* d1, int w )
{
	for (BYTE* s3 = s1 + ((w>>1)-1)*4; s1 < s3; s1 += 4, d1 += 8) {
		d1[0] = s1[0];
		d1[1] = s1[1];
		d1[2] = (s1[0]+s1[2])>>1;
		d1[3] = s1[3];

		d1[4] = s1[2];
		d1[5] = (s1[1]+s1[5])>>1;
		d1[6] = (s1[2]+s1[4])>>1;
		d1[7] = (s1[3]+s1[7])>>1;
	}

	*d1++ = s1[0];
	*d1++ = s1[1];
	*d1++ =(s1[0]+s1[2])>>1;
	*d1++ = s1[3];

	*d1++ = s1[2];
	*d1++ = s1[1];
	*d1++ = s1[2];
	*d1++ = s1[3];

	s1 += 4;
}
*/

void Scale2x_YUY2(int w, int h, BYTE* d, int dpitch, BYTE* s, int spitch)
{
	BYTE* s1;
	BYTE* s2;
	BYTE* d1;

	for (s1 = s, s2 = s + h * spitch, d1 = d; s1 < s2; d1 += dpitch) {
		// row0, 4 pixels: y1|u1|y2|v1|y3|u2|y4|v2
		// ->
		// row0, 8 pixels: y1|u1|(y1+y2)/2|v1|y2|(u1+u2)/2|(y2+y3)/2|(v1+v2)/2

		Scale2x_YUY2_SSE2(s1, d1, w);

		s1 += spitch;
		d1 += dpitch;
	}

	AvgLines8(d, h * 2, dpitch);
}

void Scale2x_RGB555(int w, int h, BYTE* d, int dpitch, BYTE* s, int spitch)
{
	BYTE* s1;
	BYTE* s2;
	BYTE* d1;

	for (s1 = s, s2 = s + h * spitch, d1 = d; s1 < s2; d1 += dpitch) { // TODO: replace this mess with mmx code
		BYTE* stmp = s1 + spitch;
		BYTE* dtmp = d1 + dpitch;

		for (BYTE* s3 = s1 + (w - 1) * 2; s1 < s3; s1 += 2, d1 += 4) {
			*((WORD*)d1) = *((WORD*)s1);
			*((WORD*)d1 + 1) =
				((((*((WORD*)s1) & 0x7c00) + (*((WORD*)s1 + 1) & 0x7c00)) >> 1) & 0x7c00) |
				((((*((WORD*)s1) & 0x03e0) + (*((WORD*)s1 + 1) & 0x03e0)) >> 1) & 0x03e0) |
				((((*((WORD*)s1) & 0x001f) + (*((WORD*)s1 + 1) & 0x001f)) >> 1) & 0x001f);
		}

		*((WORD*)d1) = *((WORD*)s1);
		*((WORD*)d1 + 1) = *((WORD*)s1);

		s1 += 2;
		d1 += 4;

		s1 = stmp;
		d1 = dtmp;
	}

	AvgLines555(d, h * 2, dpitch);
}

void Scale2x_RGB565(int w, int h, BYTE* d, int dpitch, BYTE* s, int spitch)
{
	BYTE* s1;
	BYTE* s2;
	BYTE* d1;

	for (s1 = s, s2 = s + h * spitch, d1 = d; s1 < s2; d1 += dpitch) { // TODO: replace this mess with mmx code
		BYTE* stmp = s1 + spitch;
		BYTE* dtmp = d1 + dpitch;

		for (BYTE* s3 = s1 + (w - 1) * 2; s1 < s3; s1 += 2, d1 += 4) {
			*((WORD*)d1) = *((WORD*)s1);
			*((WORD*)d1 + 1) =
				((((*((WORD*)s1) & 0xf800) + (*((WORD*)s1 + 1) & 0xf800)) >> 1) & 0xf800) |
				((((*((WORD*)s1) & 0x07e0) + (*((WORD*)s1 + 1) & 0x07e0)) >> 1) & 0x07e0) |
				((((*((WORD*)s1) & 0x001f) + (*((WORD*)s1 + 1) & 0x001f)) >> 1) & 0x001f);
		}

		*((WORD*)d1) = *((WORD*)s1);
		*((WORD*)d1 + 1) = *((WORD*)s1);

		s1 += 2;
		d1 += 4;

		s1 = stmp;
		d1 = dtmp;
	}

	AvgLines565(d, h * 2, dpitch);
}

void Scale2x_RGB24(int w, int h, BYTE* d, int dpitch, BYTE* s, int spitch)
{
	BYTE* s1;
	BYTE* s2;
	BYTE* d1;

	for (s1 = s, s2 = s + h * spitch, d1 = d; s1 < s2; d1 += dpitch) { // TODO: replace this mess with mmx code
		BYTE* stmp = s1 + spitch;
		BYTE* dtmp = d1 + dpitch;

		for (BYTE* s3 = s1 + (w - 1) * 3; s1 < s3; s1 += 3, d1 += 6) {
			d1[0] = s1[0];
			d1[1] = s1[1];
			d1[2] = s1[2];
			d1[3] = (s1[0] + s1[3]) >> 1;
			d1[4] = (s1[1] + s1[4]) >> 1;
			d1[5] = (s1[2] + s1[5]) >> 1;
		}

		d1[0] = d1[3] = s1[0];
		d1[1] = d1[4] = s1[1];
		d1[2] = d1[5] = s1[2];

		s1 += 3;
		d1 += 6;

		s1 = stmp;
		d1 = dtmp;
	}

	AvgLines8(d, h * 2, dpitch);
}

void Scale2x_XRGB32_SSE2(BYTE* s1, BYTE* d1, int w)
{
	const __m128i mm_zero = _mm_setzero_si128();				//pxor	mm0, mm0
	for (BYTE* s3 = s1 + (w - 1) * 4; s1 < s3; s1 += 4, d1 += 8) {

		__m128i mm1 = _mm_loadl_epi64((const __m128i*)(s1));	//movq	mm1, [esi]
		__m128i mm2 = _mm_move_epi64(mm1);						//movq	mm2, mm1

		mm1 = _mm_unpacklo_epi8(mm1, mm_zero);					//punpcklbw mm1, mm0	// mm1 = 00xx00r100g100b1
		mm2 = _mm_unpacklo_epi8(mm2, mm_zero);					//punpckhbw mm2, mm0	// mm2 = 00xx00r200g200b2

		mm2 = _mm_adds_epi16(mm2, mm1);							//paddw	mm2, mm1
		mm2 = _mm_srli_epi16(mm2, 1);							//psrlw	mm2, 1		// mm2 = (mm1 + mm2) / 2

		mm1 = _mm_packus_epi16(mm1, mm2);						//packuswb	mm1, mm2

		_mm_storel_epi64((__m128i*)(d1), mm1);					//movq	[edi], mm0
	}

	*((DWORD*)d1) = *((DWORD*)s1);
	*((DWORD*)d1 + 1) = *((DWORD*)s1);

	s1 += 4;
	d1 += 8;
}

/*
void Scale2x_XRGB32_c( BYTE* s1, BYTE* d1, int w )
{
	for (BYTE* s3 = s1 + (w-1)*4; s1 < s3; s1 += 3, d1 += 6) {
		d1[0] = s1[0];
		d1[1] = s1[1];
		d1[2] = s1[2];
		d1[3] = s1[3];

		d1[4] = (s1[0]+s1[4])>>1;
		d1[5] = (s1[1]+s1[5])>>1;
		d1[6] = (s1[2]+s1[6])>>1;
		d1[7] = (s1[4]+s1[7])>>1;
	}

	*((DWORD*)d1) = *((DWORD*)s1);
	*((DWORD*)d1+1) = *((DWORD*)s1);

	s1 += 4;
	d1 += 8;
}
*/

void Scale2x_XRGB32(int w, int h, BYTE* d, int dpitch, BYTE* s, int spitch)
{
	BYTE* s1;
	BYTE* s2;
	BYTE* d1;

	for (s1 = s, s2 = s + h * spitch, d1 = d; s1 < s2; d1 += dpitch) {
		Scale2x_XRGB32_SSE2(s1, d1, w);

		s1 += spitch;
		d1 += dpitch;
	}

	AvgLines8(d, h * 2, dpitch);
}

/* ResX2 */
void Scale2x(const GUID& subtype, BYTE* d, int dpitch, BYTE* s, int spitch, int w, int h)
{
	if (subtype == MEDIASUBTYPE_YV12 || subtype == MEDIASUBTYPE_I420 || subtype == MEDIASUBTYPE_IYUV) {
		Scale2x_YV(w, h, d, dpitch, s, spitch);
	}
	else if (subtype == MEDIASUBTYPE_YUY2) {
		Scale2x_YUY2(w, h, d, dpitch, s, spitch);
	}
	else if (subtype == MEDIASUBTYPE_RGB555) {
		Scale2x_RGB555(w, h, d, dpitch, s, spitch);
	}
	else if (subtype == MEDIASUBTYPE_RGB565) {
		Scale2x_RGB565(w, h, d, dpitch, s, spitch);
	}
	else if (subtype == MEDIASUBTYPE_RGB24) {
		Scale2x_RGB24(w, h, d, dpitch, s, spitch);
	}
	else if (subtype == MEDIASUBTYPE_RGB32 || subtype == MEDIASUBTYPE_ARGB32) {
		Scale2x_XRGB32(w, h, d, dpitch, s, spitch);
	}
}




void VDCPUTest() {
	SYSTEM_INFO si;

	long lEnableFlags = CPUCheckForExtensions();

	GetSystemInfo(&si);

	if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
		if (si.wProcessorLevel < 4)
			lEnableFlags &= ~CPUF_SUPPORTS_FPU;		// Not strictly true, but very slow anyway

	// Enable FPU support...

	CPUEnableExtensions(lEnableFlags);

	VDFastMemcpyAutodetect();
}


bool BitBltFromI420ToI420(int w, int h, BYTE* dsty, BYTE* dstu, BYTE* dstv, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch)
{
	VDPixmap srcbm = {0};

	srcbm.data		= srcy;
	srcbm.pitch		= srcpitch;
	srcbm.w			= w;
	srcbm.h			= h;
	srcbm.format	= nsVDPixmap::kPixFormat_YUV420_Planar;
	srcbm.data2		= srcu;
	srcbm.pitch2	= srcpitch / 2;
	srcbm.data3		= srcv;
	srcbm.pitch3	= srcpitch / 2;

	VDPixmap dstpxm = {0};

	dstpxm.data		= dsty;
	dstpxm.pitch	= dstpitch;
	dstpxm.w		= w;
	dstpxm.h		= h;
	dstpxm.format	= nsVDPixmap::kPixFormat_YUV420_Planar;
	dstpxm.data2	= dstu;
	dstpxm.pitch2	= dstpitch / 2;
	dstpxm.data3	= dstv;
	dstpxm.pitch3	= dstpitch / 2;

	return VDPixmapBlt(dstpxm, srcbm);
}

bool BitBltFromI420ToNV12(int w, int h, BYTE* dsty, BYTE* dstu, BYTE* dstv, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch)
{
	VDPixmap srcbm = {0};

	srcbm.data		= srcy;
	srcbm.pitch		= srcpitch;
	srcbm.w			= w;
	srcbm.h			= h;
	srcbm.format	= nsVDPixmap::kPixFormat_YUV420_Planar;
	srcbm.data2		= srcu;
	srcbm.pitch2	= srcpitch / 2;
	srcbm.data3		= srcv;
	srcbm.pitch3	= srcpitch / 2;

	VDPixmap dstpxm	= {0};

	dstpxm.data		= dsty;
	dstpxm.pitch	= dstpitch;
	dstpxm.w		= w;
	dstpxm.h		= h;
	dstpxm.format	= nsVDPixmap::kPixFormat_YUV420_NV12;
	dstpxm.data2	= dstu;
	dstpxm.pitch2	= dstpitch;
	dstpxm.data3	= dstv;
	dstpxm.pitch3	= dstpitch;

	return VDPixmapBlt(dstpxm, srcbm);
}

bool BitBltFromYUY2ToYUY2(int w, int h, BYTE* dst, int dstpitch, BYTE* src, int srcpitch)
{
	VDPixmap srcbm = {0};

	srcbm.data		= src;
	srcbm.pitch		= srcpitch;
	srcbm.w			= w;
	srcbm.h			= h;
	srcbm.format	= nsVDPixmap::kPixFormat_YUV422_YUYV;

	VDPixmap dstpxm = {
		dst,
		NULL,
		w,
		h,
		dstpitch
	};

	dstpxm.format = nsVDPixmap::kPixFormat_YUV422_YUYV;

	return VDPixmapBlt(dstpxm, srcbm);
}

bool BitBltFromI420ToRGB(int w, int h, BYTE* dst, int dstpitch, int dbpp, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch)
{
	VDPixmap srcbm = {0};

	srcbm.data		= srcy;
	srcbm.pitch		= srcpitch;
	srcbm.w			= w;
	srcbm.h			= h;
	srcbm.format	= nsVDPixmap::kPixFormat_YUV420_Planar;
	srcbm.data2		= srcu;
	srcbm.pitch2	= srcpitch/2;
	srcbm.data3		= srcv;
	srcbm.pitch3	= srcpitch/2;

	VDPixmap dstpxm = {
		(char *)dst + dstpitch * (h - 1),
		NULL,
		w,
		h,
		-dstpitch
	};

	switch(dbpp) {
	case 16:
		dstpxm.format = nsVDPixmap::kPixFormat_RGB565;
		break;
	case 24:
		dstpxm.format = nsVDPixmap::kPixFormat_RGB888;
		break;
	case 32:
		dstpxm.format = nsVDPixmap::kPixFormat_XRGB8888;
		break;
	default:
		VDASSERT(false);
	}

	return VDPixmapBlt(dstpxm, srcbm);
}

bool BitBltFromI420ToYUY2(int w, int h, BYTE* dst, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch)
{
	if(srcpitch == 0) srcpitch = w;

#ifndef _WIN64
	if(!((DWORD_PTR)srcy&15) && !((DWORD_PTR)srcu&15) && !((DWORD_PTR)srcv&15) && !(srcpitch&31)
		&& !((DWORD_PTR)dst&15) && !(dstpitch&15))
	{
		if(w<=0 || h<=0 || (w&1) || (h&1))
			return false;

		yv12_yuy2_sse2(srcy, srcu, srcv, srcpitch/2, w/2, h, dst, dstpitch);
		return true;
	}
#endif

	VDPixmap srcbm = {0};

	srcbm.data		= srcy;
	srcbm.pitch		= srcpitch;
	srcbm.w			= w;
	srcbm.h			= h;
	srcbm.format	= nsVDPixmap::kPixFormat_YUV420_Planar;
	srcbm.data2		= srcu;
	srcbm.pitch2	= srcpitch/2;
	srcbm.data3		= srcv;
	srcbm.pitch3	= srcpitch/2;

	VDPixmap dstpxm = {
		dst,
		NULL,
		w,
		h,
		dstpitch
	};

	dstpxm.format = nsVDPixmap::kPixFormat_YUV422_YUYV;

	return VDPixmapBlt(dstpxm, srcbm);
}

bool BitBltFromRGBToRGB(int w, int h, BYTE* dst, int dstpitch, int dbpp, BYTE* src, int srcpitch, int sbpp)
{
	VDPixmap srcbm = {
		(char *)src + srcpitch * (h - 1),
		NULL,
		w,
		h,
		-srcpitch
	};

	switch(sbpp) {
	case 8:
		srcbm.format = nsVDPixmap::kPixFormat_Pal8;
		break;
	case 16:
		srcbm.format = nsVDPixmap::kPixFormat_RGB565;
		break;
	case 24:
		srcbm.format = nsVDPixmap::kPixFormat_RGB888;
		break;
	case 32:
		srcbm.format = nsVDPixmap::kPixFormat_XRGB8888;
		break;
	default:
		VDASSERT(false);
	}

	VDPixmap dstpxm = {
		(char *)dst + dstpitch * (h - 1),
		NULL,
		w,
		h,
		-dstpitch
	};

	switch(dbpp) {
	case 8:
		dstpxm.format = nsVDPixmap::kPixFormat_Pal8;
		break;
	case 16:
		dstpxm.format = nsVDPixmap::kPixFormat_RGB565;
		break;
	case 24:
		dstpxm.format = nsVDPixmap::kPixFormat_RGB888;
		break;
	case 32:
		dstpxm.format = nsVDPixmap::kPixFormat_XRGB8888;
		break;
	default:
		VDASSERT(false);
	}

	return VDPixmapBlt(dstpxm, srcbm);
}


bool BitBltFromYUY2ToRGB(int w, int h, BYTE* dst, int dstpitch, int dbpp, BYTE* src, int srcpitch)
{
	if(srcpitch == 0) srcpitch = w;

	VDPixmap srcbm = {0};

	srcbm.data		= src;
	srcbm.pitch		= srcpitch;
	srcbm.w			= w;
	srcbm.h			= h;
	srcbm.format	= nsVDPixmap::kPixFormat_YUV422_YUYV;

	VDPixmap dstpxm = {
		(char *)dst + dstpitch * (h - 1),
		NULL,
		w,
		h,
		-dstpitch
	};

	switch(dbpp) {
	case 16:
		dstpxm.format = nsVDPixmap::kPixFormat_RGB565;
		break;
	case 24:
		dstpxm.format = nsVDPixmap::kPixFormat_RGB888;
		break;
	case 32:
		dstpxm.format = nsVDPixmap::kPixFormat_XRGB8888;
		break;
	default:
		VDASSERT(false);
	}

	return VDPixmapBlt(dstpxm, srcbm);
}

static void yuvtoyuy2row_c(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width)
{
	WORD* dstw = (WORD*)dst;
	for(; width > 1; width -= 2)
	{
		*dstw++ = (*srcu++<<8)|*srcy++;
		*dstw++ = (*srcv++<<8)|*srcy++;
	}
}

static void yuvtoyuy2row_avg_c(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width, DWORD pitchuv)
{
	WORD* dstw = (WORD*)dst;
	for(; width > 1; width -= 2, srcu++, srcv++)
	{
		*dstw++ = (((srcu[0]+srcu[pitchuv])>>1)<<8)|*srcy++;
		*dstw++ = (((srcv[0]+srcv[pitchuv])>>1)<<8)|*srcy++;
	}
}

bool BitBltFromI420ToYUY2Interlaced(int w, int h, BYTE* dst, int dstpitch, BYTE* srcy, BYTE* srcu, BYTE* srcv, int srcpitch)
{
	if(w<=0 || h<=0 || (w&1) || (h&1))
		return false;

	if(srcpitch == 0) srcpitch = w;

	void (*yuvtoyuy2row)(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width) = NULL;
	void (*yuvtoyuy2row_avg)(BYTE* dst, BYTE* srcy, BYTE* srcu, BYTE* srcv, DWORD width, DWORD pitchuv) = NULL;

#ifndef _WIN64
	if(!((DWORD_PTR)srcy&15) && !((DWORD_PTR)srcu&15) && !((DWORD_PTR)srcv&15) && !(srcpitch&31)
		&& !((DWORD_PTR)dst&15) && !(dstpitch&15))
	{
		yv12_yuy2_sse2_interlaced(srcy, srcu, srcv, srcpitch/2, w/2, h, dst, dstpitch);
		return true;
	}

	if(!(w&7))
	{
		yuvtoyuy2row = yuvtoyuy2row_MMX;
		yuvtoyuy2row_avg = yuvtoyuy2row_avg_MMX;
	}
	else
#endif
	{
		yuvtoyuy2row = yuvtoyuy2row_c;
		yuvtoyuy2row_avg = yuvtoyuy2row_avg_c;
	}

	if(!yuvtoyuy2row)
		return false;

	int halfsrcpitch = srcpitch/2;
	do
	{
		yuvtoyuy2row(dst, srcy, srcu, srcv, w);
		yuvtoyuy2row_avg(dst + dstpitch, srcy + srcpitch, srcu, srcv, w, halfsrcpitch);

		dst += 2*dstpitch;
		srcy += 2*srcpitch;
		srcu += halfsrcpitch;
		srcv += halfsrcpitch;
	}
	while((h -= 2) > 2);

	yuvtoyuy2row(dst, srcy, srcu, srcv, w);
	yuvtoyuy2row(dst + dstpitch, srcy + srcpitch, srcu, srcv, w);

#ifndef _WIN64
	__asm emms
#endif

	return true;
}
