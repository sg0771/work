#ifndef __AVISYNTH_C_H_
#define __AVISYNTH_C_H_

#include <MediaLibAPIExt.h>

#define FRAME_ALIGN 16

#ifdef __cplusplus

#include <string>
#include <exception>

#define VideoInfo AVS_VideoInfo
#define VideoFrameBuffer AVS_VideoFrameBuffer
#define VideoFrame AVS_VideoFrame

typedef float SFLOAT;
class AvisynthError :std::exception /* exception */ {
public:
    const char* const msg;
    AvisynthError(const char* _msg) : msg(_msg) {}
}; // end class AvisynthError
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define AVSC_CC __stdcall
#define AVSC_INLINE static __inline
#define AVSC_API(ret, name) EXTERN_C  ret AVSC_CC name

#define AVISYNTH_INTERFACE_VERSION 6
#define AVS_FRAME_ALIGN 16 

    enum {
        SAMPLE_INT8 = 1 << 0,
        SAMPLE_INT16 = 1 << 1,
        SAMPLE_INT24 = 1 << 2,    // Int24 is a very stupid thing to code, but it's supported by some hardware.
        SAMPLE_INT32 = 1 << 3,
        SAMPLE_FLOAT = 1 << 4
    };

    enum {
        PLANAR_Y = 1 << 0,
        PLANAR_U = 1 << 1,
        PLANAR_V = 1 << 2,
        PLANAR_ALIGNED = 1 << 3,
        PLANAR_Y_ALIGNED = PLANAR_Y | PLANAR_ALIGNED,
        PLANAR_U_ALIGNED = PLANAR_U | PLANAR_ALIGNED,
        PLANAR_V_ALIGNED = PLANAR_V | PLANAR_ALIGNED,
        PLANAR_A = 1 << 4,
        PLANAR_R = 1 << 5,
        PLANAR_G = 1 << 6,
        PLANAR_B = 1 << 7,
        PLANAR_A_ALIGNED = PLANAR_A | PLANAR_ALIGNED,
        PLANAR_R_ALIGNED = PLANAR_R | PLANAR_ALIGNED,
        PLANAR_G_ALIGNED = PLANAR_G | PLANAR_ALIGNED,
        PLANAR_B_ALIGNED = PLANAR_B | PLANAR_ALIGNED,
    };
    enum {
        CS_BGR = 1 << 28,
        CS_YUV = 1 << 29,
        CS_INTERLEAVED = 1 << 30,
        CS_PLANAR = 1 << 31,

        CS_Shift_Sub_Width = 0,
        CS_Shift_Sub_Height = 8,
        CS_Shift_Sample_Bits = 16,

        CS_Sub_Width_Mask = 7 << CS_Shift_Sub_Width,
        CS_Sub_Width_1 = 3 << CS_Shift_Sub_Width, // YV24
        CS_Sub_Width_2 = 0 << CS_Shift_Sub_Width, // YV12, I420, YV16
        CS_Sub_Width_4 = 1 << CS_Shift_Sub_Width, // YUV9, YV411

        CS_VPlaneFirst = 1 << 3, // YV12, YV16, YV24, YV411, YUV9
        CS_UPlaneFirst = 1 << 4, // I420

        CS_Sub_Height_Mask = 7 << CS_Shift_Sub_Height,
        CS_Sub_Height_1 = 3 << CS_Shift_Sub_Height, // YV16, YV24, YV411
        CS_Sub_Height_2 = 0 << CS_Shift_Sub_Height, // YV12, I420
        CS_Sub_Height_4 = 1 << CS_Shift_Sub_Height, // YUV9

        CS_Sample_Bits_Mask = 7 << CS_Shift_Sample_Bits,
        CS_Sample_Bits_8 = 0 << CS_Shift_Sample_Bits,
        CS_Sample_Bits_16 = 1 << CS_Shift_Sample_Bits,
        CS_Sample_Bits_32 = 2 << CS_Shift_Sample_Bits,

        CS_PLANAR_MASK = CS_PLANAR | CS_INTERLEAVED | CS_YUV | CS_BGR | CS_Sample_Bits_Mask
        | CS_Sub_Height_Mask | CS_Sub_Width_Mask,
        CS_PLANAR_FILTER = ~(CS_VPlaneFirst | CS_UPlaneFirst),

        // Specific colorformats
        CS_UNKNOWN = 0,
        CS_BGR24 = 1 << 0 | CS_BGR | CS_INTERLEAVED,
        CS_BGR32 = 1 << 1 | CS_BGR | CS_INTERLEAVED,
        CS_YUY2 = 1 << 2 | CS_YUV | CS_INTERLEAVED,
        //  CS_YV12  = 1<<3  Reserved
        //  CS_I420  = 1<<4  Reserved
        CS_RAW32 = 1 << 5 | CS_INTERLEAVED,

        //  YV12 must be 0xA000008 2.5 Baked API will see all new planar as YV12
        //  I420 must be 0xA000010

        CS_YV24 = CS_PLANAR | CS_YUV | CS_Sample_Bits_8 | CS_VPlaneFirst | CS_Sub_Height_1 | CS_Sub_Width_1,  // YVU 4:4:4 planar
        CS_YV16 = CS_PLANAR | CS_YUV | CS_Sample_Bits_8 | CS_VPlaneFirst | CS_Sub_Height_1 | CS_Sub_Width_2,  // YVU 4:2:2 planar
        CS_YV12 = CS_PLANAR | CS_YUV | CS_Sample_Bits_8 | CS_VPlaneFirst | CS_Sub_Height_2 | CS_Sub_Width_2,  // YVU 4:2:0 planar
        CS_I420 = CS_PLANAR | CS_YUV | CS_Sample_Bits_8 | CS_UPlaneFirst | CS_Sub_Height_2 | CS_Sub_Width_2,  // YUV 4:2:0 planar
        CS_IYUV = CS_I420,
        CS_YUV9 = CS_PLANAR | CS_YUV | CS_Sample_Bits_8 | CS_VPlaneFirst | CS_Sub_Height_4 | CS_Sub_Width_4,  // YUV 4:1:0 planar
        CS_YV411 = CS_PLANAR | CS_YUV | CS_Sample_Bits_8 | CS_VPlaneFirst | CS_Sub_Height_1 | CS_Sub_Width_4,  // YUV 4:1:1 planar

        CS_Y8 = CS_PLANAR | CS_INTERLEAVED | CS_YUV | CS_Sample_Bits_8,                                     // Y   4:0:0 planar

        CS_YV48 = CS_PLANAR | CS_YUV | CS_Sample_Bits_16 | CS_VPlaneFirst | CS_Sub_Height_1 | CS_Sub_Width_1, // YUV 4:4:4 16bit samples
        CS_Y16 = CS_PLANAR | CS_INTERLEAVED | CS_YUV | CS_Sample_Bits_16,                                    // Y   4:0:0 16bit samples

        CS_YV96 = CS_PLANAR | CS_YUV | CS_Sample_Bits_32 | CS_VPlaneFirst | CS_Sub_Height_1 | CS_Sub_Width_1, // YUV 4:4:4 32bit samples
        CS_Y32 = CS_PLANAR | CS_INTERLEAVED | CS_YUV | CS_Sample_Bits_32,                                    // Y   4:0:0 32bit samples

        //CS_PRGB  = CS_PLANAR | CS_RGB | CS_Sample_Bits_8,                                                      // Planar RGB
        //CS_RGB48 = CS_PLANAR | CS_RGB | CS_Sample_Bits_16,                                                     // Planar RGB 16bit samples
        //CS_RGB96 = CS_PLANAR | CS_RGB | CS_Sample_Bits_32,                                                     // Planar RGB 32bit samples




    };
    enum {
        IT_BFF = 1 << 0,
        IT_TFF = 1 << 1,
        IT_FIELDBASED = 1 << 2
    };

    // Chroma placement bits 20 -> 23  ::FIXME:: Really want a Class to support this
    enum {
        CS_UNKNOWN_CHROMA_PLACEMENT = 0 << 20,
        CS_MPEG1_CHROMA_PLACEMENT = 1 << 20,
        CS_MPEG2_CHROMA_PLACEMENT = 2 << 20,
        CS_YUY2_CHROMA_PLACEMENT = 3 << 20,
        CS_TOPLEFT_CHROMA_PLACEMENT = 4 << 20
    };

    enum {
        AVS_SAMPLE_INT8 = 1 << 0,
        AVS_SAMPLE_INT16 = 1 << 1,
        AVS_SAMPLE_INT24 = 1 << 2,
        AVS_SAMPLE_INT32 = 1 << 3,
        AVS_SAMPLE_FLOAT = 1 << 4
    };

    enum {
        AVS_PLANAR_Y = 1 << 0,
        AVS_PLANAR_U = 1 << 1,
        AVS_PLANAR_V = 1 << 2,
        AVS_PLANAR_ALIGNED = 1 << 3,
        AVS_PLANAR_Y_ALIGNED = AVS_PLANAR_Y | AVS_PLANAR_ALIGNED,
        AVS_PLANAR_U_ALIGNED = AVS_PLANAR_U | AVS_PLANAR_ALIGNED,
        AVS_PLANAR_V_ALIGNED = AVS_PLANAR_V | AVS_PLANAR_ALIGNED,
        AVS_PLANAR_A = 1 << 4,
        AVS_PLANAR_R = 1 << 5,
        AVS_PLANAR_G = 1 << 6,
        AVS_PLANAR_B = 1 << 7,
        AVS_PLANAR_A_ALIGNED = AVS_PLANAR_A | AVS_PLANAR_ALIGNED,
        AVS_PLANAR_R_ALIGNED = AVS_PLANAR_R | AVS_PLANAR_ALIGNED,
        AVS_PLANAR_G_ALIGNED = AVS_PLANAR_G | AVS_PLANAR_ALIGNED,
        AVS_PLANAR_B_ALIGNED = AVS_PLANAR_B | AVS_PLANAR_ALIGNED
    };

    // Colorspace properties.
    enum {
        AVS_CS_BGR = 1 << 28,
        AVS_CS_YUV = 1 << 29,
        AVS_CS_INTERLEAVED = 1 << 30,
        AVS_CS_PLANAR = 1 << 31,

        AVS_CS_SHIFT_SUB_WIDTH = 0,
        AVS_CS_SHIFT_SUB_HEIGHT = 8,
        AVS_CS_SHIFT_SAMPLE_BITS = 16,

        AVS_CS_SUB_WIDTH_MASK = 7 << AVS_CS_SHIFT_SUB_WIDTH,
        AVS_CS_SUB_WIDTH_1 = 3 << AVS_CS_SHIFT_SUB_WIDTH, // YV24
        AVS_CS_SUB_WIDTH_2 = 0 << AVS_CS_SHIFT_SUB_WIDTH, // YV12, I420, YV16
        AVS_CS_SUB_WIDTH_4 = 1 << AVS_CS_SHIFT_SUB_WIDTH, // YUV9, YV411

        AVS_CS_VPLANEFIRST = 1 << 3, // YV12, YV16, YV24, YV411, YUV9
        AVS_CS_UPLANEFIRST = 1 << 4, // I420

        AVS_CS_SUB_HEIGHT_MASK = 7 << AVS_CS_SHIFT_SUB_HEIGHT,
        AVS_CS_SUB_HEIGHT_1 = 3 << AVS_CS_SHIFT_SUB_HEIGHT, // YV16, YV24, YV411
        AVS_CS_SUB_HEIGHT_2 = 0 << AVS_CS_SHIFT_SUB_HEIGHT, // YV12, I420
        AVS_CS_SUB_HEIGHT_4 = 1 << AVS_CS_SHIFT_SUB_HEIGHT, // YUV9

        AVS_CS_SAMPLE_BITS_MASK = 7 << AVS_CS_SHIFT_SAMPLE_BITS,
        AVS_CS_SAMPLE_BITS_8 = 0 << AVS_CS_SHIFT_SAMPLE_BITS,
        AVS_CS_SAMPLE_BITS_16 = 1 << AVS_CS_SHIFT_SAMPLE_BITS,
        AVS_CS_SAMPLE_BITS_32 = 2 << AVS_CS_SHIFT_SAMPLE_BITS,

        AVS_CS_PLANAR_MASK = AVS_CS_PLANAR | AVS_CS_INTERLEAVED | AVS_CS_YUV | AVS_CS_BGR | AVS_CS_SAMPLE_BITS_MASK | AVS_CS_SUB_HEIGHT_MASK | AVS_CS_SUB_WIDTH_MASK,
        AVS_CS_PLANAR_FILTER = ~(AVS_CS_VPLANEFIRST | AVS_CS_UPLANEFIRST)
    };

    enum {
        AVS_CS_UNKNOWN = 0,
        AVS_CS_BGR24 = 1 << 0 | AVS_CS_BGR | AVS_CS_INTERLEAVED,
        AVS_CS_BGR32 = 1 << 1 | AVS_CS_BGR | AVS_CS_INTERLEAVED,
        AVS_CS_YUY2 = 1 << 2 | AVS_CS_YUV | AVS_CS_INTERLEAVED,
        //  AVS_CS_YV12  = 1<<3  Reserved
        //  AVS_CS_I420  = 1<<4  Reserved
        AVS_CS_RAW32 = 1 << 5 | AVS_CS_INTERLEAVED,

        AVS_CS_YV24 = AVS_CS_PLANAR | AVS_CS_YUV | AVS_CS_SAMPLE_BITS_8 | AVS_CS_VPLANEFIRST | AVS_CS_SUB_HEIGHT_1 | AVS_CS_SUB_WIDTH_1,  // YVU 4:4:4 planar
        AVS_CS_YV16 = AVS_CS_PLANAR | AVS_CS_YUV | AVS_CS_SAMPLE_BITS_8 | AVS_CS_VPLANEFIRST | AVS_CS_SUB_HEIGHT_1 | AVS_CS_SUB_WIDTH_2,  // YVU 4:2:2 planar
        AVS_CS_YV12 = AVS_CS_PLANAR | AVS_CS_YUV | AVS_CS_SAMPLE_BITS_8 | AVS_CS_VPLANEFIRST | AVS_CS_SUB_HEIGHT_2 | AVS_CS_SUB_WIDTH_2,  // YVU 4:2:0 planar
        AVS_CS_I420 = AVS_CS_PLANAR | AVS_CS_YUV | AVS_CS_SAMPLE_BITS_8 | AVS_CS_UPLANEFIRST | AVS_CS_SUB_HEIGHT_2 | AVS_CS_SUB_WIDTH_2,  // YUV 4:2:0 planar
        AVS_CS_IYUV = AVS_CS_I420,
        AVS_CS_YV411 = AVS_CS_PLANAR | AVS_CS_YUV | AVS_CS_SAMPLE_BITS_8 | AVS_CS_VPLANEFIRST | AVS_CS_SUB_HEIGHT_1 | AVS_CS_SUB_WIDTH_4,  // YVU 4:1:1 planar
        AVS_CS_YUV9 = AVS_CS_PLANAR | AVS_CS_YUV | AVS_CS_SAMPLE_BITS_8 | AVS_CS_VPLANEFIRST | AVS_CS_SUB_HEIGHT_4 | AVS_CS_SUB_WIDTH_4,  // YVU 4:1:0 planar
        AVS_CS_Y8 = AVS_CS_PLANAR | AVS_CS_INTERLEAVED | AVS_CS_YUV | AVS_CS_SAMPLE_BITS_8                                              // Y   4:0:0 planar
    };

    enum {
        AVS_IT_BFF = 1 << 0,
        AVS_IT_TFF = 1 << 1,
        AVS_IT_FIELDBASED = 1 << 2
    };

    enum {
        AVS_FILTER_TYPE = 1,
        AVS_FILTER_INPUT_COLORSPACE = 2,
        AVS_FILTER_OUTPUT_TYPE = 9,
        AVS_FILTER_NAME = 4,
        AVS_FILTER_AUTHOR = 5,
        AVS_FILTER_VERSION = 6,
        AVS_FILTER_ARGS = 7,
        AVS_FILTER_ARGS_INFO = 8,
        AVS_FILTER_ARGS_DESCRIPTION = 10,
        AVS_FILTER_DESCRIPTION = 11
    };

    enum {  //SUBTYPES
        AVS_FILTER_TYPE_AUDIO = 1,
        AVS_FILTER_TYPE_VIDEO = 2,
        AVS_FILTER_OUTPUT_TYPE_SAME = 3,
        AVS_FILTER_OUTPUT_TYPE_DIFFERENT = 4
    };

    enum {
        // New 2.6 explicitly defined cache hints.
        AVS_CACHE_NOTHING = 10, // Do not cache video.
        AVS_CACHE_WINDOW = 11, // Hard protect upto X frames within a range of X from the current frame N.
        AVS_CACHE_GENERIC = 12, // LRU cache upto X frames.
        AVS_CACHE_FORCE_GENERIC = 13, // LRU cache upto X frames, override any previous CACHE_WINDOW.

        AVS_CACHE_GET_POLICY = 30, // Get the current policy.
        AVS_CACHE_GET_WINDOW = 31, // Get the current window h_span.
        AVS_CACHE_GET_RANGE = 32, // Get the current generic frame range.

        AVS_CACHE_AUDIO = 50, // Explicitly do cache audio, X byte cache.
        AVS_CACHE_AUDIO_NOTHING = 51, // Explicitly do not cache audio.
        AVS_CACHE_AUDIO_NONE = 52, // Audio cache off (auto mode), X byte intial cache.
        AVS_CACHE_AUDIO_AUTO = 53, // Audio cache on (auto mode), X byte intial cache.

        AVS_CACHE_GET_AUDIO_POLICY = 70, // Get the current audio policy.
        AVS_CACHE_GET_AUDIO_SIZE = 71, // Get the current audio cache size.

        AVS_CACHE_PREFETCH_FRAME = 100, // Queue request to prefetch frame N.
        AVS_CACHE_PREFETCH_GO = 101, // Action video prefetches.

        AVS_CACHE_PREFETCH_AUDIO_BEGIN = 120, // Begin queue request transaction to prefetch audio (take critical section).
        AVS_CACHE_PREFETCH_AUDIO_STARTLO = 121, // Set low 32 bits of start.
        AVS_CACHE_PREFETCH_AUDIO_STARTHI = 122, // Set high 32 bits of start.
        AVS_CACHE_PREFETCH_AUDIO_COUNT = 123, // Set low 32 bits of length.
        AVS_CACHE_PREFETCH_AUDIO_COMMIT = 124, // Enqueue request transaction to prefetch audio (release critical section).
        AVS_CACHE_PREFETCH_AUDIO_GO = 125, // Action audio prefetches.

        AVS_CACHE_GETCHILD_CACHE_MODE = 200, // Cache ask Child for desired video cache mode.
        AVS_CACHE_GETCHILD_CACHE_SIZE = 201, // Cache ask Child for desired video cache size.
        AVS_CACHE_GETCHILD_AUDIO_MODE = 202, // Cache ask Child for desired audio cache mode.
        AVS_CACHE_GETCHILD_AUDIO_SIZE = 203, // Cache ask Child for desired audio cache size.

        AVS_CACHE_GETCHILD_COST = 220, // Cache ask Child for estimated processing cost.
        AVS_CACHE_COST_ZERO = 221, // Child response of zero cost (ptr arithmetic only).
        AVS_CACHE_COST_UNIT = 222, // Child response of unit cost (less than or equal 1 full frame blit).
        AVS_CACHE_COST_LOW = 223, // Child response of light cost. (Fast)
        AVS_CACHE_COST_MED = 224, // Child response of medium cost. (Real time)
        AVS_CACHE_COST_HI = 225, // Child response of heavy cost. (Slow)

        AVS_CACHE_GETCHILD_THREAD_MODE = 240, // Cache ask Child for thread safetyness.
        AVS_CACHE_THREAD_UNSAFE = 241, // Only 1 thread allowed for all instances. 2.5 filters default!
        AVS_CACHE_THREAD_CLASS = 242, // Only 1 thread allowed for each instance. 2.6 filters default!
        AVS_CACHE_THREAD_SAFE = 243, //  Allow all threads in any instance.
        AVS_CACHE_THREAD_OWN = 244, // Safe but limit to 1 thread, internally threaded.

        AVS_CACHE_GETCHILD_ACCESS_COST = 260, // Cache ask Child for preferred access pattern.
        AVS_CACHE_ACCESS_RAND = 261, // Filter is access order agnostic.
        AVS_CACHE_ACCESS_SEQ0 = 262, // Filter prefers sequential access (low cost)
        AVS_CACHE_ACCESS_SEQ1 = 263, // Filter needs sequential access (high cost)
    };

    typedef struct AVS_Clip AVS_Clip;
    typedef struct AVS_ScriptEnvironment AVS_ScriptEnvironment;

    typedef struct AVS_VideoInfo {
        int width, height;    // width=0 means no video
        unsigned fps_numerator, fps_denominator;
        int num_frames;

        int pixel_type;

        int audio_samples_per_second;   // 0 means no audio
        int sample_type;
        INT64 num_audio_samples;
        int nchannels;

        int image_type;

#ifdef __cplusplus
        double aspect_ratio;

        bool IsY() const { return false; }

        bool IsYUVA()const { return false; }
        bool IsPlanarRGBA() const { return false; }
        int ComponentSize() const { return 0; }
        // useful functions of the above
        bool HasVideo() const { return (width != 0); }
        bool HasAudio() const { return (audio_samples_per_second != 0); }
        bool IsRGB() const { return !!(pixel_type & CS_BGR); }
        bool IsRGB24() const { return (pixel_type & CS_BGR24) == CS_BGR24; } // Clear out additional properties
        bool IsRGB32() const { return (pixel_type & CS_BGR32) == CS_BGR32; }
        bool IsYUV() const { return !!(pixel_type & CS_YUV); }
        bool IsYUY2() const { return (pixel_type & CS_YUY2) == CS_YUY2; }

        bool IsYV24()  const { return (pixel_type & CS_PLANAR_MASK) == (CS_YV24 & CS_PLANAR_FILTER); }
        bool IsYV16()  const { return (pixel_type & CS_PLANAR_MASK) == (CS_YV16 & CS_PLANAR_FILTER); }
        bool IsYV12()  const { return (pixel_type & CS_PLANAR_MASK) == (CS_YV12 & CS_PLANAR_FILTER); }
        bool IsY8()    const { return (pixel_type & CS_PLANAR_MASK) == (CS_Y8 & CS_PLANAR_FILTER); }

        bool IsYV411() const { return (pixel_type & CS_PLANAR_MASK) == (CS_YV411 & CS_PLANAR_FILTER); }
        //bool VideoInfo::IsYUV9()  const { return (pixel_type & CS_PLANAR_MASK) == (CS_YUV9  & CS_PLANAR_FILTER); }

        /* Baked ********************
        bool IsColorSpace(int c_space) const { return ((pixel_type & c_space) == c_space); }
           Baked ********************/
        bool IsColorSpace(int c_space) const {
            return IsPlanar() ? ((pixel_type & CS_PLANAR_MASK) == (c_space & CS_PLANAR_FILTER)) : ((pixel_type & c_space) == c_space);
        }

        bool Is(int property) const { return ((image_type & property) == property); }
        bool IsPlanar() const { return !!(pixel_type & CS_PLANAR); }
        bool IsFieldBased() const { return !!(image_type & IT_FIELDBASED); }
        bool IsParityKnown() const { return ((image_type & IT_FIELDBASED) && (image_type & (IT_BFF | IT_TFF))); }
        bool IsBFF() const { return !!(image_type & IT_BFF); }
        bool IsTFF() const { return !!(image_type & IT_TFF); }

        __int64 AudioSamplesFromFrames(int frames) const { return (fps_numerator && HasVideo()) ? ((__int64)(frames)*audio_samples_per_second * fps_denominator / fps_numerator) : 0; }
        int FramesFromAudioSamples(__int64 samples) const { return (fps_denominator && HasAudio()) ? (int)((samples * fps_numerator) / ((__int64)fps_denominator * audio_samples_per_second)) : 0; }
        __int64 AudioSamplesFromBytes(__int64 bytes) const { return HasAudio() ? bytes / BytesPerAudioSample() : 0; }
        __int64 BytesFromAudioSamples(__int64 samples) const { return samples * BytesPerAudioSample(); }
        int AudioChannels() const { return HasAudio() ? nchannels : 0; }
        int SampleType() const { return sample_type; }
        bool IsSampleType(int testtype) const { return !!(sample_type & testtype); }
        int SamplesPerSecond() const { return audio_samples_per_second; }
        int BytesPerAudioSample() const { return nchannels * BytesPerChannelSample(); }
        void SetFieldBased(bool isfieldbased) { if (isfieldbased) image_type |= IT_FIELDBASED; else  image_type &= ~IT_FIELDBASED; }
        void Set(int property) { image_type |= property; }
        void Clear(int property) { image_type &= ~property; }


        int BytesPerChannelSample() const {
            switch (sample_type) {
            case SAMPLE_INT8:
                return sizeof(unsigned char);
            case SAMPLE_INT16:
                return sizeof(signed short);
            case SAMPLE_INT24:
                return 3;
            case SAMPLE_INT32:
                return sizeof(signed int);
            case SAMPLE_FLOAT:
                return sizeof(SFLOAT);
            default:
                //_ASSERTE("Sample type not recognized!");
                return 0;
            }
        }

        bool IsVPlaneFirst() const {
            return !IsY8() && IsPlanar() && (pixel_type & (CS_VPlaneFirst | CS_UPlaneFirst)) == CS_VPlaneFirst;   // Shouldn't use this
        }

        int BytesFromPixels(int pixels) const {
            return !IsY8() && IsPlanar() ? pixels << ((pixel_type >> CS_Shift_Sample_Bits) & 3) : pixels * (BitsPerPixel() >> 3);   // For planar images, will return luma plane
        }

        int RowSize(int plane = 0) const {
            const int rowsize = BytesFromPixels(width);

            switch (plane) {
            case PLANAR_U: case PLANAR_V:
                return (!IsY8() && IsPlanar()) ? rowsize >> GetPlaneWidthSubsampling(plane) : 0;

            case PLANAR_U_ALIGNED: case PLANAR_V_ALIGNED:
                return (!IsY8() && IsPlanar()) ? ((rowsize >> GetPlaneWidthSubsampling(plane)) + FRAME_ALIGN - 1) & (~(FRAME_ALIGN - 1)) : 0; // Aligned rowsize

            case PLANAR_Y_ALIGNED:
                return (rowsize + FRAME_ALIGN - 1) & (~(FRAME_ALIGN - 1)); // Aligned rowsize
            }
            return rowsize;
        }

        int GetPlaneWidthSubsampling(int plane) const {  // Subsampling in bitshifts!
            if (plane == PLANAR_Y)  // No subsampling
                return 0;
            if (IsY8())
                throw AvisynthError("Filter error: GetPlaneWidthSubsampling not available on Y8 pixel type.");
            if (plane == PLANAR_U || plane == PLANAR_V) {
                if (IsYUY2())
                    return 1;
                else if (IsPlanar())
                    return ((pixel_type >> CS_Shift_Sub_Width) + 1) & 3;
                else
                    throw AvisynthError("Filter error: GetPlaneWidthSubsampling called with unsupported pixel type.");
            }
            throw AvisynthError("Filter error: GetPlaneWidthSubsampling called with unsupported plane.");
        }

        int GetPlaneHeightSubsampling(int plane) const {  // Subsampling in bitshifts!
            if (plane == PLANAR_Y)  // No subsampling
                return 0;
            if (IsY8())
                throw AvisynthError("Filter error: GetPlaneHeightSubsampling not available on Y8 pixel type.");
            if (plane == PLANAR_U || plane == PLANAR_V) {
                if (IsYUY2())
                    return 0;
                else if (IsPlanar())
                    return ((pixel_type >> CS_Shift_Sub_Height) + 1) & 3;
                else
                    throw AvisynthError("Filter error: GetPlaneHeightSubsampling called with unsupported pixel type.");
            }
            throw AvisynthError("Filter error: GetPlaneHeightSubsampling called with supported plane.");
        }

        int BitsPerPixel() const {
            // Lookup Interleaved, calculate PLANAR's
            switch (pixel_type) {
            case CS_BGR24:
                return 24;
            case CS_BGR32:
                return 32;
            case CS_YUY2:
                return 16;
            case CS_Y8:
                return 8;
                //    case CS_Y16:
                //      return 16;
                //    case CS_Y32:
                //      return 32;
            }
            if (IsPlanar()) {
                const int S = IsYUV() ? GetPlaneWidthSubsampling(PLANAR_U) + GetPlaneHeightSubsampling(PLANAR_U) : 0;
                return (((1 << S) + 2) * (8 << ((pixel_type >> CS_Shift_Sample_Bits) & 3))) >> S;
            }
            return 0;
        }

        // useful mutator
        void SetFPS(unsigned numerator, unsigned denominator) {
            if ((numerator == 0) || (denominator == 0)) {
                fps_numerator = 0;
                fps_denominator = 1;
            }
            else {
                unsigned x = numerator, y = denominator;
                while (y) {   // find gcd
                    unsigned t = x % y; x = y; y = t;
                }
                fps_numerator = numerator / x;
                fps_denominator = denominator / x;
            }
    }

        // Range protected multiply-divide of FPS
        void MulDivFPS(unsigned multiplier, unsigned divisor) {
            unsigned __int64 numerator = UInt32x32To64(fps_numerator, multiplier);
            unsigned __int64 denominator = UInt32x32To64(fps_denominator, divisor);

            unsigned __int64 x = numerator, y = denominator;
            while (y) {   // find gcd
                unsigned __int64 t = x % y; x = y; y = t;
            }
            numerator /= x; // normalize
            denominator /= x;

            unsigned __int64 temp = numerator | denominator; // Just looking top bit
            unsigned u = 0;
            while (temp & 0xffffffff80000000) { // or perhaps > 16777216*2
                temp = Int64ShrlMod32(temp, 1);
                u++;
            }
            if (u) { // Scale to fit
                const unsigned round = 1 << (u - 1);
                SetFPS((unsigned)Int64ShrlMod32(numerator + round, u),
                    (unsigned)Int64ShrlMod32(denominator + round, u));
            }
            else {
                fps_numerator = (unsigned)numerator;
                fps_denominator = (unsigned)denominator;
            }
        }

        // Test for same colorspace
        bool IsSameColorspace(const AVS_VideoInfo& vi) const {
            if (vi.pixel_type == pixel_type) return TRUE;
            if (IsYV12() && vi.IsYV12()) return TRUE;
            return FALSE;
        }
#endif
    } AVS_VideoInfo;

    AVSC_INLINE int avs_has_video(const AVS_VideoInfo* p)
    {
        return (p->width != 0);
    }

    AVSC_INLINE int avs_has_audio(const AVS_VideoInfo* p)
    {
        return (p->audio_samples_per_second != 0);
    }

    AVSC_INLINE int avs_is_rgb(const AVS_VideoInfo* p)
    {
        return !!(p->pixel_type & AVS_CS_BGR);
    }

    AVSC_INLINE int avs_is_rgb24(const AVS_VideoInfo* p)
    {
        return (p->pixel_type & AVS_CS_BGR24) == AVS_CS_BGR24;
    } // Clear out additional properties

    AVSC_API(int, avs_bits_per_pixel)(const AVS_VideoInfo* p);

    AVSC_INLINE int avs_bytes_per_channel_sample(const AVS_VideoInfo* p)
    {
        switch (p->sample_type) {
        case AVS_SAMPLE_INT8:  return sizeof(signed char);
        case AVS_SAMPLE_INT16: return sizeof(signed short);
        case AVS_SAMPLE_INT24: return 3;
        case AVS_SAMPLE_INT32: return sizeof(signed int);
        case AVS_SAMPLE_FLOAT: return sizeof(float);
        default: return 0;
        }
    }

    typedef struct AVS_VideoFrameBuffer {
#ifndef __cplusplus
        BYTE* data;
        int data_size;
        volatile long sequence_number;
        volatile long refcount;
        bool holded;//false
#endif

#ifdef __cplusplus
        BYTE* data = NULL;
        int data_size = 0;
        volatile long sequence_number = 0;
        volatile long refcount = 0;
        bool holded = false;//false

        friend struct VideoFrame;
        friend class Cache;
        friend class ScriptEnvironment;
    protected:
        VideoFrameBuffer(size_t size);
        VideoFrameBuffer();
        ~VideoFrameBuffer();

    public:
        const BYTE* GetReadPtr() const { return data; }
        BYTE* GetWritePtr() { InterlockedIncrement(&sequence_number); return data; }
        size_t GetDataSize() const { return data_size; }
        int GetSequenceNumber() const { return sequence_number; }
        int GetRefcount() const { return refcount; }
#endif
    } AVS_VideoFrameBuffer;

    typedef struct AVS_VideoFrame {
        volatile long refcount;
        AVS_VideoFrameBuffer* vfb;
        int offset, pitch, row_size, height, offsetU, offsetV, pitchUV;  // U&V offsets are from top of picture.
        int row_sizeUV, heightUV;
#ifdef __cplusplus
        friend class PVideoFrame;
        void AddRef() { InterlockedIncrement(&refcount); }
        void Release() {
            AVS_VideoFrameBuffer* _vfb = vfb;
            if (!InterlockedDecrement(&refcount))
                InterlockedDecrement(&_vfb->refcount);
        }

        friend class ScriptEnvironment;
        friend class Cache;

        VideoFrame(VideoFrameBuffer* _vfb, size_t _offset, int _pitch, int _row_size, int _height);
        VideoFrame(VideoFrameBuffer* _vfb, size_t _offset, int _pitch, int _row_size, int _height,
            size_t _offsetU, size_t _offsetV, int _pitchUV, int _row_sizeUV, int _heightUV);

        void* operator new(size_t size);
        // TESTME: OFFSET U/V may be switched to what could be expected from AVI standard!
    public:
        void SetHoldvfb(bool flag) { vfb->holded = flag; }
        int GetPitch(int plane = 0)const {
            switch (plane) {
            case PLANAR_U:
            case PLANAR_V:
                return pitchUV;
            }
            return pitch;
        }
        int GetRowSize(int plane = 0) const {
            switch (plane) {
            case PLANAR_U: case PLANAR_V: if (pitchUV) return row_sizeUV; else return 0;
            case PLANAR_U_ALIGNED: case PLANAR_V_ALIGNED:
                if (pitchUV) {
                    const int r = (row_sizeUV + FRAME_ALIGN - 1) & (~(FRAME_ALIGN - 1)); // Aligned rowsize
                    if (r <= pitchUV)
                        return r;
                    return row_sizeUV;
                }
                else return 0;
            case PLANAR_ALIGNED: case PLANAR_Y_ALIGNED:
                const int r = (row_size + FRAME_ALIGN - 1) & (~(FRAME_ALIGN - 1)); // Aligned rowsize
                if (r <= pitch)
                    return r;
                return row_size;
            }
            return row_size;
        }

        int GetHeight(int plane = 0) const {
            switch (plane) {
            case PLANAR_U:
            case PLANAR_V:
                if (pitchUV)
                    return heightUV;
                return 0;
            }
            return height;
        }

        VideoFrameBuffer* GetFrameBuffer()const { return vfb; }
        size_t GetOffset(int plane = 0)const {
            switch (plane) {
            case PLANAR_U:
                return offsetU;
            case PLANAR_V:
                return offsetV;
            default:
                return offset;
            };
        }

        VideoFrame* Subframe(int rel_offset, int new_pitch, int new_row_size, int new_height) const;
        VideoFrame* Subframe(int rel_offset, int new_pitch, int new_row_size, int new_height,
            int rel_offsetU, int rel_offsetV, int pitchUV) const;

        const BYTE* GetReadPtr(int plane = 0) const { return vfb->GetReadPtr() + GetOffset(plane); }

        bool IsWritable() const {
            if (refcount == 1 && vfb->refcount == 1) {
                vfb->GetWritePtr(); // Bump sequence number
                return true;
            }
            return false;
        }

        BYTE* GetWritePtr(int plane = 0) const {
            if (!plane || plane == PLANAR_Y) {
                //if (vfb->GetRefcount() > 1) {
                //    _ASSERT(FALSE);
                //    //        throw AvisynthError("Internal Error - refcount was more than one!");
                //}
                if (refcount > 0 && vfb->refcount > 0) {
                    return  vfb->GetWritePtr() + GetOffset(plane);
                }
                return (refcount == 1 && vfb->refcount == 1) ? vfb->GetWritePtr() + GetOffset(plane) : 0;
            }
            return vfb->data + GetOffset(plane);
        }

        ~VideoFrame() { DESTRUCTOR(); }

    public:
        void DESTRUCTOR() { Release(); }
#endif
    } AVS_VideoFrame;

    AVSC_API(int, avs_get_pitch_p)(const AVS_VideoFrame* p, int plane);

    AVSC_INLINE int avs_get_pitch(const AVS_VideoFrame* p) {
        return avs_get_pitch_p(p, 0);
    }

    AVSC_API(int, avs_get_row_size_p)(const AVS_VideoFrame* p, int plane);

    AVSC_INLINE int avs_get_row_size(const AVS_VideoFrame* p) {
        return p->row_size;
    }

    AVSC_API(int, avs_get_height_p)(const AVS_VideoFrame* p, int plane);

    AVSC_INLINE int avs_get_height(const AVS_VideoFrame* p) {
        return p->height;
    }

    AVSC_API(const BYTE*, avs_get_read_ptr_p)(const AVS_VideoFrame* p, int plane);

    AVSC_INLINE const BYTE* avs_get_read_ptr(const AVS_VideoFrame* p) {
        return avs_get_read_ptr_p(p, 0);
    }

    AVSC_API(void, avs_release_video_frame)(AVS_VideoFrame*);


    AVSC_API(void, avs_release_value)(AVS_Value);

    AVSC_INLINE int avs_is_clip(AVS_Value v) { return v.type == 'c'; }

    AVSC_INLINE int avs_is_error(AVS_Value v) { return v.type == 'e'; }

    AVSC_API(AVS_Clip*, avs_wrapper_clip)(void*, AVS_ScriptEnvironment*);

    AVSC_API(AVS_Clip*, avs_take_clip)(AVS_Value, AVS_ScriptEnvironment*);

    AVSC_API(void, avs_set_to_clip)(AVS_Value*, AVS_Clip*);

    AVSC_INLINE const char* avs_as_error(AVS_Value v)
    {
        return avs_is_error(v) ? v.d.string : 0;
    }

    AVSC_INLINE AVS_Value avs_new_value_clip(AVS_Clip* v0)
    {
        AVS_Value v; avs_set_to_clip(&v, v0); return v;
    }

    AVSC_API(void, avs_release_clip)(AVS_Clip*);

    AVSC_API(const char*, avs_clip_get_error)(AVS_Clip*); // return 0 if no error

    AVSC_API(const AVS_VideoInfo*, avs_get_video_info)(AVS_Clip*);

    AVSC_API(int, avs_get_version)(AVS_Clip*);

    AVSC_API(AVS_VideoFrame*, avs_get_frame)(AVS_Clip*, int n);
    // The returned video frame must be released with avs_release_video_frame

    AVSC_API(int, avs_get_audio)(AVS_Clip*, void* buf,
        INT64 start, INT64 count);

    AVSC_API(void*, avs_get_envplus)(AVS_ScriptEnvironment*); // return 0 if no error

    AVSC_API(const char*, avs_get_error)(AVS_ScriptEnvironment*); // return 0 if no error


    AVSC_API(void, avs_bit_blt)(AVS_ScriptEnvironment*, BYTE* dstp, int dst_pitch, const BYTE* srcp, int src_pitch, int row_size, int height);

    AVSC_API(AVS_ScriptEnvironment*, avs_create_script_environment)(int version);

#ifdef __cplusplus
}
#endif

#endif  //__AVISYNTH_C_H_
