

#include "avisynth/avisynth_stdafx.h"

#include "focus.h"

extern const AVSFunction Focus_filters[] = {
  { "Blur", "cf[]f[mmx]b", Create_Blur },                     // amount [-1.0 - 1.5849625] -- log2(3)
  { "Sharpen", "cf[]f[mmx]b", Create_Sharpen },               // amount [-1.5849625 - 1.0]
  { 0 }
};


AdjustFocusV::AdjustFocusV(double _amount, PClip _child, bool _mmx)
: GenericVideoFilter(_child,__FUNCTION__ ), amount(int(32768*pow(2.0, _amount)+0.5)), line(NULL), mmx(_mmx) {}

AdjustFocusV::~AdjustFocusV(void) 
{ 
  if (line) delete[] line; 
}

// --------------------------------
// Blur/Sharpen Vertical GetFrame()
// --------------------------------

PVideoFrame __stdcall AdjustFocusV::GetFrame(int n, IScriptEnvironment* env) 
{
	PVideoFrame frame = child->GetFrame(n, env);
	if (frame == nullptr || frame.m_ptr == nullptr) {
		return nullptr;
	}
	env->MakeWritable(&frame);
	if (!line)
		line = new uc[frame->GetRowSize()+32];
	uc* linea = (uc*)(((intptr_t)line+15) & -16); // Align 16

	if (vi.IsPlanar()) {
		for(int cplane=0;cplane<3;cplane++) {
			int plane=0;
			if (cplane==0)  plane = PLANAR_Y;
			if (cplane==1)  plane = PLANAR_U;
			if (cplane==2)  plane = PLANAR_V;
			uc* buf      = frame->GetWritePtr(plane);
			int pitch    = frame->GetPitch(plane);
			int row_size = frame->GetRowSize(plane);
			int height   = frame->GetHeight(plane);
			memcpy(linea, buf, row_size); // First row - map centre as upper
			// All normal cases will have pitch aligned 16, we
			// need 8. If someone works hard enough to override
			// this we can't process the short fall. Use C Code.
#ifdef _M_IX86
			if (mmx && (pitch >= ((row_size+7) & -8))) {
				AFV_MMX(linea, buf, height, pitch, row_size, amount);
			} else
#endif
			{
				AFV_C(linea, buf, height, pitch, row_size, amount);
			}
		}

	} else {
		uc* buf      = frame->GetWritePtr();
		int pitch    = frame->GetPitch();
		int row_size = vi.RowSize();
		int height   = vi.height;
		memcpy(linea, buf, row_size); // First row - map centre as upper
#ifdef _M_IX86
		if (mmx && (pitch >= ((row_size+7) & -8))) {
			AFV_MMX(linea, buf, height, pitch, row_size, amount);
		} else 
#endif
		{
			AFV_C(linea, buf, height, pitch, row_size, amount);
		}
	}
	return frame;
}

// ------------------------------
// Blur/Sharpen Vertical C++ Code
// ------------------------------

void AFV_C(uc* l, uc* p, const int height, const int pitch, const int row_size, const int amount) {
	const int center_weight = amount*2;
	const int outer_weight = 32768-amount;
	for (int y = height-1; y>0; --y) {
		for (int x = 0; x < row_size; ++x) {
			const uc a = ScaledPixelClip(p[x] * center_weight + (l[x] + p[x+pitch]) * outer_weight);
			l[x] = p[x];
			p[x] = a;
		}
		p += pitch;
	}
	for (int x = 0; x < row_size; ++x) { // Last row - map centre as lower
		p[x] = ScaledPixelClip(p[x] * center_weight + (l[x] + p[x]) * outer_weight);
	}
}

