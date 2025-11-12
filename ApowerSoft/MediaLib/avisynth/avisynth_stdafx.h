
#ifndef __Avisynth_Stdafx_H__
#define __Avisynth_Stdafx_H__

#ifndef _CRTIMP
#define _CRTIMP
#endif

//C
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include <string.h>
#include <io.h>
#include <ctype.h>
#include <wtypes.h>

//windows
#include <objbase.h>
#include <vfw.h>
#include <windows.h>
#include <mmsystem.h>
#include <msacm.h>
#include <shlobj.h>

//STL
#include <vector>
#include <algorithm>

#include <wxlog.h>


extern "C" {
#include "libavutil/mem.h"
}

#include <libyuv/libyuv.h>

// Tells if a number is a power of two.
#define IS_POWER2(n) ((n) && !((n) & ((n) - 1)))

// Tells if the pointer "ptr" is aligned to "align" bytes.
#define IS_PTR_ALIGNED(ptr, align) (((uintptr_t)ptr & ((uintptr_t)(align-1))) == 0)

// Rounds up the number "n" to the next greater multiple of "align"
#define ALIGN_NUMBER(n, align) (((n) + (align)-1) & (~((align)-1)))

// Rounds up the pointer address "ptr" to the next greater multiple of "align"
#define ALIGN_POINTER(ptr, align) (((uintptr_t)(ptr) + (align)-1) & (~(uintptr_t)((align)-1)))


#include <cassert>
#include <cstdlib>

template<typename T>
static bool IsPtrAligned(T* ptr, size_t align)
{
    assert(IS_POWER2(align));
    return (bool)IS_PTR_ALIGNED(ptr, align);
}

enum { MC_ReturnVideoFrameBuffer = 0xFFFF0001 };
enum { MC_ManageVideoFrameBuffer = 0xFFFF0002 };
enum { MC_PromoteVideoFrameBuffer = 0xFFFF0003 };
enum { MC_RegisterCache = 0xFFFF0004 };
enum { MC_IncVFBRefcount = 0xFFFF0005 };

#include "avisynth.h"

struct AVSFunction {
    const char* name;
    const char* param_types;
    AVSValue(__cdecl* apply)(AVSValue args, void* user_data, IScriptEnvironment* env);
    void* user_data;
};


int RGB2YUV(int rgb);

PClip new_Splice(PClip _child1, PClip _child2, bool realign_sound, IScriptEnvironment* env);

void BitBlt(BYTE* dstp, int dst_pitch, const BYTE* srcp,
    int src_pitch, int row_size, int height);

void asm_BitBlt_ISSE(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height);


long GetCPUFlags();


class _PixelClip {
    enum { buffer = 320 };
    BYTE clip[256 + buffer * 2];
public:
    _PixelClip() {
        memset(clip, 0, buffer);
        for (int i = 0; i < 256; ++i) clip[i + buffer] = (BYTE)i;
        memset(clip + buffer + 256, 255, buffer);
    }
    BYTE operator()(int i) { return clip[i + buffer]; }
};

extern _PixelClip PixelClip;


template<class ListNode>
static __inline void Relink(ListNode* newprev, ListNode* me, ListNode* newnext) {
    if (me == newprev || me == newnext) return;
    me->next->prev = me->prev;
    me->prev->next = me->next;
    me->prev = newprev;
    me->next = newnext;
    me->prev->next = me->next->prev = me;
}



/*** Inline helper methods ***/


static __inline BYTE ScaledPixelClip(int i) {
    return PixelClip((i + 32768) >> 16);
}


static __inline bool IsClose(int a, int b, unsigned threshold)
{
    return (unsigned(a - b + threshold) <= threshold * 2);
}




#endif // __Stdafx_H__
