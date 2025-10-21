


#include "avisynth/avisynth_stdafx.h"

#include "merge.h"

__declspec(align(8)) static const __int64 I1=0x00ff00ff00ff00ff;  // Luma mask
__declspec(align(8)) static const __int64 I2=0xff00ff00ff00ff00;  // Chroma mask
__declspec(align(8)) static const __int64 rounder = 0x0000400000004000;  // (0.5)<<15 in each dword


extern const AVSFunction Merge_filters[] = {
  { "Merge", "cc[weight]f", MergeAll::Create },  // src, src2, weight
  { 0 }
};
#define TEST(off, on)
#define TESTARG(n) 0



/*************************
******   Merge All   *****
*************************/


MergeAll::MergeAll(PClip _child, PClip _clip, float _weight, int _test, IScriptEnvironment* env)
  : GenericVideoFilter(_child,__FUNCTION__ ), clip(_clip), weight(_weight), test(_test)
{
  const VideoInfo& vi2 = clip->GetVideoInfo();

  if (!vi.IsSameColorspace(vi2))
    env->ThrowError("Merge: Pixel types are not the same. Both must be the same.");

  if (vi.width!=vi2.width || vi.height!=vi2.height)
    env->ThrowError("Merge: Images must have same width and height!");

  if (weight<0.0f) weight=0.0f;
  if (weight>1.0f) weight=1.0f;
}


PVideoFrame __stdcall MergeAll::GetFrame(int n, IScriptEnvironment* env)
{
  if (weight<0.0039f) return child->GetFrame(n, env);
  if (weight>0.9961f) return clip->GetFrame(n, env);

  PVideoFrame src  = child->GetFrame(n, env);
  if (src == nullptr || src.m_ptr == nullptr) {
      return nullptr;
  }
  PVideoFrame src2 =  clip->GetFrame(n, env);

  if (src2 == nullptr || src2.m_ptr == nullptr) {
      return nullptr;
  }
  env->MakeWritable(&src);
  BYTE* srcp  = (BYTE*)src->GetWritePtr();
  BYTE* srcp2 = (BYTE*)src2->GetReadPtr();

  const int src_pitch = src->GetPitch();
  const int src_rowsize = src->GetRowSize();
  int src_rowsize4 = (src_rowsize + 3) & -4;
  if (src_rowsize4 > src_pitch) src_rowsize4 = src_pitch;
  int src_rowsize8 = (src_rowsize + 7) & -8;
  if (src_rowsize8 > src_pitch) src_rowsize8 = src_pitch;

#ifdef _M_IX86
  if (TEST(16, 32) (env->GetCPUFlags() & CPUF_INTEGER_SSE) && ((src_rowsize4 & 3)==0) && (weight>0.4961f) && (weight<0.5039f)) {
    isse_avg_plane(srcp, srcp2, src_pitch, src2->GetPitch(), src_rowsize4, src->GetHeight());
  }
  else if (TEST(4, 8) (env->GetCPUFlags() & CPUF_MMX) && ((src_rowsize8 & 7)==0)) {
	const int iweight = (int)(weight*32767.0f);
	const int invweight = 32767-iweight;
    mmx_weigh_plane(srcp, srcp2, src_pitch, src2->GetPitch(), src_rowsize8, src->GetHeight(), iweight, invweight);
  }
  else
#endif
  {
	const int iweight = (int)(weight*65535.0f);
	const int invweight = 65535-iweight;
    weigh_plane(srcp, srcp2, src_pitch, src2->GetPitch(), src_rowsize, src->GetHeight(), iweight, invweight);
  }

  if (vi.IsPlanar()) {
    BYTE* srcpU  = (BYTE*)src->GetWritePtr(PLANAR_U);
    BYTE* srcpV  = (BYTE*)src->GetWritePtr(PLANAR_V);
    BYTE* srcp2U = (BYTE*)src2->GetReadPtr(PLANAR_U);
    BYTE* srcp2V = (BYTE*)src2->GetReadPtr(PLANAR_V);
 
    const int src_pitch = src->GetPitch(PLANAR_U);
    const int src_rowsize = src->GetRowSize(PLANAR_U);
    src_rowsize4 = (src_rowsize + 3) & -4;
    if (src_rowsize4 > src_pitch) src_rowsize4 = src_pitch;
    src_rowsize8 = (src_rowsize + 7) & -8;
    if (src_rowsize8 > src_pitch) src_rowsize8 = src_pitch;
#ifdef _M_IX86
    if ((TEST(4, 8) (env->GetCPUFlags() & CPUF_INTEGER_SSE) && ((src_rowsize4 & 3)==0)) && (weight>0.4961f) && (weight<0.5039f)) {
      isse_avg_plane(srcpV, srcp2V, src_pitch, src2->GetPitch(PLANAR_U), src_rowsize4, src->GetHeight(PLANAR_U));
      isse_avg_plane(srcpU, srcp2U, src_pitch, src2->GetPitch(PLANAR_V), src_rowsize4, src->GetHeight(PLANAR_V));
    }
    else if (TEST(4, 8) (env->GetCPUFlags() & CPUF_MMX) && ((src_rowsize8 & 7)==0)) {
      const int iweight = (int)(weight*32767.0f);
      const int invweight = 32767-iweight;
      mmx_weigh_plane(srcpV, srcp2V, src_pitch, src2->GetPitch(PLANAR_U), src_rowsize8, src->GetHeight(PLANAR_U), iweight, invweight);
      mmx_weigh_plane(srcpU, srcp2U, src_pitch, src2->GetPitch(PLANAR_V), src_rowsize8, src->GetHeight(PLANAR_V), iweight, invweight);
    }
    else 
#endif
    {
	  const int iweight = (int)(weight*65535.0f);
	  const int invweight = 65535-iweight;
      weigh_plane(srcpV, srcp2V, src_pitch, src2->GetPitch(PLANAR_U), src_rowsize, src->GetHeight(PLANAR_U), iweight, invweight);
      weigh_plane(srcpU, srcp2U, src_pitch, src2->GetPitch(PLANAR_V), src_rowsize, src->GetHeight(PLANAR_V), iweight, invweight);
    }
  }

  return src;
}


