#pragma once

#include <avisynth/avisynth.h>
#include <string>
#include "opengl_GlFilter.h"

class Delogo_guass : public GlFilter
{
public:
	Delogo_guass(const PClip& child, const int start, const int end, const int width,
		const int height, const int mod, const float _alpha, const float _degree, const AVSValue& rectStruct,
		IScriptEnvironment* env) : GlFilter(child, start, end, width, height, AVSValue(m_sDelogShaders[mod].c_str()), "", env)
		, m_degree(_degree), m_alpha(_alpha){

		memset(m_rects, 0x00, sizeof(m_rects));
		m_count = 0;

		if (rectStruct.IsArray()) {
			int icount = static_cast<int>(rectStruct[0].AsFloat());
			m_count = icount;
			for (int i = 0; i < icount * 4; ++i) {
				m_rects[i] = rectStruct[i + 1].AsFloat();
			}
		}
	}
	~Delogo_guass() {}

	void SetFragParameters(std::shared_ptr< OpenGLFixed> processor) override {

		processor->SetUniform("Count", 1, {static_cast<float>(m_count)});
		processor->SetUniform("degree", 1, { m_degree });
		processor->SetUniform("alpha", 1, { m_alpha });
		processor->SetUniform("rects", 4, { m_rects[0],	1 - m_rects[1], m_rects[2], m_rects[3],
										m_rects[4], 1 -  m_rects[5], m_rects[6], m_rects[7],
										m_rects[8], 1 - m_rects[9], m_rects[10], m_rects[11],
										m_rects[12],1 -  m_rects[13], m_rects[14], m_rects[15],
										m_rects[16],1 -  m_rects[17], m_rects[18], m_rects[19],
										m_rects[20],1 -  m_rects[21], m_rects[22], m_rects[23],
										m_rects[24],1 -  m_rects[25], m_rects[26], m_rects[27],
										m_rects[28],1 -  m_rects[29], m_rects[30], m_rects[31] });

	}



private:
	float m_rects[4 * 8];
	int m_count;
	float m_degree;
	float m_alpha;
public:
	const static std::vector<std::string> m_sDelogShaders;
};

AVSValue __cdecl Create_Delogo_guass(AVSValue args, void* user_data, IScriptEnvironment* env) {
	//WXLogA("------------- OPENGL File Create [%s]", __FUNCTION__);

	return new Delogo_guass(args[0].AsClip(), args[1].AsInt(0), args[2].AsInt(200000000),
		args[3].AsInt(0), args[4].AsInt(0), args[5].AsInt(0), args[6].AsFloat(0), args[7].AsFloat(0), args[8], env);
}

const std::string gBaseShader = R"!!(
	uniform vec4 rects[8];
	uniform float Count;
	uniform float degree;
	uniform float alpha;

	vec2 fTextureCoord = gl_FragCoord.xy / iResolution.xy;
	vec4 res_color = texture2D(iChannel[0], fTextureCoord);

	bool inBounds(vec2 p) {

   	for(int i = 0; i < Count; i++){
		vec4 rect = rects[i];
		
		//if(all(lessThanEqual(rect.xy, p)) && all(lessThanEqual(p, rect.xy + rect.zw)))
		if(p.x > rect.x && p.y < rect.y && p.x < (rect.x + rect.z) && p.y > (rect.y - rect.w))
			return true;
	}
	return false;
}

bool inBounds(vec2 p, out vec2 bottom, out vec2 top, out vec2 left, out vec2 right) {

   	for(int i = 0; i < Count; i++){
		vec4 rect = rects[i];
		
		//if(all(lessThanEqual(rect.xy, p)) && all(lessThanEqual(p, rect.xy + rect.zw)))
		if(p.x > rect.x && p.y < rect.y && p.x < (rect.x + rect.z) && p.y > (rect.y - rect.w)){
		
			bottom = vec2(p.x, rect.y - rect.w);
			top = vec2(p.x, rect.y);
			left = vec2(rect.x, p.y);
			right = vec2(rect.x+rect.z, p.y);
			return true;
		}
	}
	return false;
}

vec4 getFragColor();
void main() {
	gl_FragColor = getFragColor();
}

)!!";

const std::string gGuassDelogo = gBaseShader + R"!!(

