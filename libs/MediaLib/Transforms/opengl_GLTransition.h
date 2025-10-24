#pragma once
#include <avisynth/avisynth.h>
#include <string>
class GLTransition : public GenericVideoFilter
{
public:
	PClip _left_child;
	int offset;
	PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment *env) override;
	GLTransition(const PClip &child, int _offset, IScriptEnvironment *env);
	~GLTransition();
};

