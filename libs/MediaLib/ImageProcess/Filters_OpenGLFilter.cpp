#include "Filters_OpenGLFilter.h"
#include "reflector_Renderer.h"
//#include "CommonShaderContent.h"


std::shared_ptr<utils::MutexQueueImp>s_taskOpenGL(new utils::MutexQueueImp(1));
utils::MutexQueueImp* utils::MutexQueueImp::OpenglInstance()
{
	return s_taskOpenGL.get();
}

void IMulGLFilter::SetParameter(void * param)
{
	for (size_t i = 0; i < childFilters.size(); i++)
	{
		childFilters[i]->SetParameter(param);
	}
}
void IMulGLFilter::UpdateProgress(float duration, float time)
{
	IOpenGLFilter::UpdateProgress(duration, time);
	for (size_t i = 0; i < childFilters.size(); i++)
	{
		childFilters[i]->UpdateProgress(duration, time);
	}
}

OpenGLFilter::OpenGLFilter(Shader* shader, int _baseslot ) : baseslot(_baseslot) {
	shaders.push_back(shader);
}

void OpenGLFilter::Render(std::vector< FilterFrame> frames, int width, int height, void* output )
{
	//Renderer::Instance().Render(frames, this->shaders, width, height, baseslot, output);
}

void OpenGLFilter::Render(std::vector< FilterFrame> frames, const std::vector<float>& sizes, void* output)
{
	Renderer::Instance().Render(frames, this->shaders, sizes, baseslot, output);
}


 void OpenGLFilter::UpdateProgress(float duration, float time) {
	 IOpenGLFilter::UpdateProgress(duration, time);
	 for (size_t i = 0; i < shaders.size(); i++)
	 {
		 shaders[i]->SetProgress(duration, time);
	 }
 };

 void OpenGLFilter::SetParameter(void * param)
 {
	 for (size_t i = 0; i < shaders.size(); i++)
	 {
		 shaders[i]->SetParameter(param);
	 }
 }

 void OpenGLFilter::AddShader(Shader * shader)
 {
	 shaders.push_back(shader);
 }