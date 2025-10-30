
#ifndef FFAVSSOURCES_H
#define FFAVSSOURCES_H

#include <vector>
#include <windows.h>
#include <avisynth\avisynth.h>
#include "ffms.h"

struct ErrorInfo : FFMS_ErrorInfo {
    char ErrorBuffer[1024];
    ErrorInfo() {
        Buffer = ErrorBuffer;
        BufferSize = sizeof(ErrorBuffer);
    }
};

class AvisynthVideoSource : public IClip {
    struct FrameFields {
        int Top;
        int Bottom;
    };
    int m_nID = 0;
    VideoInfo VI;
	//微秒级别
	int audiolength = 0;
    bool HighBitDepth = false;
   
    int FPSNum = 0;
    int FPSDen = 0;
    int RFFMode = 0;

	
	int Track = 0;
	int Threads = 0;
	int SeekMode = 0;

    std::vector<FrameFields> FieldList;
    const char *VarPrefix = NULL;
	int ResizeToWidth = 0;
	int ResizeToHeight = 0;
	char* ResizerName = NULL;
	char* ConvertToFormatName = NULL;
	int adjustednum_frames = 0;
	int original_frames = 0;
	

    void InitOutputFormat(int ResizeToWidth, int ResizeToHeight,
        const char *ResizerName, const char *ConvertToFormatName, FFMS_VideoSource* V, IScriptEnvironment *Env);
    void OutputFrame(const FFMS_Frame *Frame, PVideoFrame &Dst, IScriptEnvironment *Env);
    void OutputField(const FFMS_Frame *Frame, PVideoFrame &Dst, int Field, IScriptEnvironment *Env);
public:
	//FFMS_VideoSource *V;
	FFMS_Index * m_Index = NULL;
	std::string  m_strUTF8 = "";
    std::wstring m_strUTF16 = L"";
	bool Revert = false;
    AvisynthVideoSource(int id, const char *SourceFile, int Track, FFMS_Index *Index,
        int FPSNum, int FPSDen, int Threads, int SeekMode, int RFFMode,
        int ResizeToWidth, int ResizeToHeight, const char *ResizerName,
        const char *ConvertToFormatName, const char *VarPrefix, IScriptEnvironment* Env);
    virtual  ~AvisynthVideoSource();
    bool __stdcall GetParity(int n);
    intptr_t __stdcall SetCacheHints(int cachehints, int frame_range) { return 0; }
    const VideoInfo& __stdcall GetVideoInfo() { return VI; }
    void __stdcall GetAudio(void* Buf, __int64 Start, __int64 Count, IScriptEnvironment *Env) {}
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment *Env);
	void SupportRevert(bool flag);
	void  ClearFFMS_VideoSource();
};

class AvisynthAudioSource : public IClip {
    VideoInfo m_VI;
	int m_Track = 0;
	FFMS_Index* m_Index = nullptr;
	int m_AdjustDelay = 0;
	std::string m_VarPrefix = "";
public:
	std::string  m_strUTF8 = "";
    std::wstring m_strUTF16 = L"";
    AvisynthAudioSource(const char *SourceFile, int Track, FFMS_Index *Index,
        int AdjustDelay, const char *VarPrefix, IScriptEnvironment* Env);
    ~AvisynthAudioSource();
    bool __stdcall GetParity(int n) { return false; }
    intptr_t __stdcall SetCacheHints(int cachehints, int frame_range) { return 0; }
    const VideoInfo& __stdcall GetVideoInfo() { return m_VI; }
    void __stdcall GetAudio(void* Buf, __int64 Start, __int64 Count, IScriptEnvironment *Env);
    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment *Env) { return nullptr; };
};

#endif
