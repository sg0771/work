#pragma once

// Codecs supported in the LAV Audio configuration
// Codecs not listed here cannot be turned off. You can request codecs to be added to this list, if you wish.
typedef enum LAVAudioCodec {
  Codec_AAC,
  Codec_AC3,
  Codec_EAC3,
  Codec_DTS,
  Codec_MP2,
  Codec_MP3,
  Codec_TRUEHD,
  Codec_FLAC,
  Codec_VORBIS,
  Codec_LPCM,
  Codec_PCM,
  Codec_WAVPACK,
  Codec_TTA,
  Codec_WMA2,
  Codec_WMAPRO,
  Codec_Cook,
  Codec_RealAudio,
  Codec_WMALL,
  Codec_ALAC,
  Codec_Opus,
  Codec_AMR,
  Codec_Nellymoser,
  Codec_MSPCM,
  Codec_Truespeech,
  Codec_TAK,
  Codec_ATRAC,

  Codec_AudioNB            // Number of entries (do not use when dynamically linking)
} LAVAudioCodec;

// Bitstreaming Codecs supported in LAV Audio
typedef enum LAVBitstreamCodec {
  Bitstream_AC3,
  Bitstream_EAC3,
  Bitstream_TRUEHD,
  Bitstream_DTS,
  Bitstream_DTSHD,

  Bitstream_NB        // Number of entries (do not use when dynamically linking)
} LAVBitstreamCodec;


// Supported Sample Formats in LAV Audio
typedef enum LAVAudioSampleFormat {
  SampleFormat_None = -1,
  SampleFormat_16,
  SampleFormat_24,
  SampleFormat_32,
  SampleFormat_U8,
  SampleFormat_FP32,
  SampleFormat_Bitstream,

  SampleFormat_NB     // Number of entries (do not use when dynamically linking)
} LAVAudioSampleFormat;

typedef enum LAVAudioMixingMode {
  MatrixEncoding_None,
  MatrixEncoding_Dolby,
  MatrixEncoding_DPLII,

  MatrixEncoding_NB
} LAVAudioMixingMode;

// LAV Audio configuration interface
interface __declspec(uuid("4158A22B-6553-45D0-8069-24716F8FF171")) ILAVAudioSettings : public IUnknown
{

  STDMETHOD(SetRuntimeConfig)(BOOL bRuntimeConfig) = 0;

  STDMETHOD(GetDRC)(BOOL *pbDRCEnabled, int *piDRCLevel) = 0;
  STDMETHOD(SetDRC)(BOOL bDRCEnabled, int iDRCLevel) = 0;
  
  STDMETHOD_(BOOL,GetFormatConfiguration)(LAVAudioCodec aCodec) = 0;
  STDMETHOD(SetFormatConfiguration)(LAVAudioCodec aCodec, BOOL bEnabled) = 0;

  STDMETHOD_(BOOL, GetBitstreamConfig)(LAVBitstreamCodec bsCodec) = 0;
  STDMETHOD(SetBitstreamConfig)(LAVBitstreamCodec bsCodec, BOOL bEnabled) = 0;
  
  // Should "normal" DTS frames be encapsulated in DTS-HD frames when bitstreaming?
  STDMETHOD_(BOOL,GetDTSHDFraming)() = 0;
  STDMETHOD(SetDTSHDFraming)(BOOL bHDFraming) = 0;

  // Control Auto A/V syncing
  STDMETHOD_(BOOL,GetAutoAVSync)() = 0;
  STDMETHOD(SetAutoAVSync)(BOOL bAutoSync) = 0;

  // Convert all Channel Layouts to standard layouts
  // Standard are: Mono, Stereo, 5.1, 6.1, 7.1
  STDMETHOD_(BOOL,GetOutputStandardLayout)() = 0;
  STDMETHOD(SetOutputStandardLayout)(BOOL bStdLayout) = 0;
  
  // Expand Mono to Stereo by simply doubling the audio
  STDMETHOD_(BOOL,GetExpandMono)() = 0;
  STDMETHOD(SetExpandMono)(BOOL bExpandMono) = 0;

  // Expand 6.1 to 7.1 by doubling the back center
  STDMETHOD_(BOOL,GetExpand61)() = 0;
  STDMETHOD(SetExpand61)(BOOL bExpand61) = 0;

  // Allow Raw PCM and SPDIF encoded input
  STDMETHOD_(BOOL,GetAllowRawSPDIFInput)() = 0;
  STDMETHOD(SetAllowRawSPDIFInput)(BOOL bAllow) = 0;

  // Configure which sample formats are enabled
  // Note: SampleFormat_Bitstream cannot be controlled by this
  STDMETHOD_(BOOL,GetSampleFormat)(LAVAudioSampleFormat format) = 0;
  STDMETHOD(SetSampleFormat)(LAVAudioSampleFormat format, BOOL bEnabled) = 0;

  // Configure a delay for the audio
  STDMETHOD(GetAudioDelay)(BOOL *pbEnabled, int *pDelay) = 0;
  STDMETHOD(SetAudioDelay)(BOOL bEnabled, int delay) = 0;

  // Enable/Disable Mixing
  STDMETHOD(SetMixingEnabled)(BOOL bEnabled) = 0;
  STDMETHOD_(BOOL,GetMixingEnabled)() = 0;

  // Control Mixing Layout
  STDMETHOD(SetMixingLayout)(DWORD dwLayout) = 0;
  STDMETHOD_(DWORD,GetMixingLayout)() = 0;

#define LAV_MIXING_FLAG_UNTOUCHED_STEREO 0x0001
#define LAV_MIXING_FLAG_NORMALIZE_MATRIX 0x0002
#define LAV_MIXING_FLAG_CLIP_PROTECTION  0x0004
  // Set Mixing Flags
  STDMETHOD(SetMixingFlags)(DWORD dwFlags) = 0;
  STDMETHOD_(DWORD,GetMixingFlags)() = 0;

  // Set Mixing Mode
  STDMETHOD(SetMixingMode)(LAVAudioMixingMode mixingMode) = 0;
  STDMETHOD_(LAVAudioMixingMode,GetMixingMode)() = 0;

  // Set Mixing Levels
  STDMETHOD(SetMixingLevels)(DWORD dwCenterLevel, DWORD dwSurroundLevel, DWORD dwLFELevel) = 0;
  STDMETHOD(GetMixingLevels)(DWORD *dwCenterLevel, DWORD *dwSurroundLevel, DWORD *dwLFELevel) = 0;

  // Toggle Dithering for sample format conversion
  STDMETHOD(SetSampleConvertDithering)(BOOL bEnabled) = 0;
  STDMETHOD_(BOOL,GetSampleConvertDithering)() = 0;

  // Suppress sample format changes. This will allow channel count to increase, but not to reduce, instead adding empty channels
  // This option is NOT persistent
  STDMETHOD(SetSuppressFormatChanges)(BOOL bEnabled) = 0;
  STDMETHOD_(BOOL, GetSuppressFormatChanges)() = 0;

  // Use 5.1 legacy layout (using back channels instead of side)
  STDMETHOD_(BOOL, GetOutput51LegacyLayout)() = 0;
  STDMETHOD(SetOutput51LegacyLayout)(BOOL b51Legacy) = 0;
};