// ------------------------------
// Blur/Sharpen Vertical MMX Code
// ------------------------------
#ifdef _M_IX86
void AFV_MMX(const uc* l, const uc* p, const int height, const int pitch, const int row_size, const int amount) {
	// round masks
	__declspec(align(8)) const static __int64 r7 = 0x0040004000400040;

	__asm { 
		sub		esp,8				; temp space on stack
		// signed word center weight ((amount+0x100)>>9) x4
		mov		eax,amount
		add		eax,100h
		sar		eax,9               ; [21..128]
		mov		[esp],ax
		mov		[esp+2],ax
		mov		[esp+4],ax
		mov		[esp+6],ax
		movq	mm7,[esp]
		// signed word outer weight 64-((amount+0x100)>>9) x4
		mov		ax,40h
		sub		ax,[esp]            ; [43..-64]
		mov		[esp],ax
		mov		[esp+2],ax
		mov		[esp+4],ax
		mov		[esp+6],ax
		movq	mm6,[esp]

		add		esp,8
		pxor	mm0,mm0
	}
	
	int nloops = __min(pitch, (row_size+7) & -8) >> 3;

	for (int y = height-1; y>0; --y) {

	__asm {
		mov			edx,p			; frame buffer
		mov			ecx,pitch
		mov			eax,edx
		add			eax,ecx
		mov			p,eax			; p += pitch;
		mov			eax,l			; line buffer
		mov			edi,nloops

		align		16
row_loop:
		movq		mm4,[eax]		; 8 Upper pixels (buffered)
		 movq		mm5,[edx+ecx]	; 8 Lower pixels
		movq		mm1,mm4			; Duplicate uppers
		 movq		mm3,mm5			; Duplicate lowers
		punpcklbw	mm4,mm0			; unpack 4 low bytes uppers
		 movq		mm2,[edx]		; 8 Centre pixels
		punpcklbw	mm5,mm0			; unpack 4 low bytes lowers
		 movq		[eax],mm2		; Save centres as next uppers
		paddw		mm5,mm4			; Uppers + Lowers				-- low
		 movq		mm4,mm2			; Duplicate centres
		pmullw		mm5,mm6			; *= outer weight				-- low [-32130, 21930]
		 punpcklbw	mm4,mm0			; unpack 4 low bytes centres
		punpckhbw	mm1,mm0			; unpack 4 high bytes uppers
		 punpckhbw	mm3,mm0			; unpack 4 high bytes lowers
		pmullw		mm4,mm7			; *= centre weight				-- low [32640, 5355]
		 paddw		mm1,mm3			; Uppers + Lowers				-- high
		punpckhbw	mm2,mm0			; unpack 4 high bytes centres
		 pmullw		mm1,mm6			; *= outer weight				-- high
		pmullw		mm2,mm7			; *= centre weight				-- high
		 paddsw		mm5,mm4			; Weighted outers+=0.5*centres	-- low
		paddsw		mm1,mm2			; Weighted outers+=0.5*centres	-- high
		 paddsw		mm5,mm4			; +=0.5*centres					-- low
		paddsw		mm1,mm2			; +=0.5*centres					-- high
 		 paddsw		mm5,r7			; += 0.5						-- low
		paddsw		mm1,r7			; += 0.5						-- high
		 psraw		mm5,7			; /= 128						-- low
		add			eax,8			; upper += 8
		 psraw		mm1,7			; /= 128						-- high
		add			edx,8			; centre += 8
		 packuswb	mm5,mm1			; pack 4 lows with 4 highs
		dec			edi				; count--
		 movq		[edx-8],mm5		; Update 8 pixels
		jnle		row_loop		; 
		}

	} // for (int y = height

	__asm { // Last row - map centre as lower
		mov			edx,p			; frame buffer
		mov			eax,l			; line buffer
		mov			edi,nloops

		align		16
lrow_loop:
		movq		mm5,[eax]		; 8 Upper pixels (buffered)
		 movq		mm4,[edx]		; 8 Centre pixels as lowers
		movq		mm1,mm5			; Duplicate uppers
		 movq		mm3,mm4			; Duplicate lowers (centres)
		punpcklbw	mm5,mm0			; unpack 4 low bytes uppers
		 punpcklbw	mm4,mm0			; unpack 4 low bytes lowers (centres)
		punpckhbw	mm3,mm0			; unpack 4 high bytes lowers (centres)
		 paddw		mm5,mm4			; Uppers + lowers (centres)		-- low
		punpckhbw	mm1,mm0			; unpack 4 high bytes uppers
		 pmullw		mm5,mm6			; *= outer weight				-- low
		paddw		mm1,mm3			; Uppers + lowers (centres)		-- high
		 pmullw		mm4,mm7			; *= centre weight				-- low
		pmullw		mm1,mm6			; *= outer weight				-- high
		 pmullw		mm3,mm7			; *= centre weight				-- high
		paddsw		mm5,mm4			; Weighted outers+=0.5*centres	-- low
		 paddsw		mm1,mm3			; Weighted outers+=0.5*centres	-- high
		paddsw		mm5,mm4			; +=0.5*centres					-- low
		 paddsw		mm1,mm3			; +=0.5*centres					-- high
		paddsw		mm5,r7			; += 0.5						-- low
		 paddsw		mm1,r7			; += 0.5						-- high
		psraw		mm5,7			; /= 128						-- low
		 add		eax,8			; upper += 8
		psraw		mm1,7			; /= 128						-- high
		 add		edx,8			; centre += 8
		packuswb	mm5,mm1			; pack 4 lows with 4 highs
		 dec		edi				; count--
		movq		[edx-8],mm5		; Update 8 pixels
		 jnle		lrow_loop		; 
	}
	__asm emms
}
#endif

AdjustFocusH::AdjustFocusH(double _amount, PClip _child, bool _mmx)
: GenericVideoFilter(_child,__FUNCTION__ ), amount(int(32768*pow(2.0, _amount)+0.5)), mmx(_mmx) {}

// ----------------------------------
// Blur/Sharpen Horizontal GetFrame()
// ----------------------------------

