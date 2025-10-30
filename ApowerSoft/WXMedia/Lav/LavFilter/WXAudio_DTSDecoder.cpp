
#include "WXAudioImpl.h"
#include "LAVAudio.h"
#include "WXAudio_PostProcessor.h"
#include "WXAudio_Media.h"

extern "C" {
#include "libavutil/intreadwrite.h"
};

#include <WXMediaCpp.h>

// PCM Volume Adjustment Factors, both for integer and float math
// entries start at 2 channel mixing, half volume
static int pcm_volume_adjust_integer[7] = {
  362, 443, 512, 572, 627, 677, 724
};

static float pcm_volume_adjust_float[7] = {
  1.41421356f, 1.73205081f, 2.00000000f, 2.23606798f, 2.44948974f, 2.64575131f, 2.82842712f
};

// SCALE_CA helper macro for SampleCopyAdjust
#define SCALE_CA(sample, iFactor, factor) { \
  if (iFactor > 0) { \
    sample *= factor; \
    sample >>= 8; \
  } else { \
    sample <<= 8; \
    sample /= factor; \
  } \
}

//
// Helper Function that reads one sample from pIn, applys the scale specified by iFactor, and writes it to pOut
//
static inline void SampleCopyAdjust(BYTE* pOut, const BYTE* pIn, int iFactor, LAVAudioSampleFormat sfSampleFormat)
{
    ASSERT(abs(iFactor) > 1 && abs(iFactor) <= 8);
    const int factorIndex = abs(iFactor) - 2;

    switch (sfSampleFormat) {
    case SampleFormat_U8:
    {
        uint8_t* pOutSample = pOut;
        int32_t sample = *pIn + INT8_MIN;
        SCALE_CA(sample, iFactor, pcm_volume_adjust_integer[factorIndex]);
        *pOutSample = av_clip_uint8(sample - INT8_MIN);
    }
    break;
    case SampleFormat_16:
    {
        int16_t* pOutSample = (int16_t*)pOut;
        int32_t sample = *((int16_t*)pIn);
        SCALE_CA(sample, iFactor, pcm_volume_adjust_integer[factorIndex]);
        *pOutSample = av_clip_int16(sample);
    }
    break;
    case SampleFormat_24:
    {
        int32_t sample = (pIn[0] << 8) + (pIn[1] << 16) + (pIn[2] << 24);
        sample >>= 8;
        SCALE_CA(sample, iFactor, pcm_volume_adjust_integer[factorIndex]);
        sample = av_clip(sample, INT24_MIN, INT24_MAX);
        pOut[0] = sample & 0xff;
        pOut[1] = (sample >> 8) & 0xff;
        pOut[2] = (sample >> 16) & 0xff;
    }
    break;
    case SampleFormat_32:
    {
        int32_t* pOutSample = (int32_t*)pOut;
        int64_t sample = *((int32_t*)pIn);
        SCALE_CA(sample, iFactor, pcm_volume_adjust_integer[factorIndex]);
        *pOutSample = av_clipl_int32(sample);
    }
    break;
    case SampleFormat_FP32:
    {
        float* pOutSample = (float*)pOut;
        float sample = *((float*)pIn);
        if (iFactor > 0) {
            sample *= pcm_volume_adjust_float[factorIndex];
        }
        else {
            sample /= pcm_volume_adjust_float[factorIndex];
        }
        *pOutSample = av_clipf(sample, -1.0f, 1.0f);
    }
    break;
    default:
        ASSERT(0);
        break;
    }
}

//
// Writes one sample of silence into the buffer
//
static inline void Silence(BYTE* pBuffer, LAVAudioSampleFormat sfSampleFormat)
{
    switch (sfSampleFormat) {
    case SampleFormat_16:
    case SampleFormat_24:
    case SampleFormat_32:
    case SampleFormat_FP32:
        memset(pBuffer, 0, get_byte_per_sample(sfSampleFormat));
        break;
    case SampleFormat_U8:
        *pBuffer = 128U;
        break;
    default:
        ASSERT(0);
    }
}

//
// Channel Remapping Processor
// This function can process a PCM buffer of any sample format, and remap the channels
// into any arbitrary layout and channel count.
//
// The samples are copied byte-by-byte, without any conversion or loss.
//
// The ChannelMap is assumed to always have at least uOutChannels valid entries.
// Its layout is in output format:
//      Map[0] is the first output channel, and should contain the index in the source stream (or -1 for silence)
//      Map[1] is the second output channel
//
// Source channels can be applied multiple times to the Destination, however Volume Adjustments are not performed.
// Furthermore, multiple Source channels cannot be merged into one channel.
// Note that when copying one source channel into multiple destinations, you always want to reduce its volume.
// You can either do this in a second step, or use ExtendedChannelMapping
//
// Examples:
// 5.1 Input Buffer, following map will extract the Center channel, and return it as Mono:
// uOutChannels == 1; map = {2}
//
// Mono Input Buffer, Convert to Stereo
// uOutChannels == 2; map = {0, 0}
//
HRESULT ChannelMapping(BufferDetails* pcm, const unsigned uOutChannels, const ChannelMap map)
{
    ExtendedChannelMap extMap;
    for (unsigned ch = 0; ch < uOutChannels; ++ch) {
        ASSERT(map[ch] >= -1 && map[ch] < pcm->wChannels);
        extMap[ch].idx = map[ch];
        extMap[ch].factor = 0;
    }

    return ExtendedChannelMapping(pcm, uOutChannels, extMap);
}

//
// Extended Channel Remapping Processor
// Same functionality as ChannelMapping, except that a factor can be applied to all PCM samples.
//
// For optimization, the factor cannot be freely specified.
// Factors -1, 0, 1 are ignored.
// A Factor of 2 doubles the volume, 3 trippled, etc.
// A Factor of -2 will produce half volume, -3 one third, etc.
// The limit is a factor of 8/-8
//
// Otherwise, see ChannelMapping
HRESULT ExtendedChannelMapping(BufferDetails* pcm, const unsigned uOutChannels, const ExtendedChannelMap extMap)
{
    // Sample Size
    const unsigned uSampleSize = get_byte_per_sample(pcm->sfFormat);

    // New Output Buffer
    GrowableArray<BYTE>* out = new GrowableArray<BYTE>();
    out->SetSize(uOutChannels * pcm->nSamples * uSampleSize);

    const BYTE* pIn = pcm->bBuffer->Ptr();
    BYTE* pOut = out->Ptr();

    for (unsigned i = 0; i < pcm->nSamples; ++i) {
        for (unsigned ch = 0; ch < uOutChannels; ++ch) {
            if (extMap[ch].idx >= 0) {
                if (!extMap[ch].factor || abs(extMap[ch].factor) == 1)
                    memcpy(pOut, pIn + (extMap[ch].idx * uSampleSize), uSampleSize);
                else
                    SampleCopyAdjust(pOut, pIn + (extMap[ch].idx * uSampleSize), extMap[ch].factor, pcm->sfFormat);
            }
            else
                Silence(pOut, pcm->sfFormat);
            pOut += uSampleSize;
        }
        pIn += uSampleSize * pcm->wChannels;
    }

    // Apply changes to buffer
    delete pcm->bBuffer;
    pcm->bBuffer = out;
    pcm->wChannels = uOutChannels;

    return S_OK;
}

