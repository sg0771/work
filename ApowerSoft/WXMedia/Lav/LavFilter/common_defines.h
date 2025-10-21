#pragma once

// Set minimal target OS (Vista+)
#ifdef _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#define _WIN32_WINNT 0x0600
#ifdef WINVER
#undef WINVER
#endif
#define WINVER _WIN32_WINNT

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#define VC_EXTRALEAN

#define LAV_AUDIO "LAV Audio Decoder"
#define LAV_VIDEO "LAV Video Decoder"
#define LAV_SPLITTER "LAV Splitter"
#include <algorithm>

#include <Windows.h>
#include <atlbase.h>
#include <atlsync.h>
#include <dshow.h>
#include <dvdmedia.h>
#include <strmif.h>

interface __declspec(uuid("8FBB906B-D1DB-4528-9498-563241CCD43D")) ILAVDynamicAllocator : public IUnknown
{
	// Query wether this allocator is using dynamic allocation of samples and will not run out of samples
	STDMETHOD_(BOOL, IsDynamicAllocator)() PURE;
};



interface __declspec(uuid("FD220BF4-3F26-4AD4-A4A9-348C1273A141")) ILAVPinInfo :
	public IUnknown
{
	// Get a set of flags that convey a special information for this kind of stream
	STDMETHOD_(DWORD, GetStreamFlags)() PURE;
#define LAV_STREAM_FLAG_ONLY_DTS  0x0000001 ///< Stream has only DTS timestamps (AVI, MKV in MS-Compat mode)
#define LAV_STREAM_FLAG_RV34_MKV  0x0000002 ///< RV30/40 in MKV or similar container with horrible timstamps
#define LAV_STREAM_FLAG_LIVE      0x0000004 ///< Stream is from a Live source

	// Get the pixel format detected for this video stream
	STDMETHOD_(int, GetPixelFormat)() PURE;

	// Get the interface version
	STDMETHOD_(int, GetVersion)() PURE;

	// Get the number of B-Frames probed for this format
	STDMETHOD_(int, GetHasBFrames)() PURE;
};


interface __declspec(uuid("E92D790E-BF54-43C4-B394-8CA0A41BF9EC")) IMediaSample3D : public IMediaSample2
{
	STDMETHOD(Enable3D)() = 0;
	STDMETHOD(GetPointer3D)(BYTE** ppBuffer) = 0;
};

// -----------------------------------------------------------------
// Interface to exchange binary side data
// -----------------------------------------------------------------
// This interface should be implemented in IMediaSample objects and accessed through IUnknown
// It allows binary side data to be attached to the media samples and delivered with them
// Restrictions: Only one side data per type can be attached
interface __declspec(uuid("F940AE7F-48EB-4377-806C-8FC48CAB2292")) IMediaSideData : public IUnknown
{
	// Set the side data identified by guidType to the data provided
	// The provided data will be copied and stored internally
	STDMETHOD(SetSideData)(GUID guidType, const BYTE* pData, size_t size) PURE;

	// Get the side data identified by guidType
	// The caller receives pointers to the internal data, and the pointers shall stay
	// valid for the lifetime of the object
	STDMETHOD(GetSideData)(GUID guidType, const BYTE** pData, size_t* pSize) PURE;
};


// -----------------------------------------------------------------
// High-Dynamic-Range (HDR) Side Data
// -----------------------------------------------------------------



#pragma pack(push, 1)
struct MediaSideDataHDR
{
	// coordinates of the primaries, in G-B-R order
	double display_primaries_x[3];
	double display_primaries_y[3];
	// white point
	double white_point_x;
	double white_point_y;
	// luminance
	double max_display_mastering_luminance;
	double min_display_mastering_luminance;
};
#pragma pack(pop)



#pragma pack(push, 1)
struct MediaSideDataHDRContentLightLevel
{
	// maximum content light level (cd/m2)
	unsigned int MaxCLL;

	// maximum frame average light level (cd/m2)
	unsigned int MaxFALL;
};
#pragma pack(pop)

// -----------------------------------------------------------------
// 3D Plane Offset Side Data
// -----------------------------------------------------------------

