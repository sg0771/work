
#include "stb_image.h"

#include "utils.hpp"

#pragma once
#include <avisynth/avisynth.h>
#include <string>
#include "opengl_SimpleGLFilter.h"
#include "ImageProcess/Filters_OpenGLFilter.h"
#include "ImageProcess/reflector_Shader.h"

const std::string lut_fs_3D = R"!!(
void main() {
	vec4 srcColor = texture2D(iChannel[7], v_TexCoord);
	vec4 newColor1 = texture3D(iChannel_3D, srcColor.bgr);
	color = newColor1;
}
)!!";

class lut : public SimpleGLFilter
{
public:
	lut(const PClip& child,
		int start, int end, int width, int height, std::string lutFilePath, IScriptEnvironment* env);

	~lut();
	void InitProcess() override;

	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env) override;
private:
	std::shared_ptr<unsigned char> m_pLutData;
	int m_lutWidth;
	int m_lutHeight;
};



 AVSValue __cdecl Create_lut(AVSValue args, void* user_data, IScriptEnvironment* env) {
	//WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);
	return new lut(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(200000000),
		args[3].AsInt(0), args[4].AsInt(0), args[5].AsString(""), env);
}


std::vector<std::string> vStringSplit(const  std::string& s, const std::string& delim = ",")
{
	std::vector<std::string> elems;
	size_t pos = 0;
	size_t len = s.length();
	size_t delim_len = delim.length();
	if (delim_len == 0) return elems;
	while (pos < len)
	{
		int find_pos = s.find(delim, pos);
		if (find_pos < 0)
		{
			elems.push_back(s.substr(pos, len - pos));
			break;
		}
		elems.push_back(s.substr(pos, find_pos - pos));
		pos = find_pos + delim_len;
	}
	return elems;
}


lut::lut(const PClip& child, int start, int end, int width, int height, std::string lutFilePath, IScriptEnvironment* env)
	: SimpleGLFilter(child, { lut_fs_3D }, start, end, env), m_lutWidth(1), m_lutHeight(1)
{
	int nchannel = 0;
	m_pLutData.reset(stbi_load(lutFilePath.c_str(), &m_lutWidth, &m_lutHeight, &nchannel, 0));

	auto p = m_pLutData.get();
	for (int i = 0; i < m_lutWidth * m_lutHeight * 4; i += 4) {
		auto temp = p[i + 2];
		p[i + 2] = p[i];
		p[i] = temp;
	}
}

lut::~lut()
{
	stbi_image_free(m_pLutData.get());
}

void lut::InitProcess()
{
	processor = new OpenGLFilter(7);
	for (size_t i = 0; i < fragshaderstrs.size(); i++)
	{
		processor->AddShader(new Shader(vertexshaderstr, fragshaderstrs[i]));
	}
}

PVideoFrame __stdcall lut::GetFrame(int n, IScriptEnvironment* env)
{
	if (n < _frame_start || n > _frame_end) {
		auto src = base::GetFrame(n, env);

		return src;
	}

	if(clips.empty())
		return base::GetFrame(n, env);

	auto dst = env->NewVideoFrame(vi);
	if (dst == nullptr || dst.m_ptr == nullptr) {
		return nullptr;
	}
	if (!processor)
		utils::MutexQueueImp::OpenglInstance()->sync([this] {InitProcess(); });


	if (!processor)
		return base::GetFrame(n, env);;

	float duration = _frame_count * vi.fps_denominator / static_cast<double>(vi.fps_numerator);
	float time = (n - _frame_start) * vi.fps_denominator / static_cast<double>(vi.fps_numerator);
	processor->UpdateProgress(duration, time);

	void* data = (void*)dst->GetWritePtr();
	std::vector<FilterFrame> frames;
	PVideoFrame pvideoframes[2];
	pvideoframes[0] = clips[0]->GetFrame(n, env);
	// 原始图片数据
	frames.push_back({ (unsigned char*)pvideoframes[0]->GetReadPtr(), clips[0]->GetVideoInfo().width, clips[0]->GetVideoInfo().height });

	// 3Dlut数据
	frames.push_back({ m_pLutData.get(), m_lutWidth, m_lutHeight / m_lutWidth, "", -1, false, FRAME_3D, m_lutHeight / m_lutWidth });

	utils::MutexQueueImp::OpenglInstance()->sync([this, frames, env, data] {
		processor->Render(frames, vi.width, vi.height, data);
	});
	return dst;

}