#define CHL_CONTAINS_ALL(l, m) (((l) & (m)) == (m))
#define CHL_ALL_OR_NONE(l, m) (((l) & (m)) == (m) || ((l) & (m)) == 0)

HRESULT CLAVAudio::CheckChannelLayoutConformity(DWORD dwLayout)
{
    int channels = av_get_channel_layout_nb_channels(dwLayout);

    // We require multi-channel and at least containing stereo
    if (!CHL_CONTAINS_ALL(dwLayout, AV_CH_LAYOUT_STEREO) || channels == 2)
        goto noprocessing;

    // We do not know what to do with "top" channels
    if (dwLayout & (AV_CH_TOP_CENTER | AV_CH_TOP_FRONT_LEFT | AV_CH_TOP_FRONT_CENTER | AV_CH_TOP_FRONT_RIGHT | AV_CH_TOP_BACK_LEFT | AV_CH_TOP_BACK_CENTER | AV_CH_TOP_BACK_RIGHT)) {
        DbgLog((LOG_ERROR, 10, L"::CheckChannelLayoutConformity(): Layout with top channels is not supported (mask: 0x%x)", dwLayout));
        goto noprocessing;
    }

    // We need either both surround channels, or none. One of a type is not supported
    if (!CHL_ALL_OR_NONE(dwLayout, AV_CH_BACK_LEFT | AV_CH_BACK_RIGHT)
        || !CHL_ALL_OR_NONE(dwLayout, AV_CH_SIDE_LEFT | AV_CH_SIDE_RIGHT)
        || !CHL_ALL_OR_NONE(dwLayout, AV_CH_FRONT_LEFT_OF_CENTER | AV_CH_FRONT_RIGHT_OF_CENTER)) {
        DbgLog((LOG_ERROR, 10, L"::CheckChannelLayoutConformity(): Layout with only one surround channel is not supported (mask: 0x%x)", dwLayout));
        goto noprocessing;
    }

    // No need to process full 5.1/6.1 layouts, or any 8 channel layouts
    if (dwLayout == AV_CH_LAYOUT_5POINT1
        || dwLayout == AV_CH_LAYOUT_5POINT1_BACK
        || dwLayout == AV_CH_LAYOUT_6POINT1_BACK
        || channels == 8) {
        DbgLog((LOG_TRACE, 10, L"::CheckChannelLayoutConformity(): Layout is already a default layout (mask: 0x%x)", dwLayout));
        goto noprocessing;
    }

    // Check 5.1 channels
    if (CHL_CONTAINS_ALL(AV_CH_LAYOUT_5POINT1, dwLayout)        /* 5.1 with side channels */
        || CHL_CONTAINS_ALL(AV_CH_LAYOUT_5POINT1_BACK, dwLayout)   /* 5.1 with back channels */
        || CHL_CONTAINS_ALL(LAV_CH_LAYOUT_5POINT1_WIDE, dwLayout)  /* 5.1 with side-front channels */
        || CHL_CONTAINS_ALL(LAV_CH_LAYOUT_5POINT1_BC, dwLayout))   /* 3/1/x layouts, front channels with a back center */
        return Create51Conformity(dwLayout);

    // Check 6.1 channels (5.1 layouts + Back Center)
    if (CHL_CONTAINS_ALL(AV_CH_LAYOUT_6POINT1, dwLayout)        /* 6.1 with side channels */
        || CHL_CONTAINS_ALL(AV_CH_LAYOUT_6POINT1_BACK, dwLayout)   /* 6.1 with back channels */
        || CHL_CONTAINS_ALL(LAV_CH_LAYOUT_5POINT1_WIDE | AV_CH_BACK_CENTER, dwLayout)) /* 6.1 with side-front channels */
        return Create61Conformity(dwLayout);

    // Check 7.1 channels
    if (CHL_CONTAINS_ALL(AV_CH_LAYOUT_7POINT1, dwLayout)              /* 7.1 with side+back channels */
        || CHL_CONTAINS_ALL(AV_CH_LAYOUT_7POINT1_WIDE, dwLayout)         /* 7.1 with front-side+back channels */
        || CHL_CONTAINS_ALL(LAV_CH_LAYOUT_7POINT1_EXTRAWIDE, dwLayout))  /* 7.1 with front-side+side channels */
        return Create71Conformity(dwLayout);

noprocessing:
    m_bChannelMappingRequired = FALSE;
    return S_FALSE;
}

HRESULT CLAVAudio::Create51Conformity(DWORD dwLayout)
{
    DbgLog((LOG_TRACE, 10, L"::Create51Conformity(): Creating 5.1 default layout (mask: 0x%x)", dwLayout));
    int ch = 0;
    ExtChMapClear(&m_ChannelMap);
    // All layouts we support have to contain L/R
    ExtChMapSet(&m_ChannelMap, 0, ch++, 0);
    ExtChMapSet(&m_ChannelMap, 1, ch++, 0);
    // Center channel
    if (dwLayout & AV_CH_FRONT_CENTER)
        ExtChMapSet(&m_ChannelMap, 2, ch++, 0);
    // LFE
    if (dwLayout & AV_CH_LOW_FREQUENCY)
        ExtChMapSet(&m_ChannelMap, 3, ch++, 0);
    // Back/Side
    if (dwLayout & (AV_CH_SIDE_LEFT | AV_CH_BACK_LEFT | AV_CH_FRONT_LEFT_OF_CENTER)) {
        ExtChMapSet(&m_ChannelMap, 4, ch++, 0);
        ExtChMapSet(&m_ChannelMap, 5, ch++, 0);
        // Back Center
    }
    else if (dwLayout & AV_CH_BACK_CENTER) {
        ExtChMapSet(&m_ChannelMap, 4, ch, -2);
        ExtChMapSet(&m_ChannelMap, 5, ch++, -2);
    }
    m_bChannelMappingRequired = TRUE;
    m_ChannelMapOutputChannels = 6;
    m_ChannelMapOutputLayout = AV_CH_LAYOUT_5POINT1;
    return S_OK;
}