PVideoFrame __stdcall AdjustFocusH::GetFrame(int n, IScriptEnvironment* env) 
{
	PVideoFrame frame = child->GetFrame(n, env);
	if (frame == nullptr || frame.m_ptr == nullptr) {
		return nullptr;
	}
	env->MakeWritable(&frame); // Damn! This screws FillBorder

	if (vi.IsPlanar()) {
		for(int cplane=0;cplane<3;cplane++) {
			int plane=0;
			if (cplane==0) plane = PLANAR_Y;
			if (cplane==1) plane = PLANAR_U;
			if (cplane==2) plane = PLANAR_V;
			const int row_size = frame->GetRowSize(plane);
			uc* q = frame->GetWritePtr(plane);
			const int pitch = frame->GetPitch(plane);
			const int height = frame->GetHeight(plane);
#ifdef _M_IX86
			if (mmx && (pitch >= ((row_size+7) & -8))) {
				AFH_YV12_MMX(q, height, pitch, row_size, amount);
			}
            else 
#endif
			{
				AFH_YV12_C(q, height, pitch, row_size, amount);
			} 
		}
	} else {
		uc* q = frame->GetWritePtr();
		const int pitch = frame->GetPitch();
		if (vi.IsYUY2()) {
#ifdef _M_IX86
			if (mmx) {
				AFH_YUY2_MMX(q, vi.height, pitch, vi.width, amount);
			}
            else
#endif
			{
				AFH_YUY2_C(q, vi.height, pitch, vi.width, amount);
			}
		} 
		else if (vi.IsRGB32()) {
#ifdef _M_IX86
			if (mmx) {
				AFH_RGB32_MMX(q, vi.height, pitch, vi.width, amount);
			}
            else 
#endif
			{
				AFH_RGB32_C(q, vi.height, pitch, vi.width, amount);
			}
		} 
		else { //rgb24
			AFH_RGB24_C(q, vi.height, pitch, vi.width, amount);
		}
	}

	return frame;
}

// --------------------------------------
// Blur/Sharpen Horizontal RGB32 C++ Code
// --------------------------------------

void AFH_RGB32_C(uc* p, int height, const int pitch, const int width, const int amount) {
	const int center_weight = amount*2;
	const int outer_weight = 32768-amount;
	for (int y = height; y>0; --y) 
	{
		uc bb = p[0];
		uc gg = p[1];
		uc rr = p[2];
		uc aa = p[3];
		int x;
		for (x = 0; x < width-1; ++x) 
		{
			const uc b = ScaledPixelClip(p[x*4+0] * center_weight + (bb + p[x*4+4]) * outer_weight);
			bb = p[x*4+0];
			p[x*4+0] = b;

			const uc g = ScaledPixelClip(p[x*4+1] * center_weight + (gg + p[x*4+5]) * outer_weight);
			gg = p[x*4+1];
			p[x*4+1] = g;

			const uc r = ScaledPixelClip(p[x*4+2] * center_weight + (rr + p[x*4+6]) * outer_weight);
			rr = p[x*4+2];
			p[x*4+2] = r;

			const uc a = ScaledPixelClip(p[x*4+3] * center_weight + (aa + p[x*4+7]) * outer_weight);
			aa = p[x*4+3];
			p[x*4+3] = a;
		}
		p[x*4+0] = ScaledPixelClip(p[x*4+0] * center_weight + (bb + p[x*4+0]) * outer_weight);
		p[x*4+1] = ScaledPixelClip(p[x*4+1] * center_weight + (gg + p[x*4+1]) * outer_weight);
		p[x*4+2] = ScaledPixelClip(p[x*4+2] * center_weight + (rr + p[x*4+2]) * outer_weight);
		p[x*4+3] = ScaledPixelClip(p[x*4+3] * center_weight + (aa + p[x*4+3]) * outer_weight);
		p += pitch;
	}
}