// {F169B76C-75A3-49E6-A23A-14983EBF4370}
DEFINE_GUID(IID_MediaSideData3DOffset,
	0xf169b76c, 0x75a3, 0x49e6, 0xa2, 0x3a, 0x14, 0x98, 0x3e, 0xbf, 0x43, 0x70);

#pragma pack(push, 1)
struct MediaSideData3DOffset
{
	// Number of valid offsets (up to 32)
	int offset_count;

	// Offset Value, can be positive or negative
	// positive values offset closer to the viewer (move right on the left view, left on the right view)
	// negative values offset further away from the viewer (move left on the left view, right on the right view)
	int offset[32];
};
#pragma pack(pop)


extern "C" {
#include "libavcodec/avcodec.h"
}

#pragma pack(push, 1)
struct MediaSideDataFFMpeg
{
	AVPacketSideData* side_data;
	int side_data_elems;
};
#pragma pack(pop)


interface ISubRenderConsumer;
interface ISubRenderProvider;
interface ISubRenderFrame;


DECLARE_INTERFACE_IID_(ISubRenderOptions, IUnknown, "7CFD3728-235E-4430-9A2D-9F25F426BD70")
{
	// Allows one party to get information from the other party.
	// The memory for strings and binary data is allocated by the callee
	// by using LocalAlloc. It is the caller's responsibility to release the
	// memory by calling LocalFree.
	// Field names and LPWSTR values should be read case insensitive.
	STDMETHOD(GetBool)(LPCSTR field, bool* value) = 0;
	STDMETHOD(GetInt)(LPCSTR field, int* value) = 0;
	STDMETHOD(GetSize)(LPCSTR field, SIZE * value) = 0;
	STDMETHOD(GetRect)(LPCSTR field, RECT * value) = 0;
	STDMETHOD(GetUlonglong)(LPCSTR field, ULONGLONG * value) = 0;
	STDMETHOD(GetDouble)(LPCSTR field, double* value) = 0;
	STDMETHOD(GetString)(LPCSTR field, LPWSTR * value, int* chars) = 0;
	STDMETHOD(GetBin)(LPCSTR field, LPVOID * value, int* size) = 0;

	// Allows one party to configure or send information to the other party.
	// The callee should copy the strings/binary data, if needed.
	// Field names and LPWSTR values should be set with the exact case listed
	// in this header (just to be safe).
	STDMETHOD(SetBool)(LPCSTR field, bool      value) = 0;
	STDMETHOD(SetInt)(LPCSTR field, int       value) = 0;
	STDMETHOD(SetSize)(LPCSTR field, SIZE      value) = 0;
	STDMETHOD(SetRect)(LPCSTR field, RECT      value) = 0;
	STDMETHOD(SetUlonglong)(LPCSTR field, ULONGLONG value) = 0;
	STDMETHOD(SetDouble)(LPCSTR field, double    value) = 0;
	STDMETHOD(SetString)(LPCSTR field, LPWSTR    value, int chars) = 0;
	STDMETHOD(SetBin)(LPCSTR field, LPVOID    value, int size) = 0;

	// "field" must be zero terminated

	// mandatory fields for consumers:
	// "name",                 LPWSTR,    info,   read only,  get name / description of the consumer
	// "version",              LPWSTR,    info,   read only,  get version number of the consumer
	// "originalVideoSize",    SIZE,      info,   read only,  size of the video before scaling and AR adjustments
	// "arAdjustedVideoSize",  SIZE,      info,   read only,  size of the video after AR adjustments
	// "videoOutputRect",      RECT,      info,   read only,  final pos/size of the video after all scaling operations
	// "subtitleTargetRect",   RECT,      info,   read only,  consumer wish for where to place the subtitles
	// "frameRate",            ULONGLONG, info,   read only,  frame rate of the video after deinterlacing (REFERENCE_TIME)
	// "refreshRate",          double,    info,   read only,  display refresh rate (0, if unknown)

	// mandatory fields for providers:
	// "name",                 LPWSTR,    info,   read only,  get name / description of the provider
	// "version",              LPWSTR,    info,   read only,  get version number of the provider
	// "yuvMatrix",            LPWSTR,    info,   read only,  RGB Subtitles: "None" (fullrange); YCbCr Subtitles: "Levels.Matrix", Levels: TV|PC, Matrix: 601|709|240M|FCC|2020
	// "combineBitmaps",       bool,      option, write/read, must the provider combine all bitmaps into one? (default: false)

	// optional fields for consumers:
	// "videoCropRect",        RECT,      info,   read only,  crops "originalVideoSize" down, e.g. because of detected black bars
	// "croppedVideoOutputRect", RECT,    info,   read only,  final pos/size of the "videoCropRect", after all scaling operations
	// "fullscreenRect",       RECT,      info,   read only,  for fullscreen drawing, this is the rect you want to stay in (left/top can be non-zero!)
	// "displayModeSize",      SIZE,      info,   read only,  display mode width/height
	// "yuvMatrix",            LPWSTR,    info,   read only,  RGB Video: "None" (fullrange); YCbCr Video: "Levels.Matrix", Levels: TV|PC, Matrix: 601|709|240M|FCC|2020
	// "supportedLevels",      int,       info,   read only,  0: PC only (default); 1: PC+TV, no preference; 2: PC+TV, PC preferred; 3: PC+TV, TV preferred

	// optional fields for providers:
	// "outputLevels",         LPWSTR,    info,   read only,  are subtitles rendered/output in RGB "PC" (default) or "TV" levels?
	// "isBitmap",             bool,      info,   read only,  are the subtitles bitmap based or text based?
	// "isMovable",            bool,      info,   read only,  can the subtitles be repositioned safely?
};

