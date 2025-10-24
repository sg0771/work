#pragma once

#include <Windows.h>
#include <stdbool.h>
#include <stdint.h> 
#include <stdlib.h>

#ifdef __cplusplus
#include <WXBase.h>
#endif

#ifdef MEDIALIB_EXPORTS
#define MEDIALIB_API EXTERN_C __declspec(dllexport)
#else
#define MEDIALIB_API EXTERN_C __declspec(dllimport)
#endif

#define NAME_LEN 256
#define ML_COUNT 16

enum MediaType{	WX_Video,
	WX_Image,
	WX_Audio,
	WX_Blank
};

enum MLAudioEffectType {
	AE_None,
	AE_Man,
	AE_Woman,
	AE_Monster,
	AE_Minions,
	AE_Echo
};

enum RotateFlipType {
	//
		 // 摘要:
		 //   指定没有顺时针旋转和翻转。
	RotateNoneFlipNone = 0,
	//
	// 摘要:
	//   指定后接水平和垂直的 180 度顺时针旋转翻转。
	Rotate180FlipXY = 0,
	//
	// 摘要:
	//   指定不进行翻转顺时针旋转 90 度。
	Rotate90FlipNone = 1,
	//
	// 摘要:
	//   指定后接水平和垂直的 270 度顺时针旋转翻转。
	Rotate270FlipXY = 1,
	//
	// 摘要:
	//   指定不进行翻转 180 度的顺时针旋转。
	Rotate180FlipNone = 2,
	//
	// 摘要:
	//   指定没有顺时针旋转后接水平和垂直翻转。
	RotateNoneFlipXY = 2,
	//
	// 摘要:
	//   指定不进行翻转顺时针旋转 270 度。
	Rotate270FlipNone = 3,
	//
	// 摘要:
	//   指定旋转 90 度顺时针旋转后接水平和垂直翻转。
	Rotate90FlipXY = 3,
	//
	// 摘要:
	//   指定没有跟水平翻转的顺时针旋转。
	RotateNoneFlipX = 4,
	//
	// 摘要:
	//   指定垂直翻转后跟 180 度顺时针旋转。
	Rotate180FlipY = 4,
	//
	// 摘要:
	//   指定后接水平翻转的 90 度的顺时针旋转。
	Rotate90FlipX = 5,
	//
	// 摘要:
	//   指定垂直翻转后跟的 270 度顺时针旋转。
	Rotate270FlipY = 5,
	//
	// 摘要:
	//   指定后接水平翻转的 180 度顺时针旋转。
	Rotate180FlipX = 6,
	//
	// 摘要:
	//   指定垂直翻转后跟没有顺时针旋转。
	RotateNoneFlipY = 6,
	//
	// 摘要:
	//   指定后接水平翻转的 270 度顺时针旋转。
	Rotate270FlipX = 7,
	//
	// 摘要:
	//   指定垂直翻转后跟 90 度顺时针旋转。
	Rotate90FlipY = 7

};

typedef struct Stream_Info {
	int index;
	int codec_type; // enum AVMediaType
	int SampleRate;
	char channel_layout[NAME_LEN];
	char language[NAME_LEN];
	char title[NAME_LEN];
	char codec_name[NAME_LEN];
	int64_t BitRate;
	double FrameRate;
	int width;
	int height;
} Stream_Info;

#ifdef __cplusplus
struct AssContext {
	char path[MAX_PATH * 4]; //UTF8
	char font[MAX_PATH * 4];
	float offset;
	float start;
	float end;
	static std::vector<AssContext> GetAssContexts();
};
#endif

typedef struct WX_D3DXVECTOR4
{
#ifdef __cplusplus
	FLOAT x = 0.0, y = 0.0, z = 0.0, w = 0.0;
#else
	FLOAT x, y, z, w;
#endif
} WX_D3DXVECTOR4, * LPWX_D3DXVECTOR4;

typedef struct stDeLogoContext
{
#ifdef __cplusplus
	int count = 0;// 马赛克个数
	int mod[ML_COUNT] = { 0 }; // 马赛克模式
	float degree[ML_COUNT] = { 0 };// 马赛克强度
	float alpha[ML_COUNT] = { 0 };// 马赛克透明度
	WX_D3DXVECTOR4 rects[ML_COUNT] = { {0} };// 矩形
#else
	int count;// 马赛克个数
	int mod[ML_COUNT]; // 马赛克模式
	float degree[ML_COUNT];// 马赛克强度
	float alpha[ML_COUNT];// 马赛克透明度
	WX_D3DXVECTOR4 rects[ML_COUNT];// 矩形
#endif
}DeLogoContext;

typedef struct ChromaKeyContext
{
#ifdef __cplusplus
	int color = -1;    // 背景色中比较亮的颜色 0x000000~0xffffff 
	int darkColor = 0x488D04;  // 背景色中比较暗的颜色 暂定为0x488D04
	float _Threshhold = 0.25f;// 抠除程度 范围0-1 推荐值0.25
	float _BlurSize = 0.01f; // 边缘模糊程度 范围0-1 推荐值0.01
#else
	int color;    // 背景色中比较亮的颜色 0x000000~0xffffff 
	int darkColor;  // 背景色中比较暗的颜色 暂定为0x488D04
	float _Threshhold;// 抠除程度 范围0-1 推荐值0.25
	float _BlurSize; // 边缘模糊程度 范围0-1 推荐值0.01
#endif
}ChromaKeyContext;

typedef struct ColorAdjustContext
{
#ifdef __cplusplus
	float hue = 0.0f;
	float sat = 0.0f;
	float bright = 0.0f;
	float cont = 0.0f;
	float highlights = 0.0f;//
	float shadows = 0.0f; //阴影
#else
	float hue;
	float sat;
	float bright;
	float cont;
	float highlights;//
	float shadows; //阴影
#endif
}ColorAdjustContext;

typedef struct ImageEnhanceContext
{
#ifdef __cplusplus
	int enable = 0;  //使能
	int threshold = 0; //阈值
	float degree = 0; //角度
#else
	int enable;  //使能
	int threshold; //阈值
	float degree; //角度
#endif
}ImageEnhanceContext;

typedef struct LutContext
{
#ifdef __cplusplus
	LutContext()
	{
		memset(lutFilePath, 0x00, MAX_PATH * sizeof(char));
	}
#endif
	char lutFilePath[MAX_PATH]; //LUT文件路径
}LutContext;

typedef struct PreviewContext
{
	ImageEnhanceContext imageenhance;
	ColorAdjustContext	coloradjust;
	LutContext			lut;
}PreviewContext;

typedef struct MediaInfo {
#ifdef __cplusplus
	int Width = 0;
	int Height = 0;
	double FrameRate = 0;
	int TotalFrames = 0;
	double Duration = 0;
	double Position = 0;
	int Volume = 0;
	int Paused = 0;
#else
	int Width;
	int Height;
	double FrameRate;
	int TotalFrames;
	double Duration;
	double Position;
	int Volume;
	int Paused;
#endif
}MediaInfo;

typedef struct TransitionContext
{
#ifdef __cplusplus
	TransitionContext() {
		memset(name, 0x00, MAX_PATH * sizeof(char));
		memset(method, 0x00, MAX_PATH * sizeof(char));
		duration = 0.0f;
		factor = 0.0f;
	}
#endif
	char name[MAX_PATH];
	char method[MAX_PATH];//转场类型,分为3类, 图片转场, opengl转场, 原生转场
	float duration;
	float factor;
}TransitionContext;

typedef struct MotionContext
{
#ifdef __cplusplus
	MotionContext() {
		memset(name, 0x00, MAX_PATH * sizeof(char));
		content = nullptr;
		start = 0.0f;
		end = 0.0f;
	}
#endif
	char name[MAX_PATH];
	char* content;
	float start;
	float end;
}MotionContext;

typedef struct RECTF
{
#ifdef __cplusplus
	RECTF(int a, int b, int c, int d) {
		left = a;
		top = b;
		right = c;
		bottom = d;
	}
	RECTF(float a, float b, float c, float d) {
		left = a;
		top = b;
		right = c;
		bottom = d;
	}
	RECTF() {
		left = 0.0f;
		top = 0.0f;
		right = 0.0f;
		bottom = 0.0f;
	}
#endif
	float  left;
	float	 top;
	float  right;
	float  bottom;
} RECTF;

typedef struct ZoomContext
{
	RECTF rect;
#ifdef __cplusplus
	float start = 0;
	float end = 0;
#else
	float start;
	float end;
#endif
}ZoomContext;

typedef struct MosaicContext
{
#ifdef __cplusplus
	int count = 0;// 马赛克个数
	int mod[ML_COUNT] = { 0 }; // 马赛克模式
	float degree[ML_COUNT] = { 0 };// 马赛克强度
	float alpha[ML_COUNT] = { 0 };// 马赛克透明度
#else
	int count;// 马赛克个数
	int mod[ML_COUNT]; // 马赛克模式
	float degree[ML_COUNT];// 马赛克强度
	float alpha[ML_COUNT];// 马赛克透明度
#endif
}MosaicContext;

typedef struct MediaInfomation {
	double rotate;
	int width;
	int height;
	int64_t duration;
	int videocount;
	int audiocount;
	char format[NAME_LEN];
	char fname[NAME_LEN];
	char acname[NAME_LEN];
	bool vfr;
	int64_t vbitrate;
	float framerate;
	int64_t abitrate;
	int samplerate;
	int nb_channels;
	char channel_layout[NAME_LEN];
	//char* path;
	bool interlaced;//add
} MediaInfomation;

typedef struct MediaHeader
{
#ifdef __cplusplus
	char path[MAX_PATH * 4] = { 0 }; //UTF8
	enum MediaType type = WX_Video;
	float start = 0.0;
	float end = 0.0;
	INT32 volume = 100;
	float fadein = 0.0f;
	float fadeout = 0.0f;
	float speed = 1.0f;
	int externangle = 0;
	MediaInfomation* info = nullptr;
	bool audioenhance = false;
	enum MLAudioEffectType AudioEffect = AE_None;
#else
	char path[MAX_PATH * 4]; //UTF8
	enum MediaType type;
	float start;
	float end;
	INT32 volume;
	float fadein;
	float fadeout;
	float speed;
	int externangle;
	MediaInfomation* info;
	bool audioenhance;
	enum MLAudioEffectType AudioEffect;
#endif
}MediaHeader;

struct AVFrame;

MEDIALIB_API HRESULT RenderFrame(struct AVFrame* frame, bool paused);

MEDIALIB_API HRESULT ForceRefresh();

MEDIALIB_API void* GetFrontSurface();

MEDIALIB_API HRESULT ResetFilterParamenter();

MEDIALIB_API HRESULT SetChromaParamenter(ChromaKeyContext context);

MEDIALIB_API HRESULT SetColorAdjustParamenter(ColorAdjustContext context);

MEDIALIB_API HRESULT SetImageEnhanceParamenter(ImageEnhanceContext context);

MEDIALIB_API HRESULT SetParameter(float Saturation, float Hue, float Brightness, float Contrast, float Hightlights, float Shadows);

MEDIALIB_API HRESULT SetPreviewParameter(PreviewContext context);

MEDIALIB_API HRESULT SetPreviewAss(char* context);

MEDIALIB_API HRESULT SetCacheAss(char* context);

MEDIALIB_API HRESULT SetDeLogoParameter(DeLogoContext context);

MEDIALIB_API RECT  GetAssPreviewSize(int width, int height, float ts, char* name, char* content);

MEDIALIB_API RECT  GetAssEventSize(int framewidth, int frameheight, float ts, char* name);

MEDIALIB_API RECT  GetAssCursorRect(int width, int height, float ts, int index, int cursorindex);

MEDIALIB_API HRESULT InitDevice(BOOL mode);

typedef struct VideoContextBase {
#ifdef __cplusplus
	struct MediaHeader header;
	enum RotateFlipType rotateFlip = RotateNoneFlipNone;
	RECTF crop;
	RECTF rect;
	ZoomContext zooms[ML_COUNT];
	RECTF mosaic[ML_COUNT];
	MosaicContext mosaicCtx;
	MotionContext motion;
	TransitionContext transition;
	ColorAdjustContext coloradjust;
	ChromaKeyContext chromekey;
	float offset = 0;
	char maskpath[MAX_PATH] = { 0 };//UTF8
	RECTF maskrect;
	bool AutoLoop = false;
#else
	struct MediaHeader header;
	enum RotateFlipType rotateFlip;
	RECTF crop;
	RECTF rect;
	ZoomContext zooms[ML_COUNT];
	RECTF mosaic[ML_COUNT];
	MosaicContext mosaicCtx;
	MotionContext motion;
	TransitionContext transition;
	ColorAdjustContext coloradjust;
	ChromaKeyContext chromekey;
	float offset;
	char maskpath[MAX_PATH];
	RECTF maskrect;
	bool AutoLoop;
#endif
}VideoContextBase;

#ifdef __cplusplus

typedef struct VideoContext : VideoContextBase {
	int _innerZooms = 0;//not used
	int _innerMosaics = 0;//not used
}VideoContext;

typedef struct SubVideoContext : VideoContextBase {
	int splitindex = 0;
	int _innerZooms = 0;//not used
#ifdef _M_IX86
	int _innerMosaics = 0;//not used
#endif
}SubVideoContext;

//typedef struct VideoContextEx : VideoContext {
//	std::vector<ZoomContext>* innerZooms = nullptr;
//	std::vector<RECTF>* innerMosaics = nullptr;
//}VideoContextEx;

//typedef struct SubVideoContextEx : SubVideoContext {
//	std::vector<ZoomContext>* innerZooms = nullptr;
//	std::vector<RECTF>* innerMosaics = nullptr;
//}SubVideoContextEx;

#endif



typedef struct TextVideoContext{
	struct MediaHeader header;
	RECTF rect;
	float offset;
	char maskpath[MAX_PATH];//UTF8
	MotionContext motion;
	float FreezePoint;
}TextVideoContext;

typedef struct AudioContext{
	struct MediaHeader header;
	float offset;
	int splitindex;
}AudioContext;

//type:
//0: glfilter
//1: innerfilter
//2: 3dlutfilter
//3: gllutfilter
typedef struct FilterContext
{
	char path[MAX_PATH];
	int type;
	float offset;
	float duration;
	int grade;
}FilterContext;

typedef struct OverlayContext
{
	char name[MAX_PATH];
	float offset;
	float duration;
}OverlayContext;

typedef struct VideoState VideoState;
typedef struct Onset Onset;
MEDIALIB_API INT32 SetTextVideoTrack(TextVideoContext datas[], int length); //No use

#ifdef __cplusplus
MEDIALIB_API INT32 SetOverlayTrack(SubVideoContext datas[], int length);//设置蒙版轨道，无效

MEDIALIB_API INT32 SetAudioTrack(AudioContext datas[], int length);//设置音频轨道
MEDIALIB_API INT32 SetRevertVideoContext(VideoContext videoContext);//单独操作
MEDIALIB_API INT32 SetVideoTrack(VideoContext datas[], int length);//设置视频主轨道素材
MEDIALIB_API INT32 SetSubVideoTrack(SubVideoContext datas[], int length);//副轨道素材
MEDIALIB_API INT32 SetFilterTrack(FilterContext datas[], int length);//设置滤镜轨道
MEDIALIB_API INT32 SetAssTrack(AssContext datas[], int length);//设置字幕轨道
#endif

//Medialib 库的初始化
MEDIALIB_API int Initialize(const char* _company, const char* _product, bool mode);

//时间轴初始化 
MEDIALIB_API INT32 TimelineSetting(int FrameRate, int FrameWidth, int FrameHeight, int SampleRate, int channels, int color, char* bgimage
#ifdef __cplusplus
	= (char*)""
#endif
	, bool isexport 
#ifdef __cplusplus
	= false
#endif
);

//UTF8 
MEDIALIB_API MediaInfomation* AnalyzeMedia(char* utf8_path);//解析音视频
MEDIALIB_API MediaInfomation* AnalyzeMedia2(char* utf8_path);//解析音视频

MEDIALIB_API HRESULT FFMpegCommand(char* utf8_cmds);//导出
MEDIALIB_API HRESULT CaptureImage(char* utf8_path, float second, wchar_t* output, int height);//截取一张图
MEDIALIB_API HRESULT CaptureImages(char* utf8_path, INT64* seconds, int size, wchar_t* output);//截取多张图
MEDIALIB_API HRESULT SplitAudio(char* filename, char* output);//获取波形图

//创建播放器
//建议后面改成如果不修改主轨道
MEDIALIB_API VideoState* OpenMedia(char* utf8_path, BOOL audiodisable, BOOL paused, float seeksecond);
MEDIALIB_API HRESULT Resume(VideoState* is);
MEDIALIB_API HRESULT Pause(VideoState* is);
MEDIALIB_API HRESULT SetPlayField(VideoState* is, char* field, char* value);
MEDIALIB_API HRESULT CloseMedia(VideoState* is);
MEDIALIB_API HRESULT Play(VideoState* is);
MEDIALIB_API HRESULT Seek(VideoState* is, double second);
MEDIALIB_API HRESULT NextFrame(VideoState* is);
MEDIALIB_API HRESULT PreFrame(VideoState* is);

MEDIALIB_API void RaiseCrash();


MEDIALIB_API HRESULT SetField(char* field, char* value);
MEDIALIB_API HRESULT DetectAudio(char* filename, char* output);

typedef struct AVS_Value AVS_Value;
struct AVS_Value {
	union {
		void* clip; // do not use directly, use avs_take_clip
		char boolean;
		intptr_t integer;
		float floating_pt;
		const char* string;
		const AVS_Value* array;
	} d;
	char type; // 'a'rray, 'c'lip, 'b'ool, 'i'nt, 'f'loat, 's'tring, 'v'oid, or 'l'ong
	short array_size;
};

MEDIALIB_API AVS_Value BuildTimeline();

struct AVS_ScriptEnvironment;
MEDIALIB_API HRESULT CaptureHEIC(wchar_t* filename, wchar_t* output);//图像文件转换
typedef void(WINAPI* ErrorLogCallBack) (char* info, int code);
typedef int(WINAPI* NormalCallBack) (void* is);
typedef int(WINAPI* OpenCallBack) (void* is, MediaInfo mi);
typedef int(WINAPI* RenderCallBack) (void* surface, MediaInfo mi);
typedef int(WINAPI* StateChangedCallBack) (int state);

typedef struct ProgressInfo {
	int remaining_time;
	int progress;//转换进度
}ProgressInfo;

typedef int(WINAPI* ProgressCallBack) (ProgressInfo* surface);

typedef struct ConvertState {
	int state; //0:progress, 1:Pause, 2:Stop,
	ProgressCallBack callback;
}ConvertState;

MEDIALIB_API const char* GetEnvPath(const char* filename);

//opengl filter
MEDIALIB_API void ImageProcessGetOpenGLLogs(char* str, int len);
MEDIALIB_API int ImageProcessInitialize();
MEDIALIB_API void ImageProcessDeinitialize();

MEDIALIB_API void Clean();


MEDIALIB_API int64_t FFMS_KeyPTS(void* _index, int64_t pts);

MEDIALIB_API void* BlackSurface();

MEDIALIB_API HRESULT SetErrorLogCallback(ErrorLogCallBack callback);

MEDIALIB_API HRESULT SetRenderCallback(VideoState* is, RenderCallBack callback);
MEDIALIB_API HRESULT SetOpenCallbackAsync(VideoState* is, OpenCallBack callback);
MEDIALIB_API HRESULT SetStateChangedCallbackAsync(VideoState* is, StateChangedCallBack callback);
MEDIALIB_API HRESULT GetMediaInfo(VideoState* videostate, MediaInfo* mi);
MEDIALIB_API HRESULT SetVolume(VideoState* is, int volume);
MEDIALIB_API HRESULT SetProgressCallback(ProgressCallBack callback);
MEDIALIB_API HRESULT SetVideoFilter(VideoState* is, char* filter);
MEDIALIB_API HRESULT SetAudioFilter(VideoState* is, char* filter);
MEDIALIB_API void ProcessFilterCommand(VideoState* is, char* command);
MEDIALIB_API HRESULT SetRenderHwnd(VideoState* is, HWND hwnd);
MEDIALIB_API HRESULT SetConvertState(int state);

MEDIALIB_API Stream_Info* GetStreamInfo(char* path, INT32* psize, int64_t* duration);

MEDIALIB_API int BuildIndex(char* source, char* cachefile);
MEDIALIB_API HRESULT StopCaptureThumb();
MEDIALIB_API AVS_Value BuildTimeline();
MEDIALIB_API INT64 GetAssDuration(char* path);
MEDIALIB_API HRESULT SetCrashReportPath(WCHAR* path);
MEDIALIB_API int InitExceptionFilter(const char* _dumppath);

//釋放資源
MEDIALIB_API void ClearSource();

//Test Player
MEDIALIB_API void* RenderCreate(HWND hwnd);

MEDIALIB_API void RenderDestroy(void* obj);

//渲染视频帧
MEDIALIB_API void RenderDraw(void* obj, struct AVFrame* frame);

MEDIALIB_API void RenderDrawData(void* p, int bRGB32, int width, int height, uint8_t* buf, int pitch);