// --------------------------------------
// Blur/Sharpen Horizontal RGB32 MMX Code
// --------------------------------------
#ifdef _M_IX86
void AFH_RGB32_MMX(const uc* p, const int height, const int pitch, const int width, const int amount) {
	// round masks
	__declspec(align(8)) const __int64 r7 = 0x0040004000400040;
	// weights
	__declspec(align(8)) __int64 cw;
	__declspec(align(8)) __int64 ow;
	__asm { 
		// signed word center weight ((amount+0x100)>>9) x4
		mov		eax,amount
		add		eax,100h
		sar		eax,9
		lea		edx,cw
		mov		[edx],ax
		mov		[edx+2],ax
		mov		[edx+4],ax
		mov		[edx+6],ax
		// signed word outer weight 64-((amount+0x100)>>9) x4
		mov		ax,40h
		sub		ax,[edx]
		lea		edx,ow
		mov		[edx],ax
		mov		[edx+2],ax
		mov		[edx+4],ax
		mov		[edx+6],ax
	}
	for (int y=0;y<height;y++) {
	__asm {
		mov			ecx,p
		mov			edi,width

		movq		mm1,[ecx]		; trash + left pixel
		pxor		mm0,mm0			; zeros
		movq		mm2,mm1			; centre + right pixel
		psllq		mm1,32			; left + zero

		align		16
row_loop:
		dec			edi
		jle			odd_end

		movq		mm7,mm2			; duplicate right pixel
		punpckhbw	mm1,mm0			; unpack left pixel
		punpckhbw	mm7,mm0			; unpack right pixel
		movq		mm4,mm2			; duplicate centre pixel
		paddsw		mm7,mm1			; right + left
		punpcklbw	mm4,mm0			; unpack centre pixel
		pmullw		mm7,ow			; *= outer weight
		pmullw		mm4,cw			; *= centre weight
		movq		mm1,mm2			; left + centre pixel
		paddsw		mm7,mm4			; Weighted centres + outers
		dec			edi
		paddsw		mm7,mm4			; Weighted centres + outers
		paddsw		mm7,r7			; += 0.5
		psraw		mm7,7			; /= 32768
		jle			even_end

		movq		mm6,mm1			; duplicate left pixel
		movq		mm2,[ecx+8]		; right + trash pixel
		punpcklbw	mm6,mm0			; unpack left pixel
		movq		mm5,mm2			; duplicate right pixel
		movq		mm4,mm1			; duplicate centre pixel
		punpcklbw	mm5,mm0			; unpack right pixel
		punpckhbw	mm4,mm0			; unpack centre pixel
		paddsw		mm6,mm5			; left + right
		pmullw		mm4,cw			; *= centre weight
		pmullw		mm6,ow			; *= outer weight
		paddsw		mm6,mm4			; Weighted centres + outers
		paddsw		mm6,mm4			; Weighted centres + outers
		paddsw		mm6,r7			; += 0.5
		psraw		mm6,7			; /= 32768
		add			ecx,8
		packuswb	mm7,mm6			; pack low with high
		movq		[ecx-8],mm7		; Update 2 centre pixels
		jmp			row_loop

		align		16
odd_end:
		punpckhbw	mm1,mm0			; unpack left pixel
		punpcklbw	mm2,mm0			; unpack centre pixel
		paddsw		mm1,mm2			; left + centre
		pmullw		mm2,cw			; *= centre weight
		pmullw		mm1,ow			; *= outer weight
		paddsw		mm1,mm2			; Weighted centres + outers
		paddsw		mm1,mm2			; Weighted centres + outers
		paddsw		mm1,r7			; += 0.5
		psraw		mm1,7			; /= 32768
		packuswb	mm1,mm1			; pack low with high
		movd		[ecx],mm1		; Update 1 centre pixels
		jmp			next_loop

		align		16
even_end:
		punpckhbw	mm2,mm0			; unpack centre pixel
		punpcklbw	mm1,mm0			; unpack left pixel
		paddsw		mm1,mm2			; left + centre
		pmullw		mm2,cw			; *= centre weight
		pmullw		mm1,ow			; *= outer weight
		paddsw		mm1,mm2			; Weighted centres + outers
		paddsw		mm1,mm2			; Weighted centres + outers
		paddsw		mm1,r7			; += 0.5
		psraw		mm1,7			; /= 32768
		packuswb	mm7,mm1			; pack low with high
		movq		[ecx],mm7		; Update 2 centre pixels

next_loop:
		}
		p += pitch;
	}
	__asm emms
}
#endif
// -------------------------------------
// Blur/Sharpen Horizontal YUY2 C++ Code
// -------------------------------------

void AFH_YUY2_C(uc* p, int height, const int pitch, const int width, const int amount) {
	const int center_weight = amount*2;
	const int outer_weight = 32768-amount;
	for (int y = height; y>0; --y) 
	{
		uc yy = p[0];
		uc uv = p[1];
		uc vu = p[3];
		int x;
		for (x = 0; x < width-2; ++x) 
		{
			const uc y = ScaledPixelClip(p[x*2+0] * center_weight + (yy + p[x*2+2]) * outer_weight);
			yy = p[x*2+0];
			p[x*2+0] = y;
			const uc w = ScaledPixelClip(p[x*2+1] * center_weight + (uv + p[x*2+5]) * outer_weight);
			uv = vu;
			vu = p[x*2+1];
			p[x*2+1] = w;
		}
		const uc y2 = ScaledPixelClip(p[x*2+0] * center_weight + (yy + p[x*2+2]) * outer_weight);
		yy         = p[x*2+0];
		p[x*2+0]   = y2;
		p[x*2+1]   = ScaledPixelClip(p[x*2+1] * center_weight + (uv + p[x*2+1]) * outer_weight);
		p[x*2+2]   = ScaledPixelClip(p[x*2+2] * center_weight + (yy + p[x*2+2]) * outer_weight);
		p[x*2+3]   = ScaledPixelClip(p[x*2+3] * center_weight + (vu + p[x*2+3]) * outer_weight);
   
		p += pitch;
	}
}


