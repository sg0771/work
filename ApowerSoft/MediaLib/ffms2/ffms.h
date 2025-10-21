#ifndef FFMS_H
#define FFMS_H

// Version format: major - minor - micro - bump
#define FFMS_VERSION ((2 << 24) | (30 << 16) | (0 << 8) | 0)
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "MediaLibAPI.h"

#define SUPPORTMI 1

#define CACHEVIDEO1
#ifdef CACHEVIDEO1
typedef struct VideoBlock
{
	int64_t frameindex;
	void* frame;
}VideoBlock;
#endif // CACHEVIDEO


// Convenience for C++ users.
#if defined(__cplusplus)
#	define FFMS_EXTERN_C extern "C"
#else
#	define FFMS_EXTERN_C
#endif

// On win32, we need to ensure we use stdcall with all compilers.
#if defined(_WIN32) && !defined(_WIN64)
#	define FFMS_CC __stdcall
#else
#	define FFMS_CC
#endif

// compiler-specific deprecation attributes
#ifndef FFMS_EXPORTS
#	if defined(__GNUC__) && (__GNUC__ >= 4)
#		define FFMS_DEPRECATED __attribute__((deprecated))
#	elif defined(_MSC_VER)
#		define FFMS_DEPRECATED __declspec(deprecated)
#	endif
#endif

#ifndef FFMS_DEPRECATED
// Define as empty if the compiler doesn't support deprecation attributes or
// if we're building FFMS2 itself
#	define FFMS_DEPRECATED
#endif

// And now for some symbol hide-and-seek...
#if defined(_WIN32) && !defined(FFMS_STATIC) // MSVC
#	if defined(FFMS_EXPORTS) // building the FFMS2 library itself, with visible API symbols
#		define FFMS_API(ret) FFMS_EXTERN_C ret FFMS_CC
#		define FFMS_DEPRECATED_API(ret) FFMS_EXTERN_C FFMS_DEPRECATED ret FFMS_CC
#	else // building something that depends on FFMS2
#		define FFMS_API(ret) FFMS_EXTERN_C /*__declspec(dllimport)*/ ret FFMS_CC
#		define FFMS_DEPRECATED_API(ret) FFMS_EXTERN_C FFMS_DEPRECATED /*__declspec(dllimport)*/ ret FFMS_CC
#	endif // defined(FFMS_EXPORTS)
// GCC 4 or later: export API symbols only. Some GCC 3.x versions support the visibility attribute too,
// but we don't really care enough about that to add compatibility defines for it.
#elif defined(__GNUC__) && __GNUC__ >= 4
#	define FFMS_API(ret) FFMS_EXTERN_C __attribute__((visibility("default"))) ret FFMS_CC
#	define FFMS_DEPRECATED_API(ret) FFMS_EXTERN_C FFMS_DEPRECATED __attribute__((visibility("default"))) ret FFMS_CC
#else // fallback for everything else
#	define FFMS_API(ret) FFMS_EXTERN_C ret FFMS_CC
#	define FFMS_DEPRECATED_API(ret) FFMS_EXTERN_C FFMS_DEPRECATED ret FFMS_CC
#endif // defined(_MSC_VER)


// we now return you to your regularly scheduled programming.



typedef struct FFMS_ErrorInfo {
    int ErrorType;
    int SubType;
    int BufferSize;
    char *Buffer;
} FFMS_ErrorInfo;



typedef struct FFMS_VideoSource FFMS_VideoSource;
typedef struct FFMS_AudioSource FFMS_AudioSource;
typedef struct FFMS_Indexer FFMS_Indexer;
typedef struct FFMS_Index FFMS_Index;
typedef struct FFMS_Track FFMS_Track;