DECLARE_INTERFACE_IID_(ISubRenderConsumer, ISubRenderOptions, "9DF90966-FE9F-4F0E-881E-DAF8A572D900")
{
	// Called by the subtitle renderer to ask the merit of the consumer.
	// Recommended merits:
	// - Subtitle Manager     0x00080000
	// - Video Renderer       0x00040000
	// - Video Post Processor 0x00020000
	// - Video Decoder        0x00010000
	STDMETHOD(GetMerit)(ULONG * merit) = 0;

	// Called by the subtitle renderer to init the provider <-> consumer
	// connection. The subtitle renderer provides an "ISubRenderProvider"
	// interface for the consumer to store and use. The consumer should
	// call "AddRef()" to make sure that the interface instance stays alive
	// as long as needed.
	STDMETHOD(Connect)(ISubRenderProvider * subtitleRenderer) = 0;

	// Called by the subtitle renderer to close the connection. The
	// consumer should react by immediately "Release()"ing the stored
	// "ISubRenderProvider" instance.
	STDMETHOD(Disconnect)(void) = 0;

	// Called by the subtitle renderer to deliver a rendered subtitle frame
	// to the consumer. The renderer may only deliver frames which were
	// requested before by the consumer.
	// The frames will be delivered in the same order as they were requested.
	// The deliverance can occur in different threads than the request, though.
	// The subtitle renderer can deliver a "NULL" subtitle frame to indicate
	// that the specified frame has no visible subtitles. The subtitle renderer
	// can also reuse the same "ISubRenderFrame" instance for multiple video
	// frames, if the subtitles didn't change.
	// The consumer should "AddRef()" the "ISubRenderFrame", if the consumer
	// wants to use it after returning from "DeliverFrame()". If the consumer
	// does that, it also needs to call "Release()" later when the
	// "ISubRenderFrame" instance is no longer needed.
	// The subtitle renderer should not require the "ISubRenderFrame" instance
	// to be released immediately. The consumer may store it for buffering/queue
	// purposes. All properties of the "ISubRenderFrame" instance must return
	// the correct results until it is released by the consumer.
	STDMETHOD(DeliverFrame)(REFERENCE_TIME start, REFERENCE_TIME stop, LPVOID context, ISubRenderFrame * subtitleFrame) = 0;
};

//[uuid("1A1737C8-2BF8-4BEA-97EA-3AB4FA8F7AC9")]
//interface ISubRenderConsumer2 : public ISubRenderConsumer
DECLARE_INTERFACE_IID_(ISubRenderConsumer2, ISubRenderConsumer, "1A1737C8-2BF8-4BEA-97EA-3AB4FA8F7AC9")
{
	// Called by the subtitle renderer e.g. when the user switches to a
	// different subtitle track. The consumer should immediately release
	// all stored subtitle frames and request them anew from the subtitle
	// renderer.
	STDMETHOD(Clear)(REFERENCE_TIME clearNewerThan = 0) = 0;
};