// -------------------------------------
// Blur/Sharpen Horizontal YUY2 MMX Code
// -------------------------------------
#ifdef _M_IX86
void AFH_YUY2_MMX(const uc* p, const int height, const int pitch, const int width, const int amount) {
	// round masks
	__declspec(align(8)) const __int64 r7 = 0x0040004000400040;
	// YY and UV masks
	__declspec(align(8)) const __int64 ym = 0x00FF00FF00FF00FF;
	__declspec(align(8)) const __int64 cm = 0xFF00FF00FF00FF00;

	__declspec(align(8)) const __int64 rightmask = 0x00FF000000000000;
	// weights
	__declspec(align(8)) __int64 cw;
	__declspec(align(8)) __int64 ow;

	__asm { 
		// signed word center weight ((amount+0x100)>>9) x4
		mov		eax,amount
		add		eax,100h
		sar		eax,9
		lea		edx,cw
		mov		[edx],ax
		mov		[edx+2],ax
		mov		[edx+4],ax
		mov		[edx+6],ax
		// signed word outer weight 64-((amount+0x100)>>9) x4
		mov		ax,40h
		sub		ax,[edx]
		lea		edx,ow
		mov		[edx],ax
		mov		[edx+2],ax
		mov		[edx+4],ax
		mov		[edx+6],ax
	}

	for (int y=0;y<height;y++) {
	__asm {
		mov			ecx,p
		 movq		mm1,ym			; 0x00FF00FF00FF00FF
		movq		mm2,[ecx]		; [2v 3y 2u 2y 0v 1y 0u 0y]	-- Centre
		 movq		mm3,cm			; 0xFF00FF00FF00FF00
		pand		mm1,mm2			; [00 3y 00 2y 00 1y 00 0y]
		 pand		mm3,mm2			; [2v 00 2u 00 0v 00 0u 00]
		psllq		mm1,48			; [00 0y 00 00 00 00 00 00]
		 psllq		mm3,32			; [0v 00 0u 00 00 00 00 00]
		mov			edi,width
		 por		mm1,mm3			; [0v 0y 0u 00 00 00 00 00]	-- Left
		sub			edi,2

		align		16
row_loop:
		jle			odd_end
		sub			edi,2
		jle			even_end

		movq		mm0,mm1			;   [-2v -1y -2u -2y -4v -3y -4u -4y]
		 movq		mm3,[ecx+8]		; [6v 7y 6u 6y 4v 5y 4u 4y]	-- Right
		movq		mm5,ym			; 0x00FF00FF00FF00FF
		 movq		mm6,mm3			;   [6v 7y 6u 6y 4v 5y 4u 4y]
		pand		mm0,mm5			;   [00-1y 00-2y 00-3y 00-4y]
		 pand		mm6,mm5			;   [007y 006y 005y 004y]
		psrlq		mm0,48			;   [0000  0000  0000  00-1y]
		 pand		mm5,mm2			; [003y 002y 001y 000y]		-- Centre
		psllq		mm6,48			;   [004y 0000 0000 0000]
		 movq		mm7,mm5			;   [003y 002y 001y 000y]
		movq		mm4,mm5			;   [003y 002y 001y 000y]
		 psllq		mm7,16			;   [002y 001y 000y 0000]
		psrlq		mm5,16			;   [0000 003y 002y 001y]
		 por		mm0,mm7			; [002y 001y 000y 00-1y]	-- Left
		por			mm6,mm5			; [004y 003y 002y 001y]		-- Right
		 pmullw		mm4,cw			; *= center weight
		paddsw		mm0,mm6			; left += right 
		 movq		mm6,cm			; 0xFF00FF00FF00FF00
		pmullw		mm0,ow			; *= outer weight
		 pand		mm1,mm6			;   [-2v00 -2u00 -4v00 -4u00]
		movq		mm5,mm2			;   [2v 3y 2u 2y 0v 1y 0u 0y]
		 paddsw		mm0,mm4			; Weighted centres + outers
		psrlq		mm1,40			;   [0000 0000 00-2v 00-2u]
		 pand		mm5,mm6			;   [2v00 2u00 0v00 0u00]
		paddsw		mm0,mm4			; Weighted centres + outers
		 psrlq		mm5,8			; [002v 002u 000v 000u]		-- Centre
		pand		mm6,mm3			;   [6v00 6u00 4v00 4u00]
		 movq		mm7,mm5			;   [002v 002u 000v 000u]
		movq		mm4,mm5			;   [002v 002u 000v 000u]
		 psllq		mm6,24			;   [004v 004u 0000 0000]
		psllq		mm7,32			;   [000v 000u 0000 0000]
		 psrlq		mm4,32			;   [0000 0000 002v 002u]
		por			mm7,mm1			; [000v 000u 00-2v 00-2u]	-- Left
		 por		mm6,mm4			; [004v 004u 002v 002u]		-- Right
		 paddsw		mm7,mm6			; left += right
		pmullw		mm5,cw			; *= center weight
		 pmullw		mm7,ow			; *= outer weight
		add			ecx,8			; 
		 paddsw		mm7,mm5			; Weighted centres + outers
		paddsw		mm0,r7			; += 0.5
		 paddsw		mm7,mm5			; Weighted centres + outers
		psraw		mm0,7			; /= 32768
		 paddsw		mm7,r7			; += 0.5
		packuswb	mm0,mm0			; [3y 2y 1y 0y 3y 2y 1y 0y] -- Unsign Saturated
		 psraw		mm7,7			; /= 32768
		movq		mm1,mm2			; 
		 packuswb	mm7,mm7			; [2v 2u 0v 0u 2v 2u 0v 0u] -- Unsign Saturated
		movq		mm2,mm3			; 
		 punpcklbw	mm0,mm7			; [2v 3y 2u 2y 0v 1y 0v 0y]
		sub			edi,2
		 movq		[ecx-8],mm0		; Update 4 centre pixels
		jmp			row_loop		; 

		align		16
odd_end:
		movq		mm5,ym			; 0x00FF00FF00FF00FF
		 movq		mm0,mm1			;   [-2v -1y -2u -2y -4v -3y -4u -4y]
	;stall
		 pand		mm0,mm5			;   [00-1y 00-2y 00-3y 00-4y]
		pand		mm5,mm2			; [00xx 00xx 001y 000y]		-- Centre
		 psrlq		mm0,48			;   [0000  0000  0000  00-1y]
		movq		mm7,mm5			;   [00xx 00xx 001y 000y]
		 movq		mm6,mm5			;   [00xx 00xx 001y 000y]
		psllq		mm7,16			;   [00xx 001y 000y 0000]
		 psrlq		mm6,16			;   [0000 00xx 00xx 001y]
		por			mm0,mm7			; [00xx 001y 000y 00-1y]	-- Left
		 punpcklwd	mm6,mm6			; [00xx 00xx 001y 001y]		-- Right
		pmullw		mm5,cw			; *= center weight YY
		 paddsw		mm0,mm6			; left += right  YY
		movq		mm3,cm			; 0xFF00FF00FF00FF00
		 pmullw		mm0,ow			; *= outer weight YY
		pand		mm1,mm3			;   [-2v00 -2u00 -4v00 -4u00]
		pand		mm3,mm2			;   [xx00 xx00 0v00 0u00]
		 psrlq		mm1,40			; [0000 0000 00-2v 00-2u]	-- Left
		psrlq		mm3,8			; [0000 0000 000v 000u]		-- Centre
		paddsw		mm1,mm3			; left += centre UV
		 pmullw		mm3,cw			; *= center weight UV
		pmullw		mm1,ow			; *= outer weight UV
		 paddsw		mm0,mm5			; Weighted centres + outers YY
		paddsw		mm1,mm3			; Weighted centres + outers UV
		 paddsw		mm0,mm5			; Weighted centres + outers YY
		paddsw		mm1,mm3			; Weighted centres + outers UV
		 paddsw		mm0,r7			; += 0.5 YY
		paddsw		mm1,r7			; += 0.5 UV
		 psraw		mm0,7			; /= 32768 YY
		psraw		mm1,7			; /= 32768 UV
		 packuswb	mm0,mm0			; [xx xx 1y 0y xx xx 1y 0y] -- Unsign Saturated
		packuswb	mm1,mm1			; [xx xx 0v 0u xx xx 0v 0u] -- Unsign Saturated
		punpcklbw	mm0,mm1			; [xx xx xx xx 0v 1y 0v 0y]
		movd		[ecx],mm0		; Update 2 centre pixels
		jmp			next_loop

		align		16
even_end:

		 movq		mm5,ym			; 0x00FF00FF00FF00FF
		movq		mm0,mm1			;   [-2v -1y -2u -2y -4v -3y -4u -4y]
		 movq		mm6,rightmask	; 0x00FF000000000000
		pand		mm0,mm5			;   [00-1y 00-2y 00-3y 00-4y]
		 pand		mm6,mm2			;   [003y 0000 0000 0000]
		pand		mm5,mm2			; [003y 002y 001y 000y]		-- Centre
		 psrlq		mm0,48			;   [0000  0000  0000  00-1y]
		movq		mm7,mm5			;   [003y 002y 001y 000y]
		 movq		mm4,mm5			;   [003y 002y 001y 000y]
		psllq		mm7,16			;   [002y 001y 000y 0000]
		 psrlq		mm4,16			;   [0000 003y 002y 001y]
		por			mm0,mm7			; [002y 001y 000y 00-1y]	-- Left
		 por		mm6,mm4			; [003y 003y 002y 001y]		-- Right
		movq		mm3,cm			; 0xFF00FF00FF00FF00
		 paddsw		mm0,mm6			; left += right 
		pmullw		mm5,cw			; *= center weight
		 pmullw		mm0,ow			; *= outer weight
		 pand		mm1,mm3			;   [-2v00 -2u00 -4v00 -4u00]
		pand		mm3,mm2			;   [2v00 2u00 0v00 0u00]
		 psrlq		mm1,40			;   [0000 0000 00-2v 00-2u]
		movq		mm4,mm3			;   [2v00 2u00 0v00 0u00]
		 movq		mm7,mm3			;   [2v00 2u00 0v00 0u00]
		psrlq		mm4,40			;   [0000 0000 002v 002u]
		 psllq		mm7,24			;   [000v 000u 0000 0000]
		movq		mm6,mm4			;   [0000 0000 002v 002u]
		 por		mm1,mm7			; [000v 000u 00-2v 00-2u]	-- Left
		psllq		mm6,32			;   [002v 002u 0000 0000]
		por			mm6,mm4			; [002v 002u 002v 002u]		-- Right
		 psrlq		mm3,8			; [002v 002u 000v 000u]		-- Centre
		paddsw		mm1,mm6			; left += right
		 pmullw		mm3,cw			; *= center weight
		pmullw		mm1,ow			; *= outer weight
		 paddsw		mm0,mm5			; Weighted centres + outers
		paddsw		mm1,mm3			; Weighted centres + outers
		 paddsw		mm0,mm5			; Weighted centres + outers
		paddsw		mm1,mm3			; Weighted centres + outers
		 paddsw		mm0,r7			; += 0.5
		paddsw		mm1,r7			; += 0.5
		 psraw		mm0,7			; /= 32768
		psraw		mm1,7			; /= 32768
		 packuswb	mm0,mm0			; [3y 2y 1y 0y 3y 2y 1y 0y] -- Unsign Saturated
		packuswb	mm1,mm1			; [2v 2u 0v 0u 2v 2u 0v 0u] -- Unsign Saturated
		punpcklbw	mm0,mm1			; [2v 3y 2u 2y 0v 1y 0v 0y]
		movq		[ecx],mm0		; Update 4 centre pixels

next_loop:
		}
	p += pitch;
	}
	__asm emms
}
#endif
// --------------------------------------
// Blur/Sharpen Horizontal RGB24 C++ Code
// --------------------------------------