typedef enum FFMS_Errors {
    // No error
    FFMS_ERROR_SUCCESS = 0,

    // Main types - where the error occurred
    FFMS_ERROR_INDEX = 1,			// index file handling
    FFMS_ERROR_INDEXING,			// indexing
    FFMS_ERROR_POSTPROCESSING,		// video postprocessing (libpostproc)
    FFMS_ERROR_SCALING,				// image scaling (libswscale)
    FFMS_ERROR_DECODING,			// audio/video decoding
    FFMS_ERROR_SEEKING,				// seeking
    FFMS_ERROR_PARSER,				// file parsing
    FFMS_ERROR_TRACK,				// track handling
    FFMS_ERROR_WAVE_WRITER,			// WAVE64 file writer
    FFMS_ERROR_CANCELLED,			// operation aborted
    FFMS_ERROR_RESAMPLING,			// audio resampling (libavresample)

    // Subtypes - what caused the error
    FFMS_ERROR_UNKNOWN = 20,		// unknown error
    FFMS_ERROR_UNSUPPORTED,			// format or operation is not supported with this binary
    FFMS_ERROR_FILE_READ,			// cannot read from file
    FFMS_ERROR_FILE_WRITE,			// cannot write to file
    FFMS_ERROR_NO_FILE,				// no such file or directory
    FFMS_ERROR_VERSION,				// wrong version
    FFMS_ERROR_ALLOCATION_FAILED,	// out of memory
    FFMS_ERROR_INVALID_ARGUMENT,	// invalid or nonsensical argument
    FFMS_ERROR_CODEC,				// decoder error
    FFMS_ERROR_NOT_AVAILABLE,		// requested mode or operation unavailable in this binary
    FFMS_ERROR_FILE_MISMATCH,		// provided index does not match the file
    FFMS_ERROR_USER					// problem exists between keyboard and chair
} FFMS_Errors;

typedef enum FFMS_SeekMode {
    FFMS_SEEK_LINEAR_NO_RW = -1,
    FFMS_SEEK_LINEAR = 0,
    FFMS_SEEK_NORMAL = 1,
    FFMS_SEEK_UNSAFE = 2,
    FFMS_SEEK_AGGRESSIVE = 3
} FFMS_SeekMode;

typedef enum FFMS_IndexErrorHandling {
    FFMS_IEH_ABORT = 0,
    FFMS_IEH_CLEAR_TRACK = 1,
    FFMS_IEH_STOP_TRACK = 2,
    FFMS_IEH_IGNORE = 3
} FFMS_IndexErrorHandling;

typedef enum FFMS_TrackType {
    FFMS_TYPE_UNKNOWN = -1,
    FFMS_TYPE_VIDEO,
    FFMS_TYPE_AUDIO,
    FFMS_TYPE_DATA,
    FFMS_TYPE_SUBTITLE,
    FFMS_TYPE_ATTACHMENT
} FFMS_TrackType;

typedef enum FFMS_SampleFormat {
    FFMS_FMT_U8 = 0,
    FFMS_FMT_S16,
    FFMS_FMT_S32,
    FFMS_FMT_FLT,
    FFMS_FMT_DBL
} FFMS_SampleFormat;

typedef enum FFMS_AudioChannel {
    FFMS_CH_FRONT_LEFT = 0x00000001,
    FFMS_CH_FRONT_RIGHT = 0x00000002,
    FFMS_CH_FRONT_CENTER = 0x00000004,
    FFMS_CH_LOW_FREQUENCY = 0x00000008,
    FFMS_CH_BACK_LEFT = 0x00000010,
    FFMS_CH_BACK_RIGHT = 0x00000020,
    FFMS_CH_FRONT_LEFT_OF_CENTER = 0x00000040,
    FFMS_CH_FRONT_RIGHT_OF_CENTER = 0x00000080,
    FFMS_CH_BACK_CENTER = 0x00000100,
    FFMS_CH_SIDE_LEFT = 0x00000200,
    FFMS_CH_SIDE_RIGHT = 0x00000400,
    FFMS_CH_TOP_CENTER = 0x00000800,
    FFMS_CH_TOP_FRONT_LEFT = 0x00001000,
    FFMS_CH_TOP_FRONT_CENTER = 0x00002000,
    FFMS_CH_TOP_FRONT_RIGHT = 0x00004000,
    FFMS_CH_TOP_BACK_LEFT = 0x00008000,
    FFMS_CH_TOP_BACK_CENTER = 0x00010000,
    FFMS_CH_TOP_BACK_RIGHT = 0x00020000,
    FFMS_CH_STEREO_LEFT = 0x20000000,
    FFMS_CH_STEREO_RIGHT = 0x40000000
} FFMS_AudioChannel;

typedef enum FFMS_AudioDelayModes {
    FFMS_DELAY_NO_SHIFT = -3,
    FFMS_DELAY_TIME_ZERO = -2,
    FFMS_DELAY_FIRST_VIDEO_TRACK = -1
} FFMS_AudioDelayModes;