// ---------------------------------------------------------------------------
// ISubRenderProvider
// ---------------------------------------------------------------------------

// The subtitle renderer provides the consumer with this interface, when
// calling the "ISubRenderConsumer.Connect()" method.

//[uuid("20752113-C883-455A-BA7B-ABA4E9115CA8")]
//interface ISubRenderProvider : public ISubRenderOptions
DECLARE_INTERFACE_IID_(ISubRenderProvider, ISubRenderOptions, "20752113-C883-455A-BA7B-ABA4E9115CA8")
{
	// Called by the consumer to request a rendered subtitle frame.
	// The subtitle renderer will deliver the frame when it is completed, by
	// calling "ISubRenderConsumer.DeliverFrame()".
	// The subtitle renderer must pass the "context" parameter to the
	// consumer when calling "DeliverFrame()".
	// Depending on the internal thread design of the subtitle renderer,
	// "RequestFrame()" can return at once, with delivery being performed
	// asynchronously in a different thread. Alternatively, "RequestFrame()"
	// may also block until the frame was delivered. The consumer should not
	// depend on either threading model, but leave this decision to the
	// subtitle renderer.
	STDMETHOD(RequestFrame)(REFERENCE_TIME start, REFERENCE_TIME stop, LPVOID context) = 0;

	// Called by the consumer to close the connection. The subtitle renderer
	// should react by immediately "Release()"ing any stored
	// "ISubRenderConsumer" interface instances pointing to this specific
	// consumer.
	STDMETHOD(Disconnect)(void) = 0;
};

// ---------------------------------------------------------------------------
// ISubRenderFrame
// ---------------------------------------------------------------------------

// This interface is the reply to a consumer's frame render request.

//[uuid("81746AB5-9407-4B43-A014-1FAAC340F973")]
//interface ISubRenderFrame : public IUnknown
DECLARE_INTERFACE_IID_(ISubRenderFrame, IUnknown, "81746AB5-9407-4B43-A014-1FAAC340F973")
{
	// "GetOutputRect()" specifies for which video rect the subtitles were
	// rendered. If the subtitle renderer doesn't scale the subtitles at all,
	// which is the recommended method for bitmap (DVD/PGS) subtitles formats,
	// GetOutputRect() should return "0, 0, originalVideoSize". If the subtitle
	// renderer scales the subtitles, which is the recommend method for text
	// (SRT, ASS) subtitle formats, GetOutputRect() should aim to match the
	// consumer's "videoOutputRect". In any case, the consumer can look at
	// GetOutputRect() to see if (and how) the rendered subtitles need to be
	// scaled before blending them onto the video image.
	STDMETHOD(GetOutputRect)(RECT * outputRect) = 0;

	// "GetClipRect()" specifies how the consumer should clip the rendered
	// subtitles, before blending them onto the video image. Usually,
	// GetClipRect() should be identical to "GetVideoOutputRect()", unless the
	// subtitle renderer repositioned the subtitles (see the top of this header
	// for more information about repositioning).
	STDMETHOD(GetClipRect)(RECT * clipRect) = 0;

	// How many separate bitmaps does this subtitle frame consist of?
	// The subtitle renderer should combine small subtitle elements which are
	// positioned near to each other, in order to optimize performance.
	// Ideally, if there are e.g. two subtitle lines, one at the top and one
	// at the bottom of the frame, the subtitle renderer should output two
	// bitmaps per frame.
	STDMETHOD(GetBitmapCount)(int* count) = 0;

	// Returns the premultiplied RGBA pixel data for the specified bitmap.
	// The ID is guaranteed to change if the content of the bitmap has changed.
	// The ID can stay identical if only the position changes.
	// Reusing the same ID for unchanged bitmaps can improve performance.
	// Subtitle bitmaps may move in and out of the video frame rectangle, so
	// the position of the subtitle bitmaps can become negative. The consumer
	// is required to do proper clipping if the subtitle bitmap is partially
	// outside the video rectangle.
	// The memory pointed to by "pixels" is only valid until the next
	// "GetBitmap" call, or until the "ISubRenderFrame" instance is released.
	STDMETHOD(GetBitmap)(int index, ULONGLONG * id, POINT * position, SIZE * size, LPCVOID * pixels, int* pitch) = 0;
};