void AFH_RGB24_C(uc* p, int height, const int pitch, const int width, const int amount) {
  const int center_weight = amount*2;
  const int outer_weight = 32768-amount;
  for (int y = height; y>0; --y) 
    {

      uc bb = p[0];
      uc gg = p[1];
      uc rr = p[2];
      int x;
	  for (x = 0; x < width-1; ++x) 
      {
        const uc b = ScaledPixelClip(p[x*3+0] * center_weight + (bb + p[x*3+3]) * outer_weight);
        bb = p[x*3+0];
        p[x*3+0] = b;

        const uc g = ScaledPixelClip(p[x*3+1] * center_weight + (gg + p[x*3+4]) * outer_weight);
        gg = p[x*3+1];
        p[x*3+1] = g;

        const uc r = ScaledPixelClip(p[x*3+2] * center_weight + (rr + p[x*3+5]) * outer_weight);
        rr = p[x*3+2];
        p[x*3+2] = r;
      }
      p[x*3+0] = ScaledPixelClip(p[x*3+0] * center_weight + (bb + p[x*3+0]) * outer_weight);
      p[x*3+1] = ScaledPixelClip(p[x*3+1] * center_weight + (gg + p[x*3+1]) * outer_weight);
      p[x*3+2] = ScaledPixelClip(p[x*3+2] * center_weight + (rr + p[x*3+2]) * outer_weight);
      p += pitch;
    }
}