AVSValue __cdecl MergeAll::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  return new MergeAll(args[0].AsClip(), args[1].AsClip(), args[2].AsFloat(0.5f), TESTARG(3), env);
}



/****************************
******    C routines    *****
****************************/


void merge_luma(unsigned int *src, unsigned int *luma, int pitch, int luma_pitch,int width, int height ) {

  int lwidth=width>>1;

  for (int y=0;y<height;y++) {
    unsigned char *lum=(unsigned char*)luma;
    unsigned char *dst=(unsigned char*)src;
    for (int x=0;x<lwidth;x++) {
      dst[x*4]   = lum[x*4];
      dst[x*4+2] = lum[x*4+2];
    }
    src+=pitch;
    luma+=luma_pitch;
  } // end for y
}


void weigh_luma(unsigned int *src,unsigned int *luma, int pitch, int luma_pitch,int width, int height, int weight, int invweight) {

  int lwidth=width>>1;

  for (int y=0;y<height;y++) {
    unsigned char *lum=(unsigned char*)luma;
    unsigned char *dst=(unsigned char*)src;
    for (int x=0;x<lwidth;x++) {
      dst[x*4]   = (lum[x*4]   * weight + dst[x*4]   * invweight + 16384) >> 15;
      dst[x*4+2] = (lum[x*4+2] * weight + dst[x*4+2] * invweight + 16384) >> 15;
    }
    src+=pitch;
    luma+=luma_pitch;
  } // end for y
}


void weigh_chroma(unsigned int *src,unsigned int *chroma, int pitch, int chroma_pitch,int width, int height, int weight, int invweight) {

  int lwidth=width>>1;

  for (int y=0;y<height;y++) {
    unsigned char *chm=(unsigned char*)chroma;
    unsigned char *dst=(unsigned char*)src;
    for (int x=0;x<lwidth;x++) {
      dst[x*4+1] = (chm[x*4+1] * weight + dst[x*4+1] * invweight + 16384) >> 15;
      dst[x*4+3] = (chm[x*4+3] * weight + dst[x*4+3] * invweight + 16384) >> 15;
    }
    src+=pitch;
    chroma+=chroma_pitch;
  } // end for y
}


