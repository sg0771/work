
#ifndef __AVISYNTH_6_H__
#define __AVISYNTH_6_H__

#include <windows.h>
#include <windef.h>
#include <string>
#include <exception>
#include <exception>
#ifdef _M_IX86
#define X86_32
#else
#define X86_64
#endif

#include "avisynth_c.h"

#define AVISYNTH_INTERFACE_VERSION 6
// Raster types used by VirtualDub & Avisynth
// 
//
typedef uint32_t  Pixel;    // this will break on 64-bit machines!


#undef _ASSERTE
#define _ASSERTE(x) //assert(x)

class IClip;
class __single_inheritance PClip;
class __single_inheritance PVideoFrame;
class IScriptEnvironment;
class __single_inheritance AVSValue;

enum {
	// Values 0 to 5 are reserved for old 2.5 plugins
	// do not use them in new plugins

	// New 2.6 explicitly defined cache hints.
	CACHE_NOTHING = 10, // Do not cache video.
	CACHE_WINDOW = 11, // Hard protect upto X frames within a range of X from the current frame N.
	CACHE_GENERIC = 12, // LRU cache upto X frames.
	CACHE_FORCE_GENERIC = 13, // LRU cache upto X frames, override any previous CACHE_WINDOW.

	CACHE_GET_POLICY = 30, // Get the current policy.
	CACHE_GET_WINDOW = 31, // Get the current window h_span.
	CACHE_GET_RANGE = 32, // Get the current generic frame range.

	CACHE_AUDIO = 50, // Explicitly cache audio, X byte cache.
	CACHE_AUDIO_NOTHING = 51, // Explicitly do not cache audio.
	CACHE_AUDIO_NONE = 52, // Audio cache off (auto mode), X byte intial cache.
	CACHE_AUDIO_AUTO = 53, // Audio cache on (auto mode), X byte intial cache.

	CACHE_GET_AUDIO_POLICY = 70, // Get the current audio policy.
	CACHE_GET_AUDIO_SIZE = 71, // Get the current audio cache size.

	CACHE_PREFETCH_FRAME = 100, // Queue request to prefetch frame N.
	CACHE_PREFETCH_GO = 101, // Action video prefetches.

	CACHE_PREFETCH_AUDIO_BEGIN = 120, // Begin queue request transaction to prefetch audio (take critical section).
	CACHE_PREFETCH_AUDIO_STARTLO = 121, // Set low 32 bits of start.
	CACHE_PREFETCH_AUDIO_STARTHI = 122, // Set high 32 bits of start.
	CACHE_PREFETCH_AUDIO_COUNT = 123, // Set low 32 bits of length.
	CACHE_PREFETCH_AUDIO_COMMIT = 124, // Enqueue request transaction to prefetch audio (release critical section).
	CACHE_PREFETCH_AUDIO_GO = 125, // Action audio prefetches.

	CACHE_GETCHILD_CACHE_MODE = 200, // Cache ask Child for desired video cache mode.
	CACHE_GETCHILD_CACHE_SIZE = 201, // Cache ask Child for desired video cache size.
	CACHE_GETCHILD_AUDIO_MODE = 202, // Cache ask Child for desired audio cache mode.
	CACHE_GETCHILD_AUDIO_SIZE = 203, // Cache ask Child for desired audio cache size.

	CACHE_GETCHILD_COST = 220, // Cache ask Child for estimated processing cost.
	CACHE_COST_ZERO = 221, // Child response of zero cost (ptr arithmetic only).
	CACHE_COST_UNIT = 222, // Child response of unit cost (less than or equal 1 full frame blit).
	CACHE_COST_LOW = 223, // Child response of light cost. (Fast)
	CACHE_COST_MED = 224, // Child response of medium cost. (Real time)
	CACHE_COST_HI = 225, // Child response of heavy cost. (Slow)

	CACHE_GETCHILD_THREAD_MODE = 240, // Cache ask Child for thread safetyness.
	CACHE_THREAD_UNSAFE = 241, // Only 1 thread allowed for all instances. 2.5 filters default!
	CACHE_THREAD_CLASS = 242, // Only 1 thread allowed for each instance. 2.6 filters default!
	CACHE_THREAD_SAFE = 243, //  Allow all threads in any instance.
	CACHE_THREAD_OWN = 244, // Safe but limit to 1 thread, internally threaded.