// -------------------------------------
// Blur/Sharpen Horizontal YV12 C++ Code
// -------------------------------------

void AFH_YV12_C(uc* p, int height, const int pitch, const int row_size, const int amount) 
{
	const int center_weight = amount*2;
	const int outer_weight = 32768-amount;

	for (int y = height; y>0; --y) {
		uc l = p[0];
		int x;
		for (x = 0; x < row_size-1; ++x) {
			const uc pp = ScaledPixelClip(p[x] * center_weight + (l + p[x+1]) * outer_weight);
			l=p[x];
            p[x]=pp;
		}
		p[x] = ScaledPixelClip(p[x] * center_weight + (l + p[x]) * outer_weight);
		p += pitch;
	}
}

// -------------------------------------
// Blur/Sharpen Horizontal YV12 MMX Code
// -------------------------------------

#define scale(mmAA,mmBB,mmCC,mmA,mmB,mmC,zeros)	\
__asm	movq		mmC,mmCC	/* Right 8 pixels      */\
__asm	 movq		mmA,mmAA	/* Left     "          */\
__asm	movq		mmB,mmBB	/* Centre   "          */\
__asm	 punpcklbw	mmC,zeros	/* Low 4 right         */\
__asm	punpcklbw	mmA,zeros	/* Low 4 left          */\
__asm	 punpcklbw	mmB,zeros	/* Low 4 centre        */\
__asm	paddsw		mmA,mmC		/* Low 4 left + right  */\
__asm	 pmullw		mmB,cw		/* *= centre weight    */\
__asm	pmullw		mmA,ow		/* *= outer weight     */\
__asm	 punpckhbw	mmCC,zeros	/* High 4 Right        */\
__asm	punpckhbw	mmAA,zeros	/* High 4 Left         */\
__asm	 punpckhbw	mmBB,zeros	/* High 4 Centre       */\
__asm	paddsw		mmAA,mmCC	/* High 4 left + right */\
__asm	 pmullw		mmBB,cw		/* *= centre weight    */\
__asm	pmullw		mmAA,ow		/* *= outer weight     */\
__asm	 paddsw		mmA,mmB		/* += weighed low 4    */\
__asm	paddsw		mmAA,mmBB	/* += weighed high 4   */\
__asm	 paddsw		mmA,mmB		/* += weighed low 4    */\
__asm	paddsw		mmAA,mmBB	/* += weighed high 4   */\
__asm	 paddsw		mmA,r7		/* += 0.5              */\
__asm	paddsw		mmAA,r7		/* += 0.5              */\
__asm	 psraw		mmA,7		/* /= 128              */\
__asm	psraw		mmAA,7		/* /= 128              */\
__asm	 add		ecx,8		/* p += 8              */\
__asm	packuswb	mmA,mmAA	/* Packed new 8 pixels */