void weigh_plane(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch,int rowsize, int height, int weight, int invweight) {

  for (int y=0;y<height;y++) {
    for (int x=0;x<rowsize;x++) {
      p1[x] = (p1[x]*invweight + p2[x]*weight + 32768) >> 16;
    }
    p2+=p2_pitch;
    p1+=p1_pitch;
  }
}


#ifdef _M_IX86
void mmx_merge_luma( unsigned int *src, unsigned int *luma, int pitch,
                     int luma_pitch,int width, int height )
{
  // [V][Y2][U][Y1]

  int row_size = width * 2;
  int row_even = row_size & -8;
  int lwidth_bytes = row_size & -16;	// bytes handled by the MMX loop

  __asm {
    movq mm7,[I1]     ; Luma
    movq mm6,[I2]     ; Chroma
  }
  for (int y=0;y<height;y++) {

    // eax=src
    // ebx=luma
    // ecx=src/luma offset

  __asm {
    push ebx // bloody compiler forgets to save ebx!!
    mov eax,src
    xor ecx,ecx
    mov ebx,luma
    align 16
goloop:
    cmp       ecx,[lwidth_bytes]	; Is eax(i) greater than endx
    jge       outloop		; Jump out of loop if true

    ; Processes 8 pixels at the time
    movq mm0,[eax+ecx]		; chroma 4 pixels
     movq mm1,[eax+ecx+8]  ; chroma next 4 pixels
    pand mm0,mm6
     movq mm2,[ebx+ecx]  ; load luma 4 pixels
    pand mm1,mm6
     movq mm3,[ebx+ecx+8]  ; load luma next 4 pixels
    pand mm2,mm7
     pand mm3,mm7
    por mm0,mm2
     por mm1,mm3
    movq [eax+ecx],mm0
     movq [eax+ecx+8],mm1
    add ecx,16   // 16 bytes per pass = 8 pixels = 2 quadwords
     jmp goloop
outloop:
    ; processes remaining pixels pair
    cmp ecx,[row_even]
    jge outeven

    movq mm0,[eax+ecx]		; chroma 4 pixels
     movq mm2,[ebx+ecx]  ; load luma 4 pixels
    pand mm0,mm6
     pand mm2,mm7
    add ecx,8
     por mm0,mm2
     movq [eax+ecx-8],mm0
outeven:
    ; processes remaining pixel
    cmp ecx,[row_size]
    jge exitloop

    movd mm0,[eax+ecx]		; chroma 2 pixels
     movd mm2,[ebx+ecx]  ; load luma 2 pixels
    pand mm0,mm6
     pand mm2,mm7
     por mm0,mm2
     movd [eax+ecx],mm0
exitloop:
    pop ebx
    }

    src += pitch;
    luma += luma_pitch;
  } // end for y
  __asm {emms};
}