HRESULT CLAVAudio::Create61Conformity(DWORD dwLayout)
{
    if (m_settings.Expand61) {
        DbgLog((LOG_TRACE, 10, L"::Create61Conformity(): Expanding to 7.1"));
        return Create71Conformity(dwLayout);
    }

    DbgLog((LOG_TRACE, 10, L"::Create61Conformity(): Creating 6.1 default layout (mask: 0x%x)", dwLayout));
    int ch = 0;
    ExtChMapClear(&m_ChannelMap);
    // All layouts we support have to contain L/R
    ExtChMapSet(&m_ChannelMap, 0, ch++, 0);
    ExtChMapSet(&m_ChannelMap, 1, ch++, 0);
    // Center channel
    if (dwLayout & AV_CH_FRONT_CENTER)
        ExtChMapSet(&m_ChannelMap, 2, ch++, 0);
    // LFE
    if (dwLayout & AV_CH_LOW_FREQUENCY)
        ExtChMapSet(&m_ChannelMap, 3, ch++, 0);
    // Back channels, if before BC
    if (dwLayout & (AV_CH_BACK_LEFT | AV_CH_FRONT_LEFT_OF_CENTER)) {
        DbgLog((LOG_TRACE, 10, L"::Create61Conformity(): Using surround channels *before* BC"));
        ExtChMapSet(&m_ChannelMap, 4, ch++, 0);
        ExtChMapSet(&m_ChannelMap, 5, ch++, 0);
    }
    // Back Center
    if (dwLayout & AV_CH_BACK_CENTER)
        ExtChMapSet(&m_ChannelMap, 6, ch++, 0);
    // Back channels, if after BC
    if (dwLayout & AV_CH_SIDE_LEFT) {
        DbgLog((LOG_TRACE, 10, L"::Create61Conformity(): Using surround channels *after* BC"));
        ExtChMapSet(&m_ChannelMap, 4, ch++, 0);
        ExtChMapSet(&m_ChannelMap, 5, ch++, 0);
    }

    m_bChannelMappingRequired = TRUE;
    m_ChannelMapOutputChannels = 7;
    m_ChannelMapOutputLayout = AV_CH_LAYOUT_6POINT1_BACK;
    return S_OK;
}

HRESULT CLAVAudio::Create71Conformity(DWORD dwLayout)
{
    DbgLog((LOG_TRACE, 10, L"::Create71Conformity(): Creating 7.1 default layout (mask: 0x%x)", dwLayout));
    int ch = 0;
    ExtChMapClear(&m_ChannelMap);
    // All layouts we support have to contain L/R
    ExtChMapSet(&m_ChannelMap, 0, ch++, 0);
    ExtChMapSet(&m_ChannelMap, 1, ch++, 0);
    // Center channel
    if (dwLayout & AV_CH_FRONT_CENTER)
        ExtChMapSet(&m_ChannelMap, 2, ch++, 0);
    // LFE
    if (dwLayout & AV_CH_LOW_FREQUENCY)
        ExtChMapSet(&m_ChannelMap, 3, ch++, 0);
    // Back channels
    if (dwLayout & AV_CH_BACK_CENTER) {
        DbgLog((LOG_TRACE, 10, L"::Create71Conformity(): Usign BC to fill back channels"));
        if (dwLayout & AV_CH_SIDE_LEFT) {
            DbgLog((LOG_TRACE, 10, L"::Create71Conformity(): Using BC before side-surround channels"));
            ExtChMapSet(&m_ChannelMap, 4, ch, -2);
            ExtChMapSet(&m_ChannelMap, 5, ch++, -2);
            ExtChMapSet(&m_ChannelMap, 6, ch++, 0);
            ExtChMapSet(&m_ChannelMap, 7, ch++, 0);
        }
        else {
            DbgLog((LOG_TRACE, 10, L"::Create71Conformity(): Using BC after side-surround channels"));
            ExtChMapSet(&m_ChannelMap, 6, ch++, 0);
            ExtChMapSet(&m_ChannelMap, 7, ch++, 0);
            ExtChMapSet(&m_ChannelMap, 4, ch, -2);
            ExtChMapSet(&m_ChannelMap, 5, ch++, -2);
        }
    }
    else {
        DbgLog((LOG_TRACE, 10, L"::Create71Conformity(): Using original 4 surround channels"));
        ExtChMapSet(&m_ChannelMap, 4, ch++, 0);
        ExtChMapSet(&m_ChannelMap, 5, ch++, 0);
        ExtChMapSet(&m_ChannelMap, 6, ch++, 0);
        ExtChMapSet(&m_ChannelMap, 7, ch++, 0);
    }

    m_bChannelMappingRequired = TRUE;
    m_ChannelMapOutputChannels = 8;
    m_ChannelMapOutputLayout = AV_CH_LAYOUT_7POINT1;
    return S_OK;
}

HRESULT CLAVAudio::PadTo32(BufferDetails* buffer)
{
    ASSERT(buffer->sfFormat == SampleFormat_24);

    const DWORD size = (buffer->nSamples * buffer->wChannels) * 4;
    GrowableArray<BYTE>* pcmOut = new GrowableArray<BYTE>();
    pcmOut->SetSize(size);

    const BYTE* pDataIn = buffer->bBuffer->Ptr();
    BYTE* pDataOut = pcmOut->Ptr();

    for (unsigned int i = 0; i < buffer->nSamples; ++i) {
        for (int ch = 0; ch < buffer->wChannels; ++ch) {
            AV_WL32(pDataOut, AV_RL24(pDataIn) << 8);
            pDataOut += 4;
            pDataIn += 3;
        }
    }
    delete buffer->bBuffer;
    buffer->bBuffer = pcmOut;
    buffer->sfFormat = SampleFormat_32;
    buffer->wBitsPerSample = 24;

    return S_OK;
}

HRESULT CLAVAudio::Truncate32Buffer(BufferDetails* buffer)
{
    ASSERT(buffer->sfFormat == SampleFormat_32 && buffer->wBitsPerSample <= 24);

    // Determine the bytes per sample to keep. Cut a 16-bit sample to 24 if 16-bit is disabled for some reason
    const int bytes_per_sample = buffer->wBitsPerSample <= 16 && GetSampleFormat(SampleFormat_16) ? 2 : 3;
    if (bytes_per_sample == 3 && !GetSampleFormat(SampleFormat_24))
        return S_FALSE;

    const int skip = 4 - bytes_per_sample;
    const DWORD size = (buffer->nSamples * buffer->wChannels) * bytes_per_sample;
    GrowableArray<BYTE>* pcmOut = new GrowableArray<BYTE>();
    pcmOut->SetSize(size);

    const BYTE* pDataIn = buffer->bBuffer->Ptr();
    BYTE* pDataOut = pcmOut->Ptr();

    pDataIn += skip;
    for (unsigned int i = 0; i < buffer->nSamples; ++i) {
        for (int ch = 0; ch < buffer->wChannels; ++ch) {
            memcpy(pDataOut, pDataIn, bytes_per_sample);
            pDataOut += bytes_per_sample;
            pDataIn += 4;
        }
    }

    delete buffer->bBuffer;
    buffer->bBuffer = pcmOut;
    buffer->sfFormat = bytes_per_sample == 3 ? SampleFormat_24 : SampleFormat_16;

    return S_OK;
}