// 
// Planer MMX blur/sharpen - process 8 pixels at a time
//   FillBorder::Create(_child)) ensures the right edge is repeated to mod 8 width
//   frame->GetRowSize(plane|PLANAR_ALIGNED) ensured rowsize is also mod 8
// For pitch less than 8 C code is used (unlikely, I couldn't force a case)
// 
#ifdef _M_IX86
void AFH_YV12_MMX(uc* p, int height, const int pitch, const int row_size, const int amount) 
{
	// weights
	__declspec(align(8)) __int64 cw;
	__declspec(align(8)) __int64 ow;
	// round masks
	__declspec(align(8)) const __int64 r7 = 0x0040004000400040;
	// edge masks
	__declspec(align(8)) const __int64 leftmask  = 0x00000000000000FF;
	__declspec(align(8)) const __int64 rightmask = 0xFF00000000000000;

	__asm { 
		// signed word center weight ((amount+0x100)>>9) x4
		mov		eax,amount
		add		eax,100h
		sar		eax,9
		lea		edx,cw
		mov		[edx],ax
		mov		[edx+2],ax
		mov		[edx+4],ax
		mov		[edx+6],ax
		// signed word outer weight 64-((amount+0x100)>>9) x4
		mov		ax,40h
		sub		ax,[edx]
		lea		edx,ow
		mov		[edx],ax
		mov		[edx+2],ax
		mov		[edx+4],ax
		mov		[edx+6],ax
	}

	for (int y=0;y<height;y++) {

	__asm {
		mov			ecx,p
		mov			edi,pitch
		add			edi,ecx
		mov			p,edi			; p += pitch;

		mov			edi,row_size
		test		edi,7
		jz			nopad
		mov			al,[ecx+edi-1]	; Pad edge pixel if needed
		mov			[ecx+edi],al
nopad:
		add			edi,7
		shr			edi,3

		pxor		mm0,mm0
; first row
		 movq		mm2,[ecx]		; Centre 8 pixels
		movq		mm1,leftmask	; 0x00000000000000ff
		 movq		mm3,mm2			; Duplicate for left
		pand		mm1,mm2			; Left edge pixel
		 psllq		mm3,8			; Left 7 other pixels
		dec			edi
		 por		mm1,mm3			; Left pixels, left most repeated
		jz			out_row_loop_ex	; 8 or less pixels per line

		movq		mm3,[ecx+1]		; Right 8 pixels

		scale(mm1,mm2,mm3,mm4,mm5,mm6,mm0)

		dec			edi
		jz			out_row_loop	; 9 to 16 pixels per line

		align 16
row_loop:
		movq		mm1,[ecx-1]		; Pickup left 8th before it's updated
		 movq		[ecx-8],mm4		; update current 8 pixels
		movq		mm2,[ecx]		; Centre 8 pixels
		 movq		mm3,[ecx+1]		; Right 8 pixels

		scale(mm1,mm2,mm3,mm4,mm5,mm6,mm0)

		dec			edi
		jnz			row_loop		; more ?

out_row_loop:
		movq		mm1,[ecx-1]		; Pickup left 8th before it's updated
		 movq		[ecx-8],mm4		; update current 8 pixels
		movq		mm2,[ecx]		; Centre 8 pixels
out_row_loop_ex:
		 movq		mm3,rightmask	; 0xFF00000000000000
		movq		mm4,mm2			; Duplicate for right
		 pand		mm3,mm2			; Right edge pixel
		psrlq		mm4,8			; Right 7 other pixels
		por			mm3,mm4			; Right pixles, right most repeated

		scale(mm1,mm2,mm3,mm4,mm5,mm6,mm0)

		movq		[ecx-8],mm4		; update current 8 pixels
		}
	}
	__asm emms
}
#endif



/************************************************
 *******   Sharpen/Blur Factory Methods   *******
 ***********************************************/

AVSValue __cdecl Create_Sharpen(AVSValue args, void*, IScriptEnvironment* env) 
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
  const double amountH = args[1].AsFloat(), amountV = args[2].AsDblDef(amountH);
  const bool mmx = args[3].AsBool(true) && (env->GetCPUFlags() & CPUF_MMX);

  if (amountH < -1.5849625 || amountH > 1.0 || amountV < -1.5849625 || amountV > 1.0) // log2(3)
    env->ThrowError("Sharpen: arguments must be in the range -1.58 to 1.0");

  if (fabs(amountH) < 0.00002201361136) { // log2(1+1/65536)
    if (fabs(amountV) < 0.00002201361136) {
      return args[0].AsClip();
    }
    else {
      return new AdjustFocusV(amountV, args[0].AsClip(), mmx);
    }
  }
  else {
    if (fabs(amountV) < 0.00002201361136) {
      return new AdjustFocusH(amountH, args[0].AsClip(), mmx);
    }
    else {
      return new AdjustFocusH(amountH, new AdjustFocusV(amountV, args[0].AsClip(), mmx), mmx);
    }
  }
	}
	catch (...) { throw; }
}

AVSValue __cdecl Create_Blur(AVSValue args, void*, IScriptEnvironment* env) 
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
  const double amountH = args[1].AsFloat(), amountV = args[2].AsDblDef(amountH);
  const bool mmx = args[3].AsBool(true) && (env->GetCPUFlags() & CPUF_MMX);

  if (amountH < -1.0 || amountH > 1.5849625 || amountV < -1.0 || amountV > 1.5849625) // log2(3)
    env->ThrowError("Blur: arguments must be in the range -1.0 to 1.58");

  if (fabs(amountH) < 0.00002201361136) { // log2(1+1/65536)
    if (fabs(amountV) < 0.00002201361136) {
      return args[0].AsClip();
    }
    else {
      return new AdjustFocusV(-amountV, args[0].AsClip(), mmx);
    }
  }
  else {
    if (fabs(amountV) < 0.00002201361136) {
      return new AdjustFocusH(-amountH, args[0].AsClip(), mmx);
    }
    else {
      return new AdjustFocusH(-amountH, new AdjustFocusV(-amountV, args[0].AsClip(), mmx), mmx);
    }
  }
	}
	catch (...) { throw; }
}