void mmx_weigh_luma(unsigned int *src,unsigned int *luma, int pitch,
                    int luma_pitch,int width, int height, int weight, int invweight)
{
  int row_size = width * 2;
  int lwidth_bytes = row_size & -8;	// bytes handled by the main loop
  // weight LLLL llll LLLL llll

  __asm {
		movq mm7,[I1]     ; Luma
		movq mm6,[I2]     ; Chroma
		movd mm5,[invweight]
		punpcklwd mm5,[weight]
		punpckldq mm5,mm5 ; Weight = invweight | (weight<<16) | (invweight<<32) | (weight<<48);
		movq mm4,[rounder]

  }
	for (int y=0;y<height;y++) {

		// eax=src
		// ebx=luma
		// ecx=src/luma offset

	__asm {
		push ebx // bloody compiler forgets to save ebx!!
		mov eax,src
		xor ecx,ecx
		mov ebx,luma
		cmp       ecx,[lwidth_bytes]	; Is eax(i) greater than endx
		jge       outloop		; Jump out of loop if true
		movq mm3,[eax+ecx]		; original 4 pixels   (cc)
		align 16
goloop:
		; Processes 4 pixels at the time
		 movq mm2,[ebx+ecx]    ; load 4 pixels
		movq mm1,mm3          ; move original pixel into mm3
		 punpckhwd mm3,mm2     ; Interleave upper pixels in mm3 | mm3= CCLL ccll CCLL ccll
		movq mm0,mm1
		 punpcklwd mm1,mm2     ; Interleave lower pixels in mm1 | mm1= CCLL ccll CCLL ccll
		pand mm3,mm7					; mm3= 00LL 00ll 00LL 00ll
		 pand mm1,mm7
		pmaddwd mm3,mm5				; Mult with weights and add. Latency 2 cycles - mult unit cannot be used
		 pand mm0,mm6					; mm0= cc00 cc00 cc00 cc00
		pmaddwd mm1,mm5
		 paddd mm3,mm4					; round to nearest
		paddd mm1,mm4					; round to nearest
		 psrld mm3,15					; Divide with total weight (=15bits) mm3 = 0000 00LL 0000 00LL
		psrld mm1,15					; Divide with total weight (=15bits) mm1 = 0000 00LL 0000 00LL
		 add ecx,8   // 8 bytes per pass = 4 pixels = 1 quadword
		packssdw mm1, mm3			; mm1 = 00LL 00LL 00LL 00LL
		 cmp ecx,[lwidth_bytes]	; Is eax(i) greater than endx
		por mm1,mm0
		 movq mm3,[eax+ecx]		; original 4 pixels   (cc)
		movq [eax+ecx-8],mm1
		 jnge goloop						; fall out of loop if true

outloop:
		// processes remaining pixels here
		cmp ecx,[row_size]
		jge	exitloop
		movd mm1,[eax+ecx]			; original 2 pixels
		 movd mm2,[ebx+ecx]			; luma 2 pixels
		movq mm0,mm1
		 punpcklwd mm1,mm2				; mm1= CCLL ccll CCLL ccll
		pand mm0,mm6						; mm0= 0000 0000 cc00 cc00
		 pand mm1,mm7						; mm1= 00LL 00ll 00LL 00ll
		 pmaddwd mm1,mm5
		 paddd mm1,mm4						; round to nearest
		 psrld mm1,15						; mm1= 0000 00LL 0000 00LL
		 packssdw mm1,mm1				; mm1= 00LL 00LL 00LL 00LL
		 por mm1,mm0							; mm0 finished
		 movd [eax+ecx],mm1
		// no loop since there is at most 2 remaining pixels
exitloop:
		pop ebx
		}
		src += pitch;
		luma += luma_pitch;
	} // end for y
  __asm {emms};
}