HRESULT CLAVAudio::PerformAVRProcessing(BufferDetails* buffer)
{
    int ret = 0;
    LAVAudioSampleFormat bufferFormat;
    GrowableArray<BYTE>* pcmOut;
    BYTE* pOut = NULL;
    BYTE* pIn = NULL;
    DWORD dwMixingLayout = m_dwOverrideMixer ? m_dwOverrideMixer : (m_settings.MixingEnabled ? m_settings.MixingLayout : buffer->dwChannelMask);
    // No mixing stereo, if the user doesn't want it
    if (buffer->wChannels <= 2 && (m_settings.MixingFlags & LAV_MIXING_FLAG_UNTOUCHED_STEREO))
        dwMixingLayout = buffer->dwChannelMask;

    LAVAudioSampleFormat outputFormat = (dwMixingLayout != buffer->dwChannelMask) ? GetBestAvailableSampleFormat(SampleFormat_FP32) : GetBestAvailableSampleFormat(buffer->sfFormat);

    // Short Circuit some processing
    if (dwMixingLayout == buffer->dwChannelMask && !buffer->bPlanar) {
        if (buffer->sfFormat == SampleFormat_24 && outputFormat == SampleFormat_32) {
            PadTo32(buffer);
            return S_OK;
        }
        else if (buffer->sfFormat == SampleFormat_32 && outputFormat == SampleFormat_24) {
            buffer->wBitsPerSample = 24;
            Truncate32Buffer(buffer);
            return S_OK;
        }
    }

    // Sadly, we need to convert this, avresample has no 24-bit mode
    if (buffer->sfFormat == SampleFormat_24) {
        PadTo32(buffer);
    }

    if (buffer->dwChannelMask != m_MixingInputLayout || (!m_avrContext && !m_bAVResampleFailed) || m_bMixingSettingsChanged || m_dwRemixLayout != dwMixingLayout || outputFormat != m_sfRemixFormat || buffer->sfFormat != m_MixingInputFormat) {
        m_bAVResampleFailed = FALSE;
        m_bMixingSettingsChanged = FALSE;
        if (m_avrContext) {
            avresample_close(m_avrContext);
            avresample_free(&m_avrContext);
        }

        m_MixingInputLayout = buffer->dwChannelMask;
        m_MixingInputFormat = buffer->sfFormat;
        m_dwRemixLayout = dwMixingLayout;
        m_sfRemixFormat = outputFormat;

        m_avrContext = avresample_alloc_context();
        av_opt_set_int(m_avrContext, "in_channel_layout", buffer->dwChannelMask, 0);
        av_opt_set_int(m_avrContext, "in_sample_fmt", get_ff_sample_fmt(buffer->sfFormat), 0);

        av_opt_set_int(m_avrContext, "out_channel_layout", dwMixingLayout, 0);
        av_opt_set_int(m_avrContext, "out_sample_fmt", get_ff_sample_fmt(m_sfRemixFormat), 0);

        av_opt_set_int(m_avrContext, "dither_method", m_settings.SampleConvertDither ? AV_RESAMPLE_DITHER_TRIANGULAR_HP : AV_RESAMPLE_DITHER_NONE, 0);

        // Setup mixing properties, if needed
        if (buffer->dwChannelMask != dwMixingLayout) {
            BOOL bNormalize = !!(m_settings.MixingFlags & LAV_MIXING_FLAG_NORMALIZE_MATRIX);
            av_opt_set_int(m_avrContext, "clip_protection", !bNormalize && (m_settings.MixingFlags & LAV_MIXING_FLAG_CLIP_PROTECTION), 0);

            // Create Matrix
            int in_ch = buffer->wChannels;
            int out_ch = av_get_channel_layout_nb_channels(dwMixingLayout);
            double* matrix_dbl = (double*)av_mallocz(in_ch * out_ch * sizeof(*matrix_dbl));

            const double center_mix_level = (double)m_settings.MixingCenterLevel / 10000.0;
            const double surround_mix_level = (double)m_settings.MixingSurroundLevel / 10000.0;
            const double lfe_mix_level = (double)m_settings.MixingLFELevel / 10000.0 / (dwMixingLayout == AV_CH_LAYOUT_MONO ? 1.0 : M_SQRT1_2);
            ret = avresample_build_matrix(buffer->dwChannelMask, dwMixingLayout, center_mix_level, surround_mix_level, lfe_mix_level, bNormalize, matrix_dbl, in_ch, (AVMatrixEncoding)m_settings.MixingMode);
            if (ret < 0) {
                DbgLog((LOG_ERROR, 10, L"avresample_build_matrix failed, layout in: %x, out: %x, sample fmt in: %d, out: %d", buffer->dwChannelMask, dwMixingLayout, buffer->sfFormat, m_sfRemixFormat));
                av_free(matrix_dbl);
                goto setuperr;
            }

            // Set Matrix on the context
            ret = avresample_set_matrix(m_avrContext, matrix_dbl, in_ch);
            av_free(matrix_dbl);
            if (ret < 0) {
                DbgLog((LOG_ERROR, 10, L"avresample_set_matrix failed, layout in: %x, out: %x, sample fmt in: %d, out: %d", buffer->dwChannelMask, dwMixingLayout, buffer->sfFormat, m_sfRemixFormat));
                goto setuperr;
            }
        }

        // Open Resample Context
        ret = avresample_open(m_avrContext);
        if (ret < 0) {
            DbgLog((LOG_ERROR, 10, L"avresample_open failed, layout in: %x, out: %x, sample fmt in: %d, out: %d", buffer->dwChannelMask, dwMixingLayout, buffer->sfFormat, m_sfRemixFormat));
            goto setuperr;
        }
    }

    if (!m_avrContext) {
        DbgLog((LOG_ERROR, 10, L"avresample context missing?"));
        return E_FAIL;
    }

    bufferFormat = (m_sfRemixFormat == SampleFormat_24) ? SampleFormat_32 : m_sfRemixFormat; // avresample always outputs 32-bit

    pcmOut = new GrowableArray<BYTE>();
    pcmOut->Allocate(FFALIGN(buffer->nSamples, 32) * av_get_channel_layout_nb_channels(m_dwRemixLayout) * get_byte_per_sample(bufferFormat));
    pOut = pcmOut->Ptr();

    pIn = buffer->bBuffer->Ptr();
    ret = avresample_convert(m_avrContext, &pOut, pcmOut->GetAllocated(), buffer->nSamples, &pIn, buffer->bBuffer->GetAllocated(), buffer->nSamples);
    if (ret < 0) {
        DbgLog((LOG_ERROR, 10, L"avresample_convert failed"));
        delete pcmOut;
        return S_FALSE;
    }

    delete buffer->bBuffer;
    buffer->bBuffer = pcmOut;
    buffer->dwChannelMask = m_dwRemixLayout;
    buffer->sfFormat = bufferFormat;
    buffer->wBitsPerSample = get_byte_per_sample(m_sfRemixFormat) << 3;
    buffer->wChannels = av_get_channel_layout_nb_channels(m_dwRemixLayout);
    buffer->bBuffer->SetSize(buffer->wChannels * buffer->nSamples * get_byte_per_sample(buffer->sfFormat));

    return S_OK;
setuperr:
    avresample_free(&m_avrContext);
    m_bAVResampleFailed = TRUE;
    return E_FAIL;
}