	CACHE_GETCHILD_ACCESS_COST = 260, // Cache ask Child for preferred access pattern.
	CACHE_ACCESS_RAND = 261, // Filter is access order agnostic.
	CACHE_ACCESS_SEQ0 = 262, // Filter prefers sequential access (low cost)
	CACHE_ACCESS_SEQ1 = 263, // Filter needs sequential access (high cost)

};

class AVSValue;

// Base class for all filters.
#define MAIN_VIDEO     0   //主轨道/转场
#define MASK_VIDEO     1   //动画的Mask
#define SUB_VIDEO      2   //包括画中画/动画，从2 开始 , 最大值6 蒙版应该是图像，不使用
#define MAX_CHANNEL    10   //最大轨道数量
class IClip {
	friend class PClip;
	friend class AVSValue;
	volatile long refcnt = 0;
	void AddRef() { InterlockedIncrement(&refcnt); }
	void Release() { if (!InterlockedDecrement(&refcnt)) delete this; }

	int m_id = -1;//标记视频在哪个轨道上
	std::string m_strName = "IClip";
public:
	void SetID(int id) {
		m_id = id;
	}
	int GetID() {
		return m_id;
	}
	IClip(const char* name);
	virtual int __stdcall GetVersion() { return AVISYNTH_INTERFACE_VERSION; }
	virtual PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) = 0;
	virtual bool __stdcall GetParity(int n) = 0;  // return field parity if field_based, else parity of first field in frame
	virtual void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) = 0;  // start and count are in samples
	/* Need to check GetVersion first, pre v5 will return random crap from EAX reg. */
	virtual intptr_t __stdcall SetCacheHints(int cachehints, int frame_range) = 0;  // We do not pass cache requests upwards, only to the next filter.
	virtual const VideoInfo& __stdcall GetVideoInfo() = 0;
	//virtual void __stdcall Invoke(const char* name,  void * args) { return ; };
	virtual __stdcall ~IClip();
}; // end class IClip

// smart pointer to IClip
class PClip {
	IClip* GetPointerWithAddRef() const { if (p) p->AddRef(); return p; }

	friend class AVSValue;
	friend struct VideoFrame;

	void Init(IClip* x) {
		if (x) x->AddRef();
		p = x;
	}

	void Set(IClip* x) {
		if (x) x->AddRef();
		if (p) p->Release();
		p = x;
	}

public:
	IClip* p = NULL;

	PClip() { CONSTRUCTOR0(); }
	PClip(const PClip& x) { CONSTRUCTOR1(x); }
	PClip(IClip* x) { CONSTRUCTOR2(x); }

	void operator=(IClip* x) { OPERATOR_ASSIGN0(x); }
	void operator=(const PClip& x) { OPERATOR_ASSIGN1(x); }

	IClip* operator->() const { return p; }

	// useful in conditional expressions
	operator void* () const { return p; }
	bool operator!() const { return !p; }

	~PClip() { DESTRUCTOR(); }
public:
	void CONSTRUCTOR0() { p = 0; }
	void CONSTRUCTOR1(const PClip& x) { Init(x.p); }
	void CONSTRUCTOR2(IClip* x) { Init(x); }
	void OPERATOR_ASSIGN0(IClip* x) { Set(x); }
	void OPERATOR_ASSIGN1(const PClip& x) { Set(x.p); }
	void DESTRUCTOR() { if (p) p->Release(); }
}; // end class PClip

class PVideoFrame {
	void Init(VideoFrame* x) {
		if (x) x->AddRef();
		m_ptr = x;
	}

	void Set(VideoFrame* x) {
		if (x) x->AddRef();
		if (m_ptr) m_ptr->Release();
		m_ptr = x;
	}
public:
	VideoFrame* m_ptr = NULL;

	PVideoFrame() { CONSTRUCTOR0(); }
	PVideoFrame(const PVideoFrame& x) { CONSTRUCTOR1(x); }
	PVideoFrame(VideoFrame* x) { CONSTRUCTOR2(x); }

	void operator=(VideoFrame* x) { OPERATOR_ASSIGN0(x); }
	void operator=(const PVideoFrame& x) { OPERATOR_ASSIGN1(x); }

	VideoFrame* operator->() const { return m_ptr; }

	// for conditional expressions
	operator void* () const {
		return m_ptr;
	}
	bool operator!() const { return !m_ptr; }