void mmx_weigh_chroma( unsigned int *src,unsigned int *chroma, int pitch,
                     int chroma_pitch,int width, int height, int weight, int invweight )
{

  int row_size = width * 2;
  int lwidth_bytes = row_size & -8;	// bytes handled by the main loop

  __asm {
		movq mm7,[I1]     ; Luma
		movd mm5,[invweight]
		punpcklwd mm5,[weight]
		punpckldq mm5,mm5 ; Weight = invweight | (weight<<16) | (invweight<<32) | (weight<<48);
		movq mm4,[rounder]

  }
	for (int y=0;y<height;y++) {

		// eax=src
		// ebx=luma
		// ecx=src/luma offset

	__asm {
		push ebx // bloody compiler forgets to save ebx!!
		mov eax,src
		xor ecx,ecx
		mov ebx,chroma
		cmp ecx,[lwidth_bytes]	; Is eax(i) greater than endx
		jge       outloop		; Jump out of loop if true
		; Processes 4 pixels at the time
		movq mm1,[eax+ecx]		; original 4 pixels   (cc)
		 movq mm2,[ebx+ecx]    ; load 4 pixels
		align 16
goloop:
		movq mm3,mm1
		 punpcklwd mm1,mm2     ; Interleave lower pixels in mm1 | mm1= CCLL ccll CCLL ccll
		movq mm0,mm3          ; move original pixel into mm3
		 psrlw mm1,8
		punpckhwd mm3,mm2     ; Interleave upper pixels in mm3 | mm3= CCLL ccll CCLL ccll
		 pmaddwd mm1,mm5
		psrlw mm3,8						; mm3= 00CC 00cc 00CC 00cc
		 paddd mm1,mm4					; round to nearest
		pmaddwd mm3,mm5				; Mult with weights and add. Latency 2 cycles - mult unit cannot be used
		 psrld mm1,15					; Divide with total weight (=15bits) mm1 = 0000 00CC 0000 00CC
		paddd mm3,mm4					; round to nearest
		 pand mm0,mm7					; mm0= 00ll 00ll 00ll 00ll
		psrld mm3,15					; Divide with total weight (=15bits) mm3 = 0000 00CC 0000 00CC
		 add ecx,8   // 8 bytes per pass = 4 pixels = 1 quadword
		packssdw mm1, mm3			; mm1 = 00CC 00CC 00CC 00CC
		 cmp ecx,[lwidth_bytes]	; Is eax(i) greater than endx
		psllw mm1,8
		 movq mm2,[ebx+ecx]    ; load 4 pixels
		por mm0,mm1
		 movq mm1,[eax+ecx]		; original 4 pixels   (cc)
		movq [eax+ecx-8],mm0
		 jnge goloop      ; fall out of loop if true

outloop:
		// processes remaining pixels here
		cmp ecx,[row_size]
		jge	exitloop
		movd mm0,[eax+ecx]			; original 2 pixels
		movd mm2,[ebx+ecx]			; luma 2 pixels
		movq mm1,mm0
		punpcklwd mm1,mm2				; mm1= CCLL ccll CCLL ccll
		psrlw mm1,8							; mm1= 00CC 00cc 00CC 00cc
		pmaddwd mm1,mm5
		pand mm0,mm7						; mm0= 0000 0000 00ll 00ll
		paddd mm1,mm4						; round to nearest
		psrld mm1,15						; mm1= 0000 00CC 0000 00CC
		packssdw mm1,mm1
		psllw mm1,8							; mm1= CC00 CC00 CC00 CC00
		por mm0,mm1							; mm0 finished
		movd [eax+ecx],mm0
		// no loop since there is at most 2 remaining pixels
exitloop:
		pop ebx
		}
		src += pitch;
		chroma += chroma_pitch;
	} // end for y
  __asm {emms};
}