HRESULT CLAVAudio::PostProcess(BufferDetails* buffer)
{
    int layout_channels = av_get_channel_layout_nb_channels(buffer->dwChannelMask);

    // Validate channel mask
    if (!buffer->dwChannelMask || layout_channels != buffer->wChannels) {
        buffer->dwChannelMask = get_channel_mask(buffer->wChannels);
        if (!buffer->dwChannelMask && buffer->wChannels <= 2) {
            buffer->dwChannelMask = buffer->wChannels == 2 ? AV_CH_LAYOUT_STEREO : AV_CH_LAYOUT_MONO;
        }
    }

    if (m_settings.SuppressFormatChanges) {
        if (!m_SuppressLayout) {
            m_SuppressLayout = buffer->dwChannelMask;
        }
        else {
            if (buffer->dwChannelMask != m_SuppressLayout && buffer->wChannels <= av_get_channel_layout_nb_channels(m_SuppressLayout)) {
                // only warn once
                if (m_dwOverrideMixer != m_SuppressLayout) {
                    DbgLog((LOG_TRACE, 10, L"Channel Format change suppressed, from 0x%x to 0x%x", m_SuppressLayout, buffer->dwChannelMask));
                    m_dwOverrideMixer = m_SuppressLayout;
                }
            }
            else if (buffer->wChannels > av_get_channel_layout_nb_channels(m_SuppressLayout)) {
                DbgLog((LOG_TRACE, 10, L"Channel count increased, allowing change from 0x%x to 0x%x", m_SuppressLayout, buffer->dwChannelMask));
                m_dwOverrideMixer = 0;
                m_SuppressLayout = buffer->dwChannelMask;
            }
        }
    }

    DWORD dwMixingLayout = m_dwOverrideMixer ? m_dwOverrideMixer : m_settings.MixingLayout;
    BOOL bMixing = (m_settings.MixingEnabled || m_dwOverrideMixer) && buffer->dwChannelMask != dwMixingLayout;
    LAVAudioSampleFormat outputFormat = GetBestAvailableSampleFormat(buffer->sfFormat);
    // Perform conversion to layout and sample format, if required
    if (bMixing || outputFormat != buffer->sfFormat) {
        PerformAVRProcessing(buffer);
    }

    // Remap to standard configurations, if requested (not in combination with mixing)
    if (!bMixing && m_settings.OutputStandardLayout) {
        if (buffer->dwChannelMask != m_DecodeLayoutSanified) {
            m_DecodeLayoutSanified = buffer->dwChannelMask;
            CheckChannelLayoutConformity(buffer->dwChannelMask);
        }
        if (m_bChannelMappingRequired) {
            ExtendedChannelMapping(buffer, m_ChannelMapOutputChannels, m_ChannelMap);
            buffer->dwChannelMask = m_ChannelMapOutputLayout;
        }
    }

    // Map to the requested 5.1 layout
    if (m_settings.Output51Legacy && buffer->dwChannelMask == AV_CH_LAYOUT_5POINT1)
        buffer->dwChannelMask = AV_CH_LAYOUT_5POINT1_BACK;
    else if (!m_settings.Output51Legacy && buffer->dwChannelMask == AV_CH_LAYOUT_5POINT1_BACK)
        buffer->dwChannelMask = AV_CH_LAYOUT_5POINT1;

    // Check if current output uses back layout, and keep it active in that case
    if (buffer->dwChannelMask == AV_CH_LAYOUT_5POINT1) {
        WAVEFORMATEX* wfe = (WAVEFORMATEX*)m_pOutput->CurrentMediaType().Format();
        if (wfe->wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
            WAVEFORMATEXTENSIBLE* wfex = (WAVEFORMATEXTENSIBLE*)wfe;
            if (wfex->dwChannelMask == AV_CH_LAYOUT_5POINT1_BACK)
                buffer->dwChannelMask = AV_CH_LAYOUT_5POINT1_BACK;
        }
    }

    // Mono -> Stereo expansion
    if (buffer->wChannels == 1 && m_settings.ExpandMono) {
        ExtendedChannelMap map = { {0,-2}, {0, -2} };
        ExtendedChannelMapping(buffer, 2, map);
        buffer->dwChannelMask = AV_CH_LAYOUT_STEREO;
    }

    // 6.1 -> 7.1 expansion
    if (m_settings.Expand61) {
        if (buffer->dwChannelMask == AV_CH_LAYOUT_6POINT1_BACK) {
            ExtendedChannelMap map = { {0,0}, {1,0}, {2,0}, {3,0}, {6,-2}, {6,-2}, {4,0}, {5,0} };
            ExtendedChannelMapping(buffer, 8, map);
            buffer->dwChannelMask = AV_CH_LAYOUT_7POINT1;
        }
        else if (buffer->dwChannelMask == AV_CH_LAYOUT_6POINT1) {
            ExtendedChannelMap map = { {0,0}, {1,0}, {2,0}, {3,0}, {4,-2}, {4,-2}, {5,0}, {6,0} };
            ExtendedChannelMapping(buffer, 8, map);
            buffer->dwChannelMask = AV_CH_LAYOUT_7POINT1;
        }
    }

    if (m_bVolumeStats) {
        UpdateVolumeStats(*buffer);
    }

    // Truncate 24-in-32 to real 24
    if (buffer->sfFormat == SampleFormat_32 && buffer->wBitsPerSample && buffer->wBitsPerSample <= 24) {
        Truncate32Buffer(buffer);
    }

    return S_OK;
}



static const uint8_t dca_channels[16] = {
  1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 6, 6, 6, 7, 8, 8
};

