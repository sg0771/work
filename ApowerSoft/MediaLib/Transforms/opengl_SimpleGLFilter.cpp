
#include "windows.h"

#include "opengl_SimpleGLFilter.h"
#include "Utils.hpp"

//#include "zlog.h"

#include <typeinfo>
#include <iostream>
#include <fstream>



SimpleGLFilter::SimpleGLFilter(const AVSValue &childs, const std::vector<std::string> &_fragmentshaders, const int start,
	const int end, IScriptEnvironment *env) : base{ childs[0].AsClip(),__FUNCTION__ }
{
	processor = NULL;
	for (size_t i = 0; i < childs.ArraySize(); i++)
	{
		this->clips.push_back(childs[i].AsClip());
	}

	const auto orig_frames = vi.num_frames;

	_frame_start = start;
	_frame_end = std::min(end, orig_frames - 1);
	_frame_count = _frame_end - _frame_start + 1;
	vertexshaderstr = DefaultVertexShader;
	
	for (size_t i = 0; i < _fragmentshaders.size(); i++)
	{
		std::stringstream ss;
		ss << FragmentShaderHeader;
		std::ifstream stream(_fragmentshaders[i]);
		auto notfile = stream.fail();
		if (notfile) {
			ss << _fragmentshaders[i];
		}
		else {
			std::string line;
			while (getline(stream, line))
				ss << line << "\n";
		}
		fragshaderstrs.push_back(ss.str());
	}
	
}


SimpleGLFilter::~SimpleGLFilter() {

	if (processor)
	{
		delete processor;
	}
}
void SimpleGLFilter::InitProcess() 
{
	processor = new OpenGLFilter();
	for (size_t i = 0; i < fragshaderstrs.size(); i++)
	{
		processor->AddShader(new Shader(vertexshaderstr, fragshaderstrs[i]));
	}
};

PVideoFrame SimpleGLFilter::GetFrame(int n, IScriptEnvironment *env) 
{

	if (n < _frame_start || n > _frame_end) {
		auto src = base::GetFrame(n, env);

		return src;
	}

	auto dst = env->NewVideoFrame(vi);
	if (dst == nullptr || dst.m_ptr == nullptr) {
		return nullptr;
	}
	
	if (!processor) 
		utils::MutexQueueImp::OpenglInstance()->sync([this] {InitProcess();});
	
		
	if (!processor) 
		return base::GetFrame(n, env);;
	
	float duration = _frame_count * vi.fps_denominator / static_cast<double>(vi.fps_numerator);
	float time= (n - _frame_start) * vi.fps_denominator / static_cast<double>(vi.fps_numerator);
		processor->UpdateProgress(duration, time);

		void* data = (void*)dst->GetWritePtr();
		std::vector<FilterFrame> frames;
		PVideoFrame pvideoframes[2];
		std::vector<float> sizes(24, 0.f);
		for (size_t i = 0; i < clips.size(); i++)
		{
			pvideoframes[i] = clips[i]->GetFrame(n, env);
			
			frames.push_back({ (unsigned char*)pvideoframes[i]->GetReadPtr(), clips[i]->GetVideoInfo().width, clips[i]->GetVideoInfo().height });
			
			if (i < 8)
			{
				sizes[3 * i] = clips[i]->GetVideoInfo().width;
				sizes[3 * i + 1] = clips[i]->GetVideoInfo().height;
			}
		}
		utils::MutexQueueImp::OpenglInstance()->sync([this, frames, env, data, sizes] {
			processor->Render(frames, sizes, data);
		});
	return dst;
}