typedef struct FFMS_ResampleOptions {
    int64_t ChannelLayout;
    FFMS_SampleFormat SampleFormat;
    int SampleRate;
    double CenterMixLevel;
    double SurroundMixLevel;
    double LFEMixLevel;
    int Normalize;
    int ForceResample;
    int ResampleFilterSize;
    int ResamplePhaseShift;
    int LinearInterpolation;
    double CutoffFrequencyRatio;
} FFMS_ResampleOptions;


typedef struct FFMS_Frame {
    const uint8_t *Data[4];
    int Linesize[4];
    int EncodedWidth;
    int EncodedHeight;
    int EncodedPixelFormat;
    int ScaledWidth;
    int ScaledHeight;
    int ConvertedPixelFormat;
    int KeyFrame;
    int RepeatPict;
    int InterlacedFrame;
    int TopFieldFirst;
    char PictType;
    int ColorSpace;
    int ColorRange;
    /* Introduced in FFMS_VERSION ((2 << 24) | (21 << 16) | (0 << 8) | 0) */
    int ColorPrimaries;
    int TransferCharateristics;
    int ChromaLocation;
    /* Introduced in FFMS_VERSION ((2 << 24) | (27 << 16) | (0 << 8) | 0) */
    int HasMasteringDisplayPrimaries;  /* Non-zero if the 4 fields below are valid */
    double MasteringDisplayPrimariesX[3];
    double MasteringDisplayPrimariesY[3];
    double MasteringDisplayWhitePointX;
    double MasteringDisplayWhitePointY;
    int HasMasteringDisplayLuminance; /* Non-zero if the 2 fields below are valid */
    double MasteringDisplayMinLuminance;
    double MasteringDisplayMaxLuminance;
    int HasContentLightLevel; /* Non-zero if the 2 fields below are valid */
    unsigned int ContentLightLevelMax;
    unsigned int ContentLightLevelAverage;
} FFMS_Frame;

typedef struct FFMS_TrackTimeBase {
    int64_t Num;
    int64_t Den;
} FFMS_TrackTimeBase;

typedef struct FFMS_FrameInfo {
    int64_t PTS;
    int RepeatPict;
    int KeyFrame;
    int64_t OriginalPTS;
} FFMS_FrameInfo;

typedef struct FFMS_VideoProperties {
    int FPSDenominator;
    int FPSNumerator;
    int RFFDenominator;
    int RFFNumerator;
    int NumFrames;
    int SARNum;
    int SARDen;
    int CropTop;
    int CropBottom;
    int CropLeft;
    int CropRight;
    int TopFieldFirst;
    FFMS_DEPRECATED int ColorSpace; /* Provided in FFMS_Frame */
    FFMS_DEPRECATED int ColorRange; /* Provided in FFMS_Frame */
    double FirstTime;
    double LastTime;
    /* Introduced in FFMS_VERSION ((2 << 24) | (24 << 16) | (0 << 8) | 0) */
    int Rotation;
    /* Introduced in FFMS_VERSION ((2 << 24) | (30 << 16) | (0 << 8) | 0) */
    double LastEndTime;
    int HasMasteringDisplayPrimaries;  /* Non-zero if the 4 fields below are valid */
    double MasteringDisplayPrimariesX[3];
    double MasteringDisplayPrimariesY[3];
    double MasteringDisplayWhitePointX;
    double MasteringDisplayWhitePointY;
    int HasMasteringDisplayLuminance; /* Non-zero if the 2 fields below are valid */
    double MasteringDisplayMinLuminance;
    double MasteringDisplayMaxLuminance;
    int HasContentLightLevel; /* Non-zero if the 2 fields below are valid */
    unsigned int ContentLightLevelMax;
    unsigned int ContentLightLevelAverage;
} FFMS_VideoProperties;

typedef struct FFMS_AudioProperties {
    int SampleFormat;
    int SampleRate;
    int BitsPerSample;
    int Channels;
    int64_t ChannelLayout; 
    int64_t NumSamples;
    double FirstTime;
    double LastTime;
    double LastEndTime;
} FFMS_AudioProperties;


FFMS_API(FFMS_VideoSource *) FFMS_CreateVideoSource(const char *SourceFile, int Track, FFMS_Index *Index, int Threads, int SeekMode, FFMS_ErrorInfo *ErrorInfo);