/** Taken from ffmpeg libavcodec/dca.c */
static const int64_t dca_core_channel_layout[] = {
  AV_CH_FRONT_CENTER,                                                      ///< 1, A
  AV_CH_LAYOUT_STEREO,                                                     ///< 2, A + B (dual mono)
  AV_CH_LAYOUT_STEREO,                                                     ///< 2, L + R (stereo)
  AV_CH_LAYOUT_STEREO,                                                     ///< 2, (L+R) + (L-R) (sum-difference)
  AV_CH_LAYOUT_STEREO,                                                     ///< 2, LT +RT (left and right total)
  AV_CH_LAYOUT_STEREO|AV_CH_FRONT_CENTER,                                  ///< 3, C+L+R
  AV_CH_LAYOUT_STEREO|AV_CH_BACK_CENTER,                                   ///< 3, L+R+S
  AV_CH_LAYOUT_STEREO|AV_CH_FRONT_CENTER|AV_CH_BACK_CENTER,                ///< 4, C + L + R+ S
  AV_CH_LAYOUT_STEREO|AV_CH_SIDE_LEFT|AV_CH_SIDE_RIGHT,                    ///< 4, L + R +SL+ SR
  AV_CH_LAYOUT_STEREO|AV_CH_FRONT_CENTER|AV_CH_SIDE_LEFT|AV_CH_SIDE_RIGHT, ///< 5, C + L + R+ SL+SR
  AV_CH_LAYOUT_STEREO|AV_CH_SIDE_LEFT|AV_CH_SIDE_RIGHT|AV_CH_FRONT_LEFT_OF_CENTER|AV_CH_FRONT_RIGHT_OF_CENTER,                    ///< 6, CL + CR + L + R + SL + SR
  AV_CH_LAYOUT_STEREO|AV_CH_BACK_LEFT|AV_CH_BACK_RIGHT|AV_CH_FRONT_CENTER|AV_CH_BACK_CENTER,                                      ///< 6, C + L + R+ LR + RR + OV
  AV_CH_FRONT_CENTER|AV_CH_FRONT_RIGHT_OF_CENTER|AV_CH_FRONT_LEFT_OF_CENTER|AV_CH_BACK_CENTER|AV_CH_BACK_LEFT|AV_CH_BACK_RIGHT,   ///< 6, CF+ CR+LF+ RF+LR + RR
  AV_CH_FRONT_LEFT_OF_CENTER|AV_CH_FRONT_CENTER|AV_CH_FRONT_RIGHT_OF_CENTER|AV_CH_LAYOUT_STEREO|AV_CH_SIDE_LEFT|AV_CH_SIDE_RIGHT, ///< 7, CL + C + CR + L + R + SL + SR
  AV_CH_FRONT_LEFT_OF_CENTER|AV_CH_FRONT_RIGHT_OF_CENTER|AV_CH_LAYOUT_STEREO|AV_CH_SIDE_LEFT|AV_CH_SIDE_RIGHT|AV_CH_BACK_LEFT|AV_CH_BACK_RIGHT, ///< 8, CL + CR + L + R + SL1 + SL2+ SR1 + SR2
  AV_CH_FRONT_LEFT_OF_CENTER|AV_CH_FRONT_CENTER|AV_CH_FRONT_RIGHT_OF_CENTER|AV_CH_LAYOUT_STEREO|AV_CH_SIDE_LEFT|AV_CH_BACK_CENTER|AV_CH_SIDE_RIGHT, ///< 8, CL + C+ CR + L + R + SL + S+ SR
};

typedef void* (*DtsOpen)();
typedef int (*DtsClose)(void *context);
typedef int (*DtsReset)(void *context);
typedef int (*DtsSetParam)(void *context, int channels, int bitdepth, int unk1, int unk2, int unk3);
typedef int (*DtsDecode)(void *context, BYTE *pInput, int len, BYTE *pOutput, int unk1, int unk2, int *pBitdepth, int *pChannels, int *pCoreSampleRate, int *pUnk4, int *pHDSampleRate, int *pUnk5, int *pProfile);

struct DTSDecoder {
  DtsOpen pDtsOpen;
  DtsClose pDtsClose;
  DtsReset pDtsReset;
  DtsSetParam pDtsSetParam;
  DtsDecode pDtsDecode;

  void *dtsContext;
  BYTE *dtsPCMBuffer;

  DTSDecoder() : pDtsOpen(nullptr), pDtsClose(nullptr), pDtsReset(nullptr), pDtsSetParam(nullptr), pDtsDecode(nullptr), dtsContext(nullptr), dtsPCMBuffer(nullptr) {}
  ~DTSDecoder() {
    if (pDtsClose && dtsContext) {
      pDtsClose(dtsContext);
    }
  }
};