/** 种子随机函数, 参数可以根据当前坐标来生成. */
float seedRandom2(vec2 v) {
    return fract(sin(dot(v, vec2(12.9898, 78.233))) * 43758.5453);
}

/** 随机二维噪声, 返回范围为 [0, 1]. 其变化频率由 v 值的范围决定. */
float noise2d(vec2 v) {
    vec2 i = floor(v);
    vec2 f = fract(v);
    vec2 u = f * f * (3.0 - 2.0 * f);

    float a = seedRandom2(i);
    float b = seedRandom2(i + vec2(1, 0));
    float c = seedRandom2(i + vec2(0, 1));
    float d = seedRandom2(i + vec2(1, 1));

    return mix(a, b, u.x) +
        (c - a) * u.y * (1.0 - u.x) +
        (d - b) * u.x * u.y;
}

// 高斯模糊权重.
const vec2 guassBlurPriority[15] = vec2[](
	vec2(-7.0, 0.0311),
	vec2(-6.0, 0.0419),
	vec2(-5.0, 0.0539),
	vec2(-4.0, 0.0663),
	vec2(-3.0, 0.0779),
	vec2(-2.0, 0.0874),
	vec2(-1.0, 0.0936),
	vec2(0.0,  0.0958),
	vec2(1.0,  0.0936),
	vec2(2.0,  0.0874),
	vec2(3.0,  0.0779),
	vec2(4.0,  0.0663),
	vec2(5.0,  0.0539),
	vec2(6.0,  0.0419),
	vec2(7.0,  0.0311)
);

/** 进行高斯模糊透明度采样. */
vec4 gaussBlurSampling() {
	vec4 color = vec4(0);
	float samplingCount = 14.0;
	vec2 d = vec2(degree*3, 0) / vec2(1280.0, 720.0) / samplingCount;


	float noiseFrequency = 2.0;
	for (int i = 0; i < 15; i++) {
		vec2 priority = guassBlurPriority[i];
		
		float noiseValue = noise2d((gl_FragCoord.xy + priority.xy) * noiseFrequency) - 0.5;
		color += texture2D(iChannel[0], fTextureCoord + (priority.x + noiseValue) * d) * priority.y;
		
		//color += texture2D(iChannel[0], fTextureCoord - priority.x * d) * priority.y;
	}

    return color /= color.a;
}

vec4 getFragColor() {
	
	if(inBounds(fTextureCoord)){
		vec4 mix_color = gaussBlurSampling();
		res_color = mix(res_color, mix_color, alpha/100);
	}
    return res_color;
}


)!!";

const std::string gFFmpegDelogo = gBaseShader + R"!!(

vec4 getFragColor() {
	
	vec2 bottom = vec2(0);
	vec2 top = vec2(0);
	vec2 left = vec2(0);
	vec2 right = vec2(0);
	if(inBounds(fTextureCoord, bottom, top, left, right)){
		vec4 vColor = mix(texture2D(iChannel[0], bottom), texture2D(iChannel[0], top), (fTextureCoord.y - bottom.y) / (top.y - bottom.y) );
		vec4 hColor = mix(texture2D(iChannel[0], left), texture2D(iChannel[0], right), (fTextureCoord.x - left.x) / (right.x - left.x) );
		
		float vFactor = (fTextureCoord.y - bottom.y) * (top.y - fTextureCoord.y);
		float hFactor = (fTextureCoord.x - left.x) * (right.x - fTextureCoord.x);
		
		vec4 mix_color = mix(vColor, hColor, vFactor*degree/100 / (vFactor*degree/100 + hFactor));
		
		res_color = mix(res_color, mix_color, alpha/100);
		
	}
    return res_color;
}

)!!";

const std::string gPixelate = gBaseShader + R"!!(

vec2 squareSize = vec2((degree+1) / 1800 / iAspect, (degree+1) / 1800);

vec4 getFragColor() {
	
	if(inBounds(fTextureCoord)){
		vec2 p = (floor(fTextureCoord / squareSize) + 0.5) * squareSize;
		vec4 mix_color = texture2D(iChannel[0], p);	
		res_color = mix(res_color, mix_color, alpha/100);
	}
	return res_color;
}

)!!";

const std::vector<std::string> Delogo_guass::m_sDelogShaders = {gFFmpegDelogo, gGuassDelogo, gPixelate};