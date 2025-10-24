#include "opengl_GLTransition.h"
#include <math.h>
PVideoFrame GLTransition::GetFrame(int n, IScriptEnvironment *env)
{
	if (offset < 0)
	{
		int temp = n + offset;
		temp = temp < 0 ? 0 : temp;
		return child->GetFrame(temp, env);
	}
	else
		return child->GetFrame(n, env);
	
}

GLTransition::GLTransition(const PClip &child, int _offset, IScriptEnvironment *env):GenericVideoFilter(child, __FUNCTION__), offset(_offset)
{
	vi = child->GetVideoInfo();
	
	vi.num_frames = vi.num_frames + abs(offset);
}


GLTransition::~GLTransition()
{
}