	~PVideoFrame() { DESTRUCTOR(); }

public:
	void CONSTRUCTOR0() { m_ptr = 0; }
	void CONSTRUCTOR1(const PVideoFrame& x) { Init(x.m_ptr); }
	void CONSTRUCTOR2(VideoFrame* x) { Init(x); }
	void OPERATOR_ASSIGN0(VideoFrame* x) { Set(x); }
	void OPERATOR_ASSIGN1(const PVideoFrame& x) { Set(x.m_ptr); }
	void DESTRUCTOR() { if (m_ptr)m_ptr->Release(); }
}; // end class PVideoFrame

class AVSValue {
public:
	union {
		IClip* clip;
		bool boolean;
		intptr_t integer;
		float floating_pt;
		const char* string;
		const AVSValue* array;
	};
	char  type;  // 'a'rray, 'c'lip, 'b'ool, 'i'nt, 'f'loat, 's'tring, 'v'oid, or 'p'tr
	short array_size;
public:

	// class AVSValue

	AVSValue() { CONSTRUCTOR0(); }
	void CONSTRUCTOR0() { type = 'v'; }

	AVSValue(IClip* c) { CONSTRUCTOR1(c); }
	void CONSTRUCTOR1(IClip* c) { type = 'c'; clip = c; if (c) c->AddRef(); }

	AVSValue(const PClip& c) { CONSTRUCTOR2(c); }
	void CONSTRUCTOR2(const PClip& c) { type = 'c'; clip = c.GetPointerWithAddRef(); }

	AVSValue(bool b) { CONSTRUCTOR3(b); }
	void CONSTRUCTOR3(bool b) { type = 'b'; boolean = b; }

	AVSValue(int i) { CONSTRUCTOR4(i); }
	void CONSTRUCTOR4(int i) { type = 'i'; integer = i; }

	AVSValue(float f) { CONSTRUCTOR5(f); }
	void CONSTRUCTOR5(float f) { type = 'f'; floating_pt = f; }

	AVSValue(double f) { CONSTRUCTOR6(f); }
	void CONSTRUCTOR6(double f) { type = 'f'; floating_pt = float(f); }

	AVSValue(const char* s) { CONSTRUCTOR7(s); }
	void CONSTRUCTOR7(const char* s) { type = 's'; string = s; }

	AVSValue(const AVSValue* a, int size) { CONSTRUCTOR8(a, size); }
	void CONSTRUCTOR8(const AVSValue* a, int size) {
		type = 'a';
		array = a;
		array_size = (short)size;
	}

	AVSValue(const AVSValue& v) { CONSTRUCTOR9(v); }
	void CONSTRUCTOR9(const AVSValue& v) { Assign(&v, true); }

	AVSValue(void* v) { CONSTRUCTOR10(v); }
	void CONSTRUCTOR10(void* v) {
		type = 'p';
		integer = (intptr_t)v;
	}

	~AVSValue() { DESTRUCTOR(); }
	void DESTRUCTOR() { if (IsClip() && clip) clip->Release(); }

	AVSValue& operator=(const AVSValue& v) { return OPERATOR_ASSIGN(v); }
	AVSValue& OPERATOR_ASSIGN(const AVSValue& v) { Assign(&v, false); return *this; }

	// Note that we transparently allow 'int' to be treated as 'float'.
	// There are no int<->bool conversions, though.

	bool Defined() const { return type != 'v'; }
	bool IsClip() const { return type == 'c'; }
	bool IsBool() const { return type == 'b'; }
	bool IsInt() const { return type == 'i'; }
	bool IsPtr() const { return type == 'p'; }
	bool IsFloat() const { return type == 'f' || type == 'i'; }
	bool IsString() const { return type == 's'; }
	bool IsArray() const { return type == 'a'; }

	PClip AsClip() const { _ASSERTE(IsClip()); return IsClip() ? clip : 0; }

	bool AsBool1() const { _ASSERTE(IsBool()); return boolean; }
	bool AsBool() const { return AsBool1(); }

	intptr_t AsPtr() const { _ASSERTE(IsPtr()); return integer; }//返回指针值

	int AsInt()      const { _ASSERTE(IsInt()); return integer; }//返回Int值


	const char* AsString1() const { _ASSERTE(IsString()); return IsString() ? string : 0; }
	const char* AsString() const { return AVSValue::AsString1(); }


