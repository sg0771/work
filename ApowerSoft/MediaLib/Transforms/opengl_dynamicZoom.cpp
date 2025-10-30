
#include <avisynth/avisynth.h>
#include <string>
#include "opengl_GlFilter.h"

class dynamicZoom : public GlFilter
{
public:
	dynamicZoom(const PClip& child, const int start, const int end, const int width, const int height,
		const AVSValue& rectStruct, IScriptEnvironment* env);
	~dynamicZoom();


	void SetFragParameters(std::shared_ptr< OpenGLFixed> processor)override;

public:
	float m_rects[4 * 8];
	int m_count;
};



const char sfragShader[] = R"!!(
#ifdef GL_ES 
precision mediump float;
#define GLSLIFY 1
#endif

#define PI 3.1415926

uniform vec4 	  targetRect[8];
uniform float	  iCount;


vec2 fTextureCoord = gl_FragCoord.xy / iResolution.xy;

float fcount = float(iCount);
int num = int(floor(iProgress * fcount));

void motion(inout vec4 rect)
{
	float subProgress = iProgress - float(num) / float(iCount);
	if(subProgress < 1./(2.*fcount)){
		rect = rect - (rect - targetRect[num]) * subProgress * 2 * iCount;
	}
	else{
		rect = rect - (rect - targetRect[num]) * (1.0/fcount - subProgress) * 2 * iCount;
	}
}

void main() {
	//if(iCount == 2){ gl_FragColor = vec4(1,1,1,1);return; }
	vec4 dynamicRect = vec4(0., 0., 1., 1.);
	motion(dynamicRect);
	
	vec2 dynamicRectCenter = vec2( dynamicRect.x + (dynamicRect.z * 0.5) , dynamicRect.y + (dynamicRect.w * 0.5) );
	vec2 ratio = dynamicRect.zw / vec2(1., 1.);
	
	float x = dynamicRectCenter.x -( (0.5 - fTextureCoord.x) * ratio.x );
	float y = dynamicRectCenter.y -( (0.5 - fTextureCoord.y) * ratio.y );
	
	gl_FragColor = texture2D(iChannel[0], vec2(x,y));
}

)!!";

dynamicZoom::dynamicZoom(const PClip& child, const int start, const int end, const int width, const int height, const AVSValue& rectStruct, 
	IScriptEnvironment* env) : GlFilter(child, start, end, width, height, sfragShader, "", env)
{
	memset(m_rects, 0x00, sizeof(m_rects));
	m_count = 0;

	if (rectStruct.IsArray()) {
		int icount = static_cast<int>(rectStruct[0].AsFloat());
		m_count = icount;
		for (int i = 0; i < icount * 4; ++i) {
			m_rects[i] = rectStruct[i+1].AsFloat();
		}
	}
}

dynamicZoom::~dynamicZoom()
{

}
void dynamicZoom::SetFragParameters(std::shared_ptr< OpenGLFixed> processor)
{
	processor->SetUniform("iCount", 1, { static_cast<float>(m_count) });
	processor->SetUniform("targetRect", 4, {m_rects[0],	 m_rects[1], m_rects[2], m_rects[3],
											m_rects[4],  m_rects[5], m_rects[6], m_rects[7], 
											m_rects[8], m_rects[9], m_rects[10], m_rects[11], 
											m_rects[12], m_rects[13], m_rects[14], m_rects[15], 
											m_rects[16], m_rects[17], m_rects[18], m_rects[19], 
											m_rects[20], m_rects[21], m_rects[22], m_rects[23], 
											m_rects[24], m_rects[25], m_rects[26], m_rects[27], 
											m_rects[28], m_rects[29], m_rects[30], m_rects[31] });
}

 AVSValue __cdecl Create_dynamicZoom(AVSValue args, void* user_data, IScriptEnvironment* env) {
	//WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);
	return new dynamicZoom(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(200000000),
		args[3].AsInt(0), args[4].AsInt(0), args[5], env);
}