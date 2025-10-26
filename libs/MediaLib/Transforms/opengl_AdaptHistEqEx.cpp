

#include <avisynth/avisynth.h>
#include <string>
#include "opengl_SimpleGLFilter.h"
#include "ImageProcess/Filters_OpenGLFilter.h"
#include "ImageProcess/reflector_Shader.h"

#include "opengl_AdaptHistEqExShaders.h"
#include "utils.hpp"

struct AdaptHistEqExContext
{
	float degree;
};

class AdaptHistEqExShader : public Shader
{
public:

	AdaptHistEqExShader(const std::string& vertexsource, const std::string& fragsource) :Shader(vertexsource, fragsource)
	{
	}
	void ApplyParameter()
	{
		SetUniform1f("degree", context.degree);
	}
	void SetParameter(void* param)
	{
		context = *(AdaptHistEqExContext*)param;
	}
private:
	AdaptHistEqExContext context;
};


class AdaptHistEqEx : public SimpleGLFilter
{
public:
	AdaptHistEqEx(const PClip& child,
		int start, int end, int width, int height, int threshold, float degree, IScriptEnvironment* env)
		: SimpleGLFilter(child, { histogramRes, laplacian_sharp }, start, end, env)
	{
		param.degree = degree;
		_threshold = threshold;
	}
	~AdaptHistEqEx();
	void InitProcess() override;


	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;
private:
	AdaptHistEqExContext param;
	int _threshold;
private:
	void process(BYTE* imageA, long imagesize, BYTE* lut);
	void histogramEqulize_simple(BYTE* pimg, int biSizeImage, BYTE* lut);
};



AVSValue __cdecl Create_AdaptHistEqEx(AVSValue args, void* user_data, IScriptEnvironment* env) {
	//WX//WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);
	return new AdaptHistEqEx(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(200000000),
		args[3].AsInt(0), args[4].AsInt(0), args[5].AsInt(0), args[6].AsFloat(0.0f), env);
}

AdaptHistEqEx::~AdaptHistEqEx()
{
}

void AdaptHistEqEx::InitProcess()
{
	processor = new OpenGLFilter();
	processor->AddShader(new AdaptHistEqExShader(vertexshaderstr, fragshaderstrs[0]));
	processor->AddShader(new AdaptHistEqExShader(vertexshaderstr, fragshaderstrs[1]));
	processor->SetParameter(&param);
};


PVideoFrame __stdcall AdaptHistEqEx::GetFrame(int n, IScriptEnvironment* env)
{
	if (!processor)
		utils::MutexQueueImp::OpenglInstance()->sync([this] {InitProcess(); });

	if (!processor)
		return base::GetFrame(n, env);;

	const void* data = child->GetFrame(n, env)->GetReadPtr();

	// 计算直方图，获得均衡后的图像
	BYTE* tempImage = new BYTE[vi.width * vi.height * 4];
	memcpy(tempImage, data, vi.width * vi.height * 4);

	BYTE LUT[256] = { 0 };
	process(tempImage, vi.width * vi.height, LUT);

	auto dst = env->NewVideoFrame(vi);

	if (dst == nullptr || dst.m_ptr == nullptr) {
		return nullptr;
	}
	void* post_data = (void*)dst->GetWritePtr();

	utils::MutexQueueImp::OpenglInstance()->sync([&] {
		std::vector<FilterFrame> frames;
		frames.push_back({ (unsigned char*)data, vi.width , vi.height });
		frames.push_back({ (unsigned char*)LUT, 256, 1 });
		processor->Render(frames, vi.width, vi.height, post_data);
		});

	delete[] tempImage;
	return dst;
}

void AdaptHistEqEx::process(BYTE* imageA, long imagesize, BYTE* lut) {
	UINT hist[256] = { 0 };
	for (BYTE* p = imageA; p <= imageA + imagesize * 4; p += 4)
	{
		++*(hist + *p);
		++*(hist + p[1]);
		++*(hist + p[2]);
	}
	//裁剪操作 
	int average = imagesize * 3 / 255;
	int LIMIT = _threshold * average;
	int steal = 0;
	for (int k = 0; k < 256; k++)
	{
		if (hist[k] > LIMIT) {
			steal += hist[k] - LIMIT;
			hist[k] = LIMIT;
		}
	}
	int bonus = steal / 256;
	//hand out the steals averagely  
	for (int k = 0; k < 256; k++)
	{
		hist[k] += bonus;
	}

	int sum = 0;
	for (int i = 0; i < 256; i++)
	{
		sum += hist[i];
		lut[i] = ((float)sum * 255.0) / (imagesize * 3);
	}
	//for (BYTE* p = imageA; p <= imageA + imagesize * 4; p += 4) {
	//	*p = lut[*p];
	//	p[1] = lut[p[1]];
	//	p[2] = lut[p[2]];
	//}
}


void AdaptHistEqEx::histogramEqulize_simple(BYTE* pimg, int biSizeImage, BYTE* LUT)
{

	//float LUTf[256];
	BYTE* pcur, * pend = pimg + biSizeImage * 4;
	unsigned int hist[256];// , LUT[256];
	memset(hist, 0, sizeof(hist));
	for (pcur = pimg + 2; pcur < pend; pcur += 4) {
		hist[(int)*pcur]++;
	}

	//裁剪操作  
	int average = biSizeImage / 256;
	int LIMIT = _threshold * average;
	int steal = 0;
	for (int k = 0; k < 256; k++) {
		if (hist[k] > LIMIT) {
			steal += hist[k] - LIMIT;
			hist[k] = LIMIT;
		}
	}
	int bonus = steal / 256;
	for (int k = 0; k < 256; k++) {
		hist[k] += bonus;
	}

	double sum = 0;
	for (int i = 0; i < 256; ++i) {
		sum += hist[i];
		LUT[i] = 256 * (sum / biSizeImage);
	}
	//for (pcur = pimg + 2; pcur < pend; pcur += 4) {
	//	*(pcur) = LUT[(int)*pcur];
	//}
}