HRESULT CLAVAudio::InitDTSDecoder()
{
  if (!m_hDllExtraDecoder) {
    // Add path of LAVAudio.ax into the Dll search path
    WCHAR wModuleFile[1024];
    GetModuleFileName(g_hInst, wModuleFile, 1024);
    PathRemoveFileSpecW(wModuleFile);
    wcscat_s(wModuleFile, TEXT("\\dtsdecoderdll.dll"));

    // Try loading from the filters directory
    HMODULE hDll = LoadLibraryEx(wModuleFile, nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
    // And try from any global directories if this failed
    if (hDll == nullptr) {
      hDll = LoadLibrary(TEXT("dtsdecoderdll.dll"));
    }
    CheckPointer(hDll, E_FAIL);

    BOOL bIncompatibleDecoder = FALSE;
    if (GetModuleFileName(hDll, wModuleFile, 1024) > 0) {
      DWORD dwVersionSize = LibInst::GetInst().mGetFileVersionInfoSizeW(wModuleFile, nullptr);
      if (dwVersionSize > 0) {
        void *versionInfo = CoTaskMemAlloc(dwVersionSize);
        BOOL bVersionInfoPresent = LibInst::GetInst().mGetFileVersionInfoW(wModuleFile, 0, dwVersionSize, versionInfo);
        if (bVersionInfoPresent) {
          VS_FIXEDFILEINFO *info;
          unsigned cbInfo;
          BOOL bInfoPresent = LibInst::GetInst().mVerQueryValueW(versionInfo, TEXT("\\"), (LPVOID*)&info, &cbInfo);
          if (bInfoPresent) {
            bInfoPresent = bInfoPresent;
            uint64_t version = info->dwFileVersionMS;
            version <<= 32;
            version += info->dwFileVersionLS;
            if (version && version < 0x0001000100000000i64)
              bIncompatibleDecoder = TRUE;
          }
        }
        CoTaskMemFree(versionInfo);
      }

    }

    if (bIncompatibleDecoder) {
      FreeLibrary(hDll);
      hDll = nullptr;
      return E_FAIL;
    }

    m_hDllExtraDecoder = hDll;
  }

  DTSDecoder *context = new DTSDecoder();

  context->pDtsOpen = (DtsOpen)GetProcAddress(m_hDllExtraDecoder, "DtsApiDecOpen");
  if(!context->pDtsOpen) goto fail;

  context->pDtsClose = (DtsClose)GetProcAddress(m_hDllExtraDecoder, "DtsApiDecClose");
  if(!context->pDtsClose) goto fail;

  context->pDtsReset = (DtsReset)GetProcAddress(m_hDllExtraDecoder, "DtsApiDecReset");
  if(!context->pDtsReset) goto fail;

  context->pDtsSetParam = (DtsSetParam)GetProcAddress(m_hDllExtraDecoder, "DtsApiDecSetParam");
  if(!context->pDtsSetParam) goto fail;

  context->pDtsDecode = (DtsDecode)GetProcAddress(m_hDllExtraDecoder, "DtsApiDecodeData");
  if(!context->pDtsDecode) goto fail;

  context->dtsContext = context->pDtsOpen();
  if(!context->dtsContext) goto fail;

  context->dtsPCMBuffer = (BYTE *)av_mallocz(LAV_AUDIO_BUFFER_SIZE + AV_INPUT_BUFFER_PADDING_SIZE);

  m_DTSBitDepth = 24;
  m_DTSDecodeChannels = 8;

  m_pDTSDecoderContext = context;

  FlushDTSDecoder();

  return S_OK;
fail:
  SAFE_DELETE(context);
  FreeLibrary(m_hDllExtraDecoder);
  m_hDllExtraDecoder = nullptr;
  return E_FAIL;
}

HRESULT CLAVAudio::FreeDTSDecoder()
{
  if (m_pDTSDecoderContext)
    av_freep(&m_pDTSDecoderContext->dtsPCMBuffer);
  SAFE_DELETE(m_pDTSDecoderContext);
  return S_OK;
}

HRESULT CLAVAudio::FlushDTSDecoder(BOOL bReopen)
{
  if (m_pDTSDecoderContext) {
    if(bReopen) {
      m_pDTSDecoderContext->pDtsClose(m_pDTSDecoderContext->dtsContext);
      m_pDTSDecoderContext->dtsContext = m_pDTSDecoderContext->pDtsOpen();
    }
    m_pDTSDecoderContext->pDtsReset(m_pDTSDecoderContext->dtsContext);
    m_pDTSDecoderContext->pDtsSetParam(m_pDTSDecoderContext->dtsContext, m_DTSDecodeChannels, m_DTSBitDepth, 0, 0, 0);
  }

  return S_OK;
}

static unsigned dts_header_get_channels(DTSHeader header)
{
  if (header.IsHD && header.HDTotalChannels)
    return header.HDTotalChannels;

  if (header.ChannelLayout > 15) // user-definied layouts
    return 8;

  unsigned channels = dca_channels[header.ChannelLayout];
  if (header.XChChannelLayout)
    channels++;
  if (header.LFE)
    channels++;
  return channels;
}

static unsigned dts_determine_decode_channels(DTSHeader header)
{
  unsigned coded_channels = dts_header_get_channels(header);
  unsigned decode_channels;
  switch(coded_channels) {
  case 2:
  case 7:
  case 8:
    decode_channels = coded_channels;
    break;
  case 1:
  case 3:
  case 4:
  case 5:
    decode_channels = 6;
    break;
  case 6:
    if ((header.ChannelLayout == 9 || header.ChannelLayout == 63) && header.LFE)  // Layout 9 is 5.0, with LFE makes default 5.1, nothing special
      decode_channels = 6;
    else                                          // Other possibility for 6 channels is 6.0
      decode_channels = 7;
    break;
  default:
    DbgLog((LOG_TRACE, 10, L"DTSDecoder(): Unknown number of channels - %d!", coded_channels));
    decode_channels = 8;
    break;
  }
  return decode_channels;
}

static void DTSRemapOutputChannels(BufferDetails *buffer, DTSHeader header)
{
  const unsigned channels = dts_header_get_channels(header);
  if (channels == 1 && buffer->wChannels == 6) {                  /* DTS 1.1.0.0 produces 6 channels, with Mono in the center */
    ChannelMap map = {2};
    ChannelMapping(buffer, 1, map);
  } else if (channels == 1 && buffer->wChannels == 2) {           /* DTS 1.1.0.8 produces 2 channels, with Mono in both L/R */
    // Take the left channel, and increase volume (reduction from 2 channels)
    ExtendedChannelMap map = {{0, 2}};
    ExtendedChannelMapping(buffer, 1, map);
  } else if (channels == 3) {                                     /* --- 3 Channel Formats --- */
    if (header.ChannelLayout == 6) {                              /* 2/1/0 Layout, L+R and BC mixed into BL/BR */
      ExtendedChannelMap map = {{0, 0}, {1, 0}, {4, 2}};
      ExtendedChannelMapping(buffer, 3, map);
    } else {
      ChannelMap map = {0, 1, 2};
      ChannelMapping(buffer, 3, map);
    }
  } else if (channels == 4) {                                     /* --- 4 Channel Formats --- */
    if (header.ChannelLayout == 6) {                              /* 2/1/1 Layout, L+R+LFE and BC mixed into BL/BR */
      ExtendedChannelMap map = {{0, 0}, {1, 0}, {3, 0}, {4, 2}};
      ExtendedChannelMapping(buffer, 4, map);
    } else if (header.ChannelLayout == 8) {                       /* 2/2/0 Layout, L+R+BL+BR */
      ChannelMap map = {0, 1, 4, 5};
      ChannelMapping(buffer, 4, map);
    } else if (header.ChannelLayout == 7) {                       /* 3/1/0 Layout, L+R+C and BC mixed into BL/BR */
      ExtendedChannelMap map = {{0, 0}, {1, 0}, {2, 0}, {4, 2}};
      ExtendedChannelMapping(buffer, 4, map);
    } else {
      ChannelMap map = {0, 1, 2, 3};
      ChannelMapping(buffer, 4, map);
    }
  } else if (channels == 5) {                                     /* --- 5 Channel Formats --- */
    if (header.ChannelLayout == 8) {                              /* 2/2/1 Layout, L+R+LFE+BL+BR */
      ChannelMap map = {0, 1, 3, 4, 5};
      ChannelMapping(buffer, 5, map);
    } else if (header.ChannelLayout == 7) {                       /* 3/1/1 Layout, L+R+C+LFE and BC mixed into BL/BR */
      ExtendedChannelMap map = {{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 2}};
      ExtendedChannelMapping(buffer, 5, map);
    } else if (header.ChannelLayout == 9) {                       /* 3/2/0 Layout, L+R+C+BL+BR */
      ChannelMap map = {0, 1, 2, 4, 5};
      ChannelMapping(buffer, 5, map);
    } else {
      ChannelMap map = {0, 1, 2, 3, 4};
      ChannelMapping(buffer, 5, map);
    }
  } else if (channels == 6) {
    if (buffer->wChannels == 7) {                                 /* 3/3/0 Layout, DTS 1.1.0.0 - packed into 7 channels, empty LFE */
      ChannelMap map = {0, 1, 2, 4, 5, 6};
      ChannelMapping(buffer, 6, map);
    } else if (header.ChannelLayout == 9 && !header.LFE && header.XChChannelLayout) {
      ChannelMap map = {0, 1, 2, 4, 5};
      ChannelMapping(buffer, 5, map);
    }
  } else if (channels == 7 && buffer->wChannels == 8 && header.LFE) {       /* 3/3/1 Layout, DTS 1.1.0.8 - packed into 8 channels, BC in BL */
    ChannelMap map = {0, 1, 2, 3, 6, 7, 4};
    ChannelMapping(buffer, 7, map);
  }

  // Assign appropriate channel mask
  if (buffer->wChannels > 6) {
    if (buffer->wChannels == 7 && header.LFE)                               /* 3/3/1 (6.1) Layout */
      buffer->dwChannelMask = AV_CH_LAYOUT_5POINT1_BACK|AV_CH_BACK_CENTER;
    else if (buffer->wChannels == 7)                                        /* 3/4/0 (7.0) Layout */
      buffer->dwChannelMask = AV_CH_LAYOUT_7POINT0;
    else                                                                    /* 3/4/1 (7.1) Layout */
      buffer->dwChannelMask = AV_CH_LAYOUT_7POINT1;
  } else if (buffer->wChannels == 6 && header.ChannelLayout == 9 && !header.LFE) { /* 3/3/0 (6.0) Layout */
    buffer->dwChannelMask = AV_CH_LAYOUT_5POINT0_BACK|AV_CH_BACK_CENTER;
  } else {
    buffer->dwChannelMask = (DWORD)dca_core_channel_layout[header.ChannelLayout];
    if(header.LFE)
      buffer->dwChannelMask |= AV_CH_LOW_FREQUENCY;
  }
}

int CLAVAudio::SafeDTSDecode(BYTE *pInput, int len, BYTE *pOutput, int unk1, int unk2, int *pBitdepth, int *pChannels, int *pCoreSampleRate, int *pUnk4, int *pHDSampleRate, int *pUnk5, int *pProfile)
{
  int nPCMLen = 0;
  __try {
    nPCMLen = m_pDTSDecoderContext->pDtsDecode(m_pDTSDecoderContext->dtsContext, pInput, len, pOutput, unk1, unk2, pBitdepth, pChannels, pCoreSampleRate, pUnk4, pHDSampleRate, pUnk5, pProfile);
  } __except(EXCEPTION_EXECUTE_HANDLER) {
    DbgLog((LOG_TRACE, 50, L"::Decode() - DTS Decoder threw an exception"));
    nPCMLen = 0;
  }
  return nPCMLen;
};

static const int DTSProfiles[] = {
  FF_PROFILE_DTS, FF_PROFILE_UNKNOWN, FF_PROFILE_DTS_ES, FF_PROFILE_DTS_96_24, FF_PROFILE_UNKNOWN, FF_PROFILE_DTS_HD_HRA, FF_PROFILE_DTS_HD_MA, FF_PROFILE_DTS_EXPRESS
};

HRESULT CLAVAudio::DecodeDTS(const BYTE * pDataBuffer, int buffsize, int &consumed, HRESULT *hrDeliver)
{
  HRESULT hr = S_FALSE;
  int nPCMLength	= 0;

  BOOL bFlush = (pDataBuffer == nullptr);

  BufferDetails out;

  consumed = 0;
  while (buffsize > 0) {
    nPCMLength = 0;
    if (bFlush) buffsize = 0;

    ASSERT(m_pParser);

    BYTE *pOut = nullptr;
    int pOut_size = 0;
    int used_bytes = av_parser_parse2(m_pParser, m_pAVCtx, &pOut, &pOut_size, pDataBuffer, buffsize, AV_NOPTS_VALUE, AV_NOPTS_VALUE, 0);
    if (used_bytes < 0) {
      return E_FAIL;
    } else if(used_bytes == 0 && pOut_size == 0) {
      DbgLog((LOG_TRACE, 50, L"::Decode() - could not process buffer, starving?"));
      break;
    }

    // Timestamp cache to compensate for one frame delay the parser might introduce, in case the frames were already perfectly sliced apart
    // If we used more (or equal) bytes then was output again, we encountered a new frame, update timestamps
    if (used_bytes >= pOut_size && m_bUpdateTimeCache) {
      m_rtStartInputCache = m_rtStartInput;
      m_rtStopInputCache = m_rtStopInput;
      m_rtStartInput = m_rtStopInput = AV_NOPTS_VALUE;
      m_bUpdateTimeCache = FALSE;
    }

    if (!bFlush && used_bytes > 0) {
      buffsize -= used_bytes;
      pDataBuffer += used_bytes;
      consumed += used_bytes;
    }

    if (pOut && pOut_size > 0) {
      // Parse DTS headers
      m_bsParser.Parse(AV_CODEC_ID_DTS, pOut, pOut_size, nullptr);
      unsigned decode_channels = dts_determine_decode_channels(m_bsParser.m_DTSHeader);

      // Init Decoder with new Parameters, if required
      if (m_DTSDecodeChannels != decode_channels) {
        DbgLog((LOG_TRACE, 20, L"::Decode(): Switching to %d channel decoding", decode_channels));
        m_DTSDecodeChannels = decode_channels;

        FlushDTSDecoder();
      }

      int bitdepth = 0, channels = 0, CoreSampleRate = 0, HDSampleRate = 0, profile = 0;
      int unk4 = 0, unk5 = 0;
      nPCMLength = SafeDTSDecode(pOut, pOut_size, m_pDTSDecoderContext->dtsPCMBuffer, 0, 8, &bitdepth, &channels, &CoreSampleRate, &unk4, &HDSampleRate, &unk5, &profile);
      if (nPCMLength > 0 && bitdepth != m_DTSBitDepth) {
        int decodeBits = bitdepth > 16 ? 24 : 16;

        // If the bit-depth changed, instruct the DTS Decoder to decode to the new bit depth, and decode the previous sample again.
        if (decodeBits != m_DTSBitDepth && out.bBuffer->GetCount() == 0) {
          DbgLog((LOG_TRACE, 20, L"::Decode(): The DTS decoder indicated that it outputs %d bits, changing config to %d bits to compensate", bitdepth, decodeBits));
          m_DTSBitDepth = decodeBits;

          FlushDTSDecoder();
          nPCMLength = SafeDTSDecode(pOut, pOut_size, m_pDTSDecoderContext->dtsPCMBuffer, 0, 2, &bitdepth, &channels, &CoreSampleRate, &unk4, &HDSampleRate, &unk5, &profile);
        }
      }
      if (nPCMLength <= 0) {
        DbgLog((LOG_TRACE, 50, L"::Decode() - DTS decoding failed"));
        FlushDTSDecoder(TRUE);
        m_bQueueResync = TRUE;
        continue;
      }

      out.wChannels        = channels;
      out.dwSamplesPerSec  = HDSampleRate;
      out.sfFormat         = m_DTSBitDepth == 24 ? SampleFormat_24 : SampleFormat_16;
      out.dwChannelMask    = get_channel_mask(channels); // TODO
      out.wBitsPerSample   = bitdepth;

      int profile_index = 0;
      while(profile >>= 1) profile_index++;

      if (profile_index > 7)
        m_pAVCtx->profile =  FF_PROFILE_UNKNOWN;
      else if (profile == 0 && !m_bsParser.m_DTSHeader.HasCore)
        m_pAVCtx->profile = FF_PROFILE_DTS_EXPRESS;
      else
        m_pAVCtx->profile = DTSProfiles[profile_index];

      // TODO: get rid of these
      m_pAVCtx->sample_rate = HDSampleRate;
      m_pAVCtx->bits_per_raw_sample = bitdepth;

      // Send current input time to the delivery function
      out.rtStart = m_rtStartInputCache;
      m_rtStartInputCache = AV_NOPTS_VALUE;
      m_bUpdateTimeCache = TRUE;
    }

    // Send to Output
    if (nPCMLength > 0) {
      hr = S_OK;
      out.bBuffer->Append(m_pDTSDecoderContext->dtsPCMBuffer, nPCMLength);
      out.nSamples = out.bBuffer->GetCount() / get_byte_per_sample(out.sfFormat) / out.wChannels;

      if (m_pAVCtx->profile != (1 << 7)) {
        DTSRemapOutputChannels(&out, m_bsParser.m_DTSHeader);
      }

      m_pAVCtx->channels = out.wChannels;
      m_DecodeFormat = out.sfFormat;
      m_DecodeLayout = out.dwChannelMask;

      if (SUCCEEDED(PostProcess(&out))) {
        *hrDeliver = QueueOutput(out);
        if (FAILED(*hrDeliver)) {
          return S_FALSE;
        }
      }
    }
  }

  return hr;
}
