
extern "C" {
#include <libavutil/mem.h>
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/pixdesc.h>
}

#include <vector>
#include "ffms.h"

enum BCSType {
    cGRAY,
    cYUV,
    cRGB,
    cUNUSABLE
};

// swscale and pp-related functions
SwsContext *GetSwsContext(int SrcW, int SrcH, AVPixelFormat SrcFormat, int SrcColorSpace, int SrcColorRange, int DstW, int DstH, AVPixelFormat DstFormat, int DstColorSpace, int DstColorRange, int64_t Flags);
BCSType GuessCSType(AVPixelFormat p);

// timebase-related functions
void CorrectRationalFramerate(int *Num, int *Den);
void CorrectTimebase(FFMS_VideoProperties *VP, FFMS_TrackTimeBase *TTimebase);

// our implementation of avcodec_find_best_pix_fmt()
AVPixelFormat FindBestPixelFormat(const std::vector<AVPixelFormat> &Dsts, AVPixelFormat Src);

// handling of alt-refs in VP8 and VP9
void ParseVP8(const uint8_t Buf, bool *Invisible, int *PictType);
void ParseVP9(const uint8_t Buf, bool *Invisible, int *PictType);