FFMS_API(FFMS_AudioSource *) FFMS_CreateAudioSource(const char *SourceFile, int Track, FFMS_Index *Index, int DelayMode, FFMS_ErrorInfo *ErrorInfo);

FFMS_API(void) FFMS_DestroyVideoSource(FFMS_VideoSource *V);

FFMS_API(void) FFMS_SupportRevert(FFMS_VideoSource *V, bool flag);

FFMS_API(void) FFMS_DestroyAudioSource(FFMS_AudioSource *A);

FFMS_API(const FFMS_VideoProperties *) FFMS_GetVideoProperties(FFMS_VideoSource *V);

FFMS_API(const FFMS_AudioProperties *) FFMS_GetAudioProperties(FFMS_AudioSource *A);

FFMS_API(const FFMS_Frame *) FFMS_GetFrame(FFMS_VideoSource *V, int n, FFMS_ErrorInfo *ErrorInfo);

FFMS_API(const FFMS_Frame *) FFMS_GetFrameByTime(FFMS_VideoSource *V, double Time, FFMS_ErrorInfo *ErrorInfo);

FFMS_API(int) FFMS_GetAudio(FFMS_AudioSource *A, void *Buf, int64_t Start, int64_t Count, FFMS_ErrorInfo *ErrorInfo);

FFMS_API(int) FFMS_SetOutputFormatV2(FFMS_VideoSource *V, int Width, int Height, FFMS_ErrorInfo *ErrorInfo);

FFMS_API(void) FFMS_DestroyIndex(FFMS_Index *Index);

FFMS_API(int) FFMS_GetFirstIndexedTrackOfType(FFMS_Index *Index, int TrackType, FFMS_ErrorInfo *ErrorInfo);

FFMS_API(int) FFMS_GetNumTracks(FFMS_Index *Index);

FFMS_API(const FFMS_FrameInfo *) FFMS_GetFrameInfo(FFMS_Track *T, int Frame);

FFMS_API(FFMS_Track *) FFMS_GetTrackFromVideo(FFMS_VideoSource *V);

FFMS_API(const FFMS_TrackTimeBase *) FFMS_GetTimeBase(FFMS_Track *T);

//��Index�ļ���ȡ������Ϣ
FFMS_API(FFMS_Indexer *) FFMS_CreateIndexer(const char *SourceFile, FFMS_ErrorInfo *ErrorInfo);

FFMS_API(void) FFMS_DestroyIndexer(FFMS_Indexer* indexer);



FFMS_API(void) FFMS_TrackIndexSettings(FFMS_Indexer *Indexer, int Track, int bIndex); /* Pass 0 to last argument, kapt to preserve abi. Introduced in FFMS_VERSION ((2 << 24) | (21 << 16) | (0 << 8) | 0) */

FFMS_API(void) FFMS_TrackTypeIndexSettings(FFMS_Indexer *Indexer, int TrackType, int Index); /* Pass 0 to last argument, kapt to preserve abi. Introduced in FFMS_VERSION ((2 << 24) | (21 << 16) | (0 << 8) | 0) */

//
FFMS_API(FFMS_Index *) FFMS_DoIndexing(FFMS_Indexer *Indexer, int ErrorHandling, FFMS_ErrorInfo *ErrorInfo); 

//��index���������ļ���ȡIndex
FFMS_API(FFMS_Index *) FFMS_ReadIndex(const char *IndexFile, FFMS_ErrorInfo *ErrorInfo);

//��index���������ļ���ȡIndex
FFMS_API(FFMS_Index*) FFMS_ReadIndexFromBuffer(uint8_t* buf, int size, FFMS_ErrorInfo* ErrorInfo);


//��⵱ǰ�����ļ��Ƿ��ڶ�ý���ļ�ƥ��
FFMS_API(int) FFMS_IndexBelongsToFile(FFMS_Index *Index, const char *SourceFile, FFMS_ErrorInfo *ErrorInfo);

//д��Index�����ļ�
FFMS_API(int) FFMS_WriteIndex(const char *IndexFile, FFMS_Index *Index, FFMS_ErrorInfo *ErrorInfo);

FFMS_API(int) FFMS_GetPixFmt(const char *Name);

FFMS_API (FFMS_Index*) FFMS_ProcessIndex(const char* Source, int* Track);

FFMS_API(MediaInfomation*) FFMS_MediaInformation(FFMS_Index* index);


#endif