void mmx_weigh_plane(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch,int rowsize, int height, int weight, int invweight) {

// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
  __asm {
      push ebx // bloody compiler forgets to save ebx!!
      movq       mm5,[rounder]
      pxor       mm6,mm6
      movd       mm7,[weight]
      punpcklwd  mm7,[invweight]
      punpckldq  mm7,mm7 ; Weight = weight | (invweight<<16) | (weight<<32) | (invweight<<48);
      mov        ebx,[rowsize]
      mov        esi,[p1]
      mov        edi,[p2]
      xor        ecx, ecx  // Height
      mov        edx,[height]
      test       ebx, ebx
      jz         outy
  
      align      16
yloopback:
      xor        eax, eax
      cmp        ecx, edx
      jge        outy

      align 16
testloop:
      movq        mm0,[edi+eax]  // y7y6 y5y4 y3y2 y1y0 img2
       movq       mm1,[esi+eax]  // Y7Y6 Y5Y4 Y3Y2 Y1Y0 IMG1
      movq        mm2,mm0
       punpcklbw  mm0,mm1        // Y3y3 Y2y2 Y1y1 Y0y0
      punpckhbw   mm2,mm1        // Y7y7 Y6y6 Y5y5 Y4y4
       movq       mm1,mm0        // Y3y3 Y2y2 Y1y1 Y0y0
      punpcklbw   mm0,mm6        // 00Y1 00y1 00Y0 00y0
       movq       mm3,mm2        // Y7y7 Y6y6 Y5y5 Y4y4
      pmaddwd     mm0,mm7        // Multiply pixels by weights.  pixel = img1*weight + img2*invweight (twice)
       punpckhbw  mm1,mm6        // 00Y3 00y3 00Y2 00y2
      paddd       mm0,mm5        // Add rounder
       pmaddwd    mm1,mm7        // Multiply pixels by weights.  pixel = img1*weight + img2*invweight (twice)
      punpcklbw   mm2,mm6        // 00Y5 00y5 00Y4 00y4
       paddd      mm1,mm5        // Add rounder                         
      psrld       mm0,15         // Shift down, so there is no fraction.
       pmaddwd    mm2,mm7        // Multiply pixels by weights.  pixel = img1*weight + img2*invweight (twice)
      punpckhbw   mm3,mm6        // 00Y7 00y7 00Y6 00y6
       paddd      mm2,mm5        // Add rounder
      pmaddwd     mm3,mm7        // Multiply pixels by weights.  pixel = img1*weight + img2*invweight (twice)
       psrld      mm1,15         // Shift down, so there is no fraction.
      paddd       mm3,mm5        // Add rounder                         
       psrld      mm2,15         // Shift down, so there is no fraction.
      psrld       mm3,15         // Shift down, so there is no fraction.
       packssdw   mm0,mm1        // 00Y3 00Y2 00Y1 00Y0
      packssdw    mm2,mm3        // 00Y7 00Y6 00Y5 00Y4
       add        eax,8
      packuswb    mm0,mm2        // Y7Y6 Y5Y4 Y3Y2 Y1Y0
       cmp        ebx, eax
      movq        [esi+eax-8],mm0
       jg         testloop

      inc         ecx
       add        esi,[p1_pitch];
      add         edi,[p2_pitch];
       jmp        yloopback
outy:
      emms
      pop ebx
  } // end asm
}


void isse_avg_plane(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch,int rowsize, int height) {

// mm0 mm1
  __asm {
      push ebx // bloody compiler forgets to save ebx!!
      mov        ebx,[rowsize]
      mov        esi,[p1]
      mov        edi,[p2]
      xor        ecx, ecx  // Height
      mov        edx,[height]
      test       ebx, ebx
      jz         outy
  
      align      16
yloopback:
      mov        eax, 16
      cmp        ecx, edx
      jge        outy

      cmp        ebx, eax
      jl         twelve
      align 16
testloop:
      movq        mm0,[edi+eax-16]  // y7y6 y5y4 y3y2 y1y0 img2
       movq       mm1,[edi+eax- 8]  // yFyE yDyC yByA y9y8 img2
      pavgb       mm0,[esi+eax-16]  // Y7Y6 Y5Y4 Y3Y2 Y1Y0 IMG1
       pavgb      mm1,[esi+eax- 8]  // YfYe YdYc YbYa Y9Y8 IMG1
      movq        [esi+eax-16],mm0
       movq       [esi+eax- 8],mm1
      add         eax,16
      cmp         ebx, eax
      jge         testloop
      align 16
twelve:
	  test        ebx, 8
	  jz          four
      movq        mm0,[edi+eax-16]  // y7y6 y5y4 y3y2 y1y0 img2
      pavgb       mm0,[esi+eax-16]  // Y7Y6 Y5Y4 Y3Y2 Y1Y0 IMG1
      movq        [esi+eax-16],mm0
      add         eax,8
      align 16
four:
	  test        ebx, 4
	  jz          zero
      movd        mm0,[edi+eax-16]  // ____ ____ y3y2 y1y0 img2
      movd        mm1,[esi+eax-16]  // ____ ____ Y3Y2 Y1Y0 IMG1
      pavgb       mm0,mm1
      movd        [esi+eax-16],mm0
      align 16
zero:
      inc         ecx
      add         esi,[p1_pitch];
      add         edi,[p2_pitch];
      jmp         yloopback
outy:
      emms
      pop ebx
  } // end asm
}
#endif