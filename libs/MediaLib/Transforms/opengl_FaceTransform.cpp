#pragma once
#include <avisynth/avisynth.h>
#include <string>
#include "opengl_GlFilter.h"
class FaceTransform : public GlFilter
{
	int init_vertex();
	std::vector<GLfloat> fromCoords;
	std::vector<GLfloat> toCoords;
public:
	FaceTransform(const PClip& child, const int start, const int end, const int width, const int height, const AVSValue& shaders,
		IScriptEnvironment* env);
	~FaceTransform();
	void SetFragParameters(std::shared_ptr< OpenGLFixed> processor)override;
	std::vector<GLfloat>  getFaceLandmarkCoords();

};



 AVSValue __cdecl Create_FaceTransform(AVSValue args, void* user_data, IScriptEnvironment* env) {
	//WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);
	return new FaceTransform(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(200000000),
		args[3].AsInt(0), args[4].AsInt(0), args[5], env);
}

int FaceTransform::init_vertex() {
	return 0;
}

std::vector<GLfloat>  FaceTransform::getFaceLandmarkCoords()
{
	return {};
}

void FaceTransform::SetFragParameters(std::shared_ptr< OpenGLFixed> processor)
{
	std::vector<GLfloat> vertexs = { 
		-1.0f,-1.0f,
		1.0f,-1.0f,
		0.5,0,
		1.0f,1.0f,
		-1.0,1.0}                      ;
	std::vector<GLuint> vertexselement = {0,1,2,3,4};

	processor->SetVertexBuffer(vertexs, vertexselement);
}


FaceTransform::FaceTransform(const PClip &child, const int start, const int end, const int width, const int height, const AVSValue &shaders,
	IScriptEnvironment *env) : GlFilter{ child, start, end, width, height, shaders,"", env }
{

}


FaceTransform::~FaceTransform()
{
}