	/* Baked ********************
	double AVSValue::AsFloat() const { _ASSERTE(IsFloat()); return IsInt()?integer:floating_pt; }
	   Baked ********************/

	double AsFloat1() const { _ASSERTE(IsFloat()); return IsInt() ? integer : floating_pt; }
	double AsFloat() const { return AsFloat1(); }


	bool AsBool2(bool def) const { _ASSERTE(IsBool() || !Defined()); return IsBool() ? boolean : def; }
	bool AsBool(bool def) const { return AsBool2(def); }

	int AsInt2(int def) const { _ASSERTE(IsInt() || !Defined()); return IsInt() ? integer : def; }
	int AsInt(int def) const { return AsInt2(def); }
	/* Baked ********************
	double AsFloat(double def) const { _ASSERTE(IsFloat()||!Defined()); return IsInt() ? integer : type=='f' ? floating_pt : def; }
	   Baked ********************/
	double AsDblDef(double def) const { _ASSERTE(IsFloat() || !Defined()); return IsInt() ? integer : type == 'f' ? floating_pt : def; }
	//float  AsFloat(double def) const { _ASSERTE(IsFloat()||!Defined()); return IsInt() ? integer : type=='f' ? floating_pt : (float)def; }

	double AsFloat2(float def) const { _ASSERTE(IsFloat() || !Defined()); return IsInt() ? integer : type == 'f' ? floating_pt : def; }
	double AsFloat(float def) const { return AsFloat2(def); }

	const char* AsString2(const char* def) const { _ASSERTE(IsString() || !Defined()); return IsString() ? string : def; }
	const char* AsString(const char* def) const { return AVSValue::AsString2(def); }

	int ArraySize() const { _ASSERTE(IsArray()); return IsArray() ? array_size : 1; }

	const AVSValue& operator[](int index) const { return OPERATOR_INDEX(index); }
	const AVSValue& OPERATOR_INDEX(int index) const {
		_ASSERTE(IsArray() && index >= 0 && index < array_size);
		return (IsArray() && index >= 0 && index < array_size) ? array[index] : *this;
	}

	void Assign(const AVSValue* src, bool init) {
		if (src->IsClip() && src->clip)
			src->clip->AddRef();
		if (!init && IsClip() && clip)
			clip->Release();
		this->integer = src->integer;
		this->type = src->type;
		this->array_size = src->array_size;
	}
}; // end class AVSValue

// For GetCPUFlags.  These are backwards-compatible with those in VirtualDub.
enum {
	/* oldest CPU to support extension */
	CPUF_FORCE = 0x01,   //  N/A
	CPUF_FPU = 0x02,   //  386/486DX
	CPUF_MMX = 0x04,   //  P55C, K6, PII
	CPUF_INTEGER_SSE = 0x08,   //  PIII, Athlon
	CPUF_SSE = 0x10,   //  PIII, Athlon XP/MP
	CPUF_SSE2 = 0x20,   //  PIV, K8
	CPUF_3DNOW = 0x40,   //  K6-2
	CPUF_3DNOW_EXT = 0x80,   //  Athlon
	CPUF_X86_64 = 0xA0,   //  Hammer (note: equiv. to 3DNow + SSE2, which
	CPUF_SSE3 = 0x100,   //  PIV+, K8 Venice
	CPUF_SSSE3 = 0x200,   //  Core 2
	CPUF_SSE4 = 0x400,   //  Penryn, Wolfdale, Yorkfield
	CPUF_SSE4_1 = 0x400,
	//CPUF_AVX          =  0x800,   //  Sandy Bridge, Bulldozer
	CPUF_SSE4_2 = 0x1000,   //  Nehalem
	//CPUF_AVX2         = 0x2000,   //  Haswell
	//CPUF_AVX512       = 0x4000,   //  Knights Landing
};

struct TimelineInfo
{
	float FrameRate;
	int	FrameWidth;
	int	FrameHeight;
	int	SampleRate;
	int	channels;
	bool ispreview;
	std::string m_IndexDir;
	//std::string m_TransDir;
};

class IScriptEnvironment {
public:
	virtual __stdcall ~IScriptEnvironment() {}

	virtual /*static*/ long __stdcall GetCPUFlags() = 0;

	virtual char* __stdcall SaveString(const char* s, int length = -1) = 0;
	virtual char* __stdcall Sprintf(const char* fmt, ...) = 0;
	// note: val is really a va_list; I hope everyone typedefs va_list to a pointer
	virtual char* __stdcall VSprintf(const char* fmt, void* val) = 0;

	__declspec(noreturn) virtual void __stdcall ThrowError(const char* fmt, ...) = 0;

	class NotFound /*exception*/ {};  // thrown by Invoke and GetVar

	typedef AVSValue(__cdecl* ApplyFunc)(AVSValue args, void* user_data, IScriptEnvironment* env);

	virtual void __stdcall AddFunction(const char* name, const char* params, ApplyFunc apply, void* user_data) = 0;
	virtual bool __stdcall FunctionExists(const char* name) = 0;
	virtual AVSValue __stdcall Invoke(const char* name, const AVSValue args, const char* const* arg_names = 0) = 0;
	//virtual AVSValue __stdcall SimpleInvoke(const char* name, const AVSValue args) = 0;

	//virtual AVSValue __stdcall InvokeCache(const AVSValue args, const char* const* arg_names = 0)=0;

	virtual AVSValue __stdcall GetVar(const char* name) = 0;
	virtual bool __stdcall SetVar(const char* name, const AVSValue& val) = 0;
	virtual bool __stdcall SetGlobalVar(const char* name, const AVSValue& val) = 0;

	virtual void __stdcall PushContext(int level = 0) = 0;
	virtual void __stdcall PopContext() = 0;

	virtual PVideoFrame __stdcall NewVideoFrame(const VideoInfo& vi, int align = FRAME_ALIGN) = 0;
	virtual bool __stdcall MakeWritable(PVideoFrame* pvf) = 0;

	virtual void __stdcall BitBlt(BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height) = 0;


	virtual void __stdcall CheckVersion(int version = AVISYNTH_INTERFACE_VERSION) = 0;

	virtual PVideoFrame __stdcall Subframe(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size, int new_height) = 0;

	virtual int __stdcall SetMemoryMax(int mem) = 0;

	virtual void* __stdcall ManageCache(int key, void* data) = 0;

	enum PlanarChromaAlignmentMode {
		PlanarChromaAlignmentOff,
		PlanarChromaAlignmentOn,
		PlanarChromaAlignmentTest
	};

	virtual bool __stdcall PlanarChromaAlignment(PlanarChromaAlignmentMode key) = 0;

	virtual PVideoFrame __stdcall SubframePlanar(PVideoFrame src, int rel_offset, int new_pitch, int new_row_size,
		int new_height, int rel_offsetU, int rel_offsetV, int new_pitchUV) = 0;

	virtual void __stdcall DeleteScriptEnvironment() = 0;

	// noThrow version of GetVar
	virtual AVSValue __stdcall GetVarDef(const char* name, const AVSValue& def = AVSValue()) = 0;

	virtual void __stdcall Reset() = 0;
	virtual void __stdcall Clear() = 0;

	virtual  TimelineInfo* __stdcall GetTimelineInfo() = 0;
	virtual void __stdcall  SetTimelineInfo(TimelineInfo info) = 0;

}; // end class IScriptEnvironment

class GenericVideoFilter : public IClip {
protected:
	PClip child;
	AVS_VideoInfo vi;
public:
	virtual ~GenericVideoFilter() {}
	GenericVideoFilter(PClip _child, const char* name) :
		IClip(name), child(_child) {
		int id = _child->GetID();
		this->SetID(id);
		vi = child->GetVideoInfo();
	}
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) {
		PVideoFrame frame = child->GetFrame(n, env);
		return frame;
	}
	void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) { child->GetAudio(buf, start, count, env); }
	const VideoInfo& __stdcall GetVideoInfo() { return vi; }
	bool __stdcall GetParity(int n) { return child->GetParity(n); }
	intptr_t __stdcall SetCacheHints(int cachehints, int frame_range) { return 0; };  // We do not pass cache requests upwards, only to the next filter.
};

// avisynth.dll exports this; it's a way to use it as a library, without
// writing an AVS script or without going through AVIFile.
EXTERN_C void* __stdcall CreateScriptEnvironment(int version/* = AVISYNTH_INTERFACE_VERSION*/);

static AVSValue AVSValueArray1(const std::initializer_list<AVSValue>& values) {
	return AVSValue(data(values), (int)values.size());
}



#endif //__AVISYNTH_6_H__
