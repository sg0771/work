#pragma once
#include<string>

const std::string sChromakeyShader = R"!!(
#ifdef GL_ES
precision mediump float;
#endif


vec2 fTextureCoord = gl_FragCoord.xy / iResolution.xy;

// ��ɫ�ͺ�ɫͨ���ľ�ֵ��ﵽ��ֵʱ, ��ʼ��͸���ȴ� 0 ���ɵ� 1.
// ȡֵ��Χ������ 0.5~0.8 ֮��. ֵԽ��, ���ٳ�������Խ��.

// ����Զ����ƴ���ֵ:
// 	   ����ͳ�� `1.0 - (g - r) * 2` ����ɫ�ֲ�.
//     ������ͳ��ͼ��һ����� (����) ���Ҳ� (ǰ��) �ֱ�ܴ�, �м���һ������ͼ��.
//     ѡ�񹵵׵��м伴��.
uniform float colorDiffThreshold = 0.8;

// �ڿ�ʼ����ƽ��ʱ�Ĺ���λ��.
// 0~1, һ��Ծ�̬��������Ϊ 1, ��̬����Ϊ 0.5.
// ������� 0, ��պ��ڱ���ɫ�ı�Եλ�ÿ�ʼ����, ����뱳��ɫ.
// ������� 1, ��պ���ɫ��ֲ�����ֵ��, ��ȫ�����뱳��ɫ, ����Ҳ�п��ܻ�����˶�ģ���Ĳ���.
float smoothStartRate = 0.5;

// �ڿ�ʼƽ���� 1 �Ĵ�����λ�ô�����ƽ��.
// 0~1, һ��Ծ�̬��������Ϊ 0.8, ��̬����Ϊ 1.
// ������� 0, ��պ���ɫ��ֲ�����ֵ����������, ���ɷ�Χ��С.
// ������� 1, ����ɵ�ֱ����ɫ��ȫ��ռ��, ���ɷ�Χ���.
float smoothEndRate = 0.75;

// ���и�˹��Ȩ����ʱ�����ذ뾶�Ի�ȡƽ���� Alpha, Ĭ��Ϊ 2, ֻ��Ҫ��֤��Ե�㹻˳������.
float alphaSamplingRadius = 2.0;

// ���и�˹��Ȩ����ʱ�����ذ뾶�Բ����ٽ�����ɫ, Ĭ��Ϊ 8, ֻ��Ҫ��Ե����ɫ���´���ɾ�����.
float neighboorSamplingRadius = 8.0;


// ���Բ�ֵ, ���� x �ķ�Χ���� 0~1 �Ľ��.
float linearStep(float x, float min, float max) {
	if (x <= min) {
		return 0.0;
	} 
	else if (x >= max) {
		return 1.0;
	}
	else {
		return (x - min) / (max - min);
	}
}

/** �����������, �������Ը��ݵ�ǰ����������. */
float seedRandom2(vec2 v) {
    return fract(sin(dot(v, vec2(12.9898, 78.233))) * 43758.5453);
}

/** �����ά����, ���ط�ΧΪ [0, 1]. ��仯Ƶ���� v ֵ�ķ�Χ����. */
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

// ��˹����������ƫ�ƺ�Ȩ��.
vec3 guassianXYP25[25] = vec3[](
	vec3(-2.0, -2.0, 0.0106),
	vec3(-2.0, -1.0, 0.0246),
	vec3(-2.0,  0.0, 0.0325),
	vec3(-2.0,  1.0, 0.0246),
	vec3(-2.0,  2.0, 0.0106),
	vec3(-1.0, -2.0, 0.0246),
	vec3(-1.0, -1.0, 0.0571),
	vec3(-1.0,  0.0, 0.0756),
	vec3(-1.0,  1.0, 0.0571),
	vec3(-1.0,  2.0, 0.0246),
	vec3( 0.0, -2.0, 0.0325),
	vec3( 0.0, -1.0, 0.0756),
	vec3( 0.0,  0.0, 0.0756),
	vec3( 0.0,  1.0, 0.0756),
	vec3( 0.0,  2.0, 0.0325),
	vec3( 1.0, -2.0, 0.0246),
	vec3( 1.0, -1.0, 0.0571),
	vec3( 1.0,  0.0, 0.0756),
	vec3( 1.0,  1.0, 0.0571),
	vec3( 1.0,  2.0, 0.0246),
	vec3( 2.0, -2.0, 0.0106),
	vec3( 2.0, -1.0, 0.0246),
	vec3( 2.0,  0.0, 0.0325),
	vec3( 2.0,  1.0, 0.0246),
	vec3( 2.0,  2.0, 0.0106)
);

// ��ȡɫ���ֵ.
float getColorDiff(vec4 color) {
	float r = color.r;
	float g = color.g;
	float b = color.b;

	// �����ɫͨ������������ͨ����ֵ�Ĳ�.
	// ��������������ΧΪ 0~1, ���Һ�����͸���������.
	return 1.0 - 2 * (g - max(r, b));
}

// ���� 5x5 ��Χ�ĸ�˹ģ������.
// ���������Լ�ǿС��ϸ����������ϸ�ڱ�Ե��ƽ����.
vec4 getGaussianBlurredColor() {
	vec4 blurredColor = vec4(0.0);
	vec2 d = alphaSamplingRadius / 2.0 / iResolution.xy;

	for (int i = 0; i < 25; i++) {
		vec3 xyp = guassianXYP25[i];
		vec4 color = texture2D(iChannel[0], fTextureCoord + vec2(xyp.x, xyp.y) * d);
		blurredColor += color * xyp.z;
	}

	return blurredColor;
}

// ��ȡ���Բ�ֵ��ϵ��.
float getLinearFactor(float value, float from, float to) {
	return (value - from) / (to - from);
}

// ��������ϵ�����в�ֵ.
float getValueByLinearFactor(float factor, float from, float to) {
	return (to - from) * factor + from;
}

// ��ȡ�ٽ�����ɫֵ, δ����Ԥ��.
vec3 getNeighborColor() {
	vec3 totalColor = vec3(0.0);
	float totalWeight = 0.0;
	vec2 d = neighboorSamplingRadius / iResolution.xy / 2.0;

	// �����ı仯Ƶ��.
	float noiseFrequency = 2.0;

	for (int i = 0; i < 25; i++) {
		vec3 xyp = guassianXYP25[i];

		// ���ڲ���������, �Բ�����������Ŷ��Է�ֹ���ֲ���.
		// �Ŷ������������, �Ŷ���С����ֲ���. �����Ȳ��Ƹ����׽���.
		float noiseValue = noise2d((gl_FragCoord.xy + xyp.xy) * noiseFrequency) - 0.5;

		// ע�����ɫΪ��Ԥ��ɫ.
		vec4 color = texture2D(iChannel[0], fTextureCoord + (xyp.xy + noiseValue) * d);

		// �ٽ���ɫ��ɫ����ͱ���ɫ��Զ��̶Ⱦ����˼�Ȩ�ı���.
		float colorDiffWeight = pow(max(getColorDiff(color) - colorDiffThreshold, 0), 4.0);

		// ��������Ȩ��.
		float colorWeight = xyp.z * colorDiffWeight;

		totalColor += color.rgb * colorWeight;
		totalWeight += colorWeight;
	}

	// ��ѯ�����Ƚ��ٽ���ǰ��ɫ.
	return totalColor / totalWeight;
}

// ��ȡ��ɫ uv ֵ.
vec2 getUV(vec3 color) {
	float u = -0.169 * color.r - 0.33 * color.g + 0.5 * color.b;
	float v = 0.5 * color.r - 0.419 * color.g - 0.081 * color.b;

	return vec2(u, v);
}

// Spill ����, ����ɫ��������, ������Ե�ͷ����������ɫ.
// �������� rb ͨ����ƽ�������� g ͨ��������, ͬʱ������ƽ��.
// ����������ѡ��:
//     ����ԭɫ - �����ǰ��ɫ�����ڽ�ɫ�ӽ�, ���Һ���ͨ����Ƚϴ�, ��ʱ����ɫ����������ܴ��ɫ��ı�.
//     ƫ���ڽ�ɫ - �����ǰ��ɫ���ڽ�ɫ�㹻�ӽ�.
vec3 spillColor(vec3 currentColor, vec3 neighborColor) {

	// ����ɫͨ��������Сͨ��ʱ�Ŵ���.
	if (currentColor.g > min(currentColor.r, currentColor.b)) {

		float newGreen = currentColor.g;


		// ��ȡ�ڽ�ɫ��ɫͨ���ں���ͨ���е�����λ��.
		// ע����������ǳ��������, ������Ҫ�������Ƶ� -1~1.
		// ���൱��ĳ�������Сֵ�˲�.
		float neighborGreenFactor = getLinearFactor(neighborColor.g, neighborColor.r, neighborColor.b);
		neighborGreenFactor = clamp(neighborGreenFactor, -1.0, 1.0);

		// ����ǰ��ɫ����ɫͨ����ֵ, ����ǽ����ɫ����ڽ�ɫ��ӽ�����ɫ.
		// ��������ʵ�ǰ��� HSL ��ɫ�ռ��е�ɫ��.
		float neighborLikeGreen = getValueByLinearFactor(neighborGreenFactor, currentColor.r, currentColor.b);

		// ��������Ĳ���С, ��ô����ϵ�������Ŷȹ���, ��ʱΪ������ʵ��ĺ�����ֵ.
		float centralGreen = (currentColor.r + currentColor.b) / 2.0;
		float factorConfidence = linearStep(abs(neighborColor.r - neighborColor.b), 0.05, 0.15);
		neighborLikeGreen = mix(centralGreen, neighborLikeGreen, factorConfidence);

		// ���ڽ�ɫ�ľ���.
		float neighborColorDiff = distance(getUV(currentColor), getUV(neighborColor));

		// ���������ɫ�� UV ����ܴ�, ��ô���ٿ����ڽ�ɫ.
		float neighborColorDiffRate = 1.0 - linearStep(neighborColorDiff, 0.15, 0.25);
		
		// �����ٽ�ɫ.
		newGreen = mix(newGreen, neighborLikeGreen, neighborColorDiffRate);

		
		// ����������ɫ�����Ĺ���仯, ��ɫ�������ֻ��ı� 0.15.
		float greenDiff = abs(currentColor.g - newGreen);
		float greenScaleBack = min(0.15 / greenDiff, 1.0);
		newGreen = currentColor.g + (newGreen - currentColor.g) * greenScaleBack;

		// ������ɫ���ȵ���.
		// ��ѵķ����Ǵ������ɫ�仯��ͼ, Ȼ����ƽ��, ��������һ����������, ����ʹ����򵥵���ֵ�취.
		newGreen = min(newGreen, max(currentColor.r, currentColor.b));
		

		currentColor.g = newGreen;
	}

	return currentColor;
}

void main() {

	// ��ȡ��ǰ��ɫ.
	vec4 currentColor = texture2D(iChannel[0], fTextureCoord);

	// ��ȡ�����˸�˹ģ������ɫ.
	vec4 blurredColor = getGaussianBlurredColor();

	// ���ɫ���.
	float colorDiff = getColorDiff(blurredColor);

	// ���Բ�ֵ��ù��ɵ� Alpha.
	// colorDiffThreshold * 2 - 1.0, colorDiffThreshold, 1.0 �պóɵȲ�����.
	float start = mix(colorDiffThreshold * 2 - 1.0, colorDiffThreshold, smoothStartRate);
	float end = mix(colorDiffThreshold, 1.0, smoothEndRate);
	float alpha = linearStep(colorDiff, start, end);
	
	// ����ٽ���ɫ.
	vec3 neighborColor = getNeighborColor();

	// ������ɫͨ��.
	vec4 color = vec4(spillColor(currentColor.rgb, neighborColor), 1.0);
	color *= alpha;

	// �����ɫ.
	gl_FragColor = color;
}

)!!";


const std::string sChromakeyShaderB = R"!!(
#ifdef GL_ES
precision mediump float;
#endif

vec2 fTextureCoord = gl_FragCoord.xy / iResolution.xy;



// ��ɫ�ͺ�ɫͨ���ľ�ֵ��ﵽ��ֵʱ, ��ʼ��͸���ȴ� 0 ���ɵ� 1.
// ȡֵ��Χ������ 0.5~0.8 ֮��. ֵԽ��, ���ٳ�������Խ��.

// ����Զ����ƴ���ֵ:
// 	   ����ͳ�� `1.0 - (g - r) * 2` ����ɫ�ֲ�.
//     ������ͳ��ͼ��һ����� (����) ���Ҳ� (ǰ��) �ֱ�ܴ�, �м���һ������ͼ��.
//     ѡ�񹵵׵��м伴��.
uniform float colorDiffThreshold = 0.8;

// �ڿ�ʼ����ƽ��ʱ�Ĺ���λ��.
// 0~1, һ��Ծ�̬��������Ϊ 1, ��̬����Ϊ 0.5.
// ������� 0, ��պ��ڱ���ɫ�ı�Եλ�ÿ�ʼ����, ����뱳��ɫ.
// ������� 1, ��պ���ɫ��ֲ�����ֵ��, ��ȫ�����뱳��ɫ, ����Ҳ�п��ܻ�����˶�ģ���Ĳ���.
float smoothStartRate = 0.5;

// �ڿ�ʼƽ���� 1 �Ĵ�����λ�ô�����ƽ��.
// 0~1, һ��Ծ�̬��������Ϊ 0.8, ��̬����Ϊ 1.
// ������� 0, ��պ���ɫ��ֲ�����ֵ����������, ���ɷ�Χ��С.
// ������� 1, ����ɵ�ֱ����ɫ��ȫ��ռ��, ���ɷ�Χ���.
float smoothEndRate = 0.75;

// ���и�˹��Ȩ����ʱ�����ذ뾶�Ի�ȡƽ���� Alpha, Ĭ��Ϊ 2, ֻ��Ҫ��֤��Ե�㹻˳������.
float alphaSamplingRadius = 2.0;

// ���и�˹��Ȩ����ʱ�����ذ뾶�Բ����ٽ�����ɫ, Ĭ��Ϊ 8, ֻ��Ҫ��Ե����ɫ���´���ɾ�����.
float neighboorSamplingRadius = 8.0;

// ���Բ�ֵ, ���� x �ķ�Χ���� 0~1 �Ľ��.
float linearStep(float x, float min, float max) {
	if (x <= min) {
		return 0.0;
	} 
	else if (x >= max) {
		return 1.0;
	}
	else {
		return (x - min) / (max - min);
	}
}

/** �����������, �������Ը��ݵ�ǰ����������. */
float seedRandom2(vec2 v) {
    return fract(sin(dot(v, vec2(12.9898, 78.233))) * 43758.5453);
}

/** �����ά����, ���ط�ΧΪ [0, 1]. ��仯Ƶ���� v ֵ�ķ�Χ����. */
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

// ��˹����������ƫ�ƺ�Ȩ��.
vec3 guassianXYP25[25] = vec3[](
	vec3(-2.0, -2.0, 0.0106),
	vec3(-2.0, -1.0, 0.0246),
	vec3(-2.0,  0.0, 0.0325),
	vec3(-2.0,  1.0, 0.0246),
	vec3(-2.0,  2.0, 0.0106),
	vec3(-1.0, -2.0, 0.0246),
	vec3(-1.0, -1.0, 0.0571),
	vec3(-1.0,  0.0, 0.0756),
	vec3(-1.0,  1.0, 0.0571),
	vec3(-1.0,  2.0, 0.0246),
	vec3( 0.0, -2.0, 0.0325),
	vec3( 0.0, -1.0, 0.0756),
	vec3( 0.0,  0.0, 0.0756),
	vec3( 0.0,  1.0, 0.0756),
	vec3( 0.0,  2.0, 0.0325),
	vec3( 1.0, -2.0, 0.0246),
	vec3( 1.0, -1.0, 0.0571),
	vec3( 1.0,  0.0, 0.0756),
	vec3( 1.0,  1.0, 0.0571),
	vec3( 1.0,  2.0, 0.0246),
	vec3( 2.0, -2.0, 0.0106),
	vec3( 2.0, -1.0, 0.0246),
	vec3( 2.0,  0.0, 0.0325),
	vec3( 2.0,  1.0, 0.0246),
	vec3( 2.0,  2.0, 0.0106)
);

// ��ȡɫ���ֵ.
float getColorDiff(vec4 color) {
	float r = color.g;
	float g = color.r;
	float b = color.b;

	// �����ɫͨ������������ͨ����ֵ�Ĳ�.
	// ��������������ΧΪ 0~1, ���Һ�����͸���������.
	return 1.0 - 2 * (g - max(r, b));
}

// ���� 5x5 ��Χ�ĸ�˹ģ������.
// ���������Լ�ǿС��ϸ����������ϸ�ڱ�Ե��ƽ����.
vec4 getGaussianBlurredColor() {
	vec4 blurredColor = vec4(0.0);
	vec2 d = alphaSamplingRadius / 2.0 / iResolution.xy;

	for (int i = 0; i < 25; i++) {
		vec3 xyp = guassianXYP25[i];
		vec4 color = texture2D(iChannel[0], fTextureCoord + vec2(xyp.x, xyp.y) * d);
		blurredColor += color * xyp.z;
	}

	return blurredColor;
}

// ��ȡ���Բ�ֵ��ϵ��.
float getLinearFactor(float value, float from, float to) {
	return (value - from) / (to - from);
}

// ��������ϵ�����в�ֵ.
float getValueByLinearFactor(float factor, float from, float to) {
	return (to - from) * factor + from;
}

// ��ȡ�ٽ�����ɫֵ, δ����Ԥ��.
vec3 getNeighborColor() {
	vec3 totalColor = vec3(0.0);
	float totalWeight = 0.0;
	vec2 d = neighboorSamplingRadius / iResolution.xy / 2.0;

	// �����ı仯Ƶ��.
	float noiseFrequency = 2.0;

	for (int i = 0; i < 25; i++) {
		vec3 xyp = guassianXYP25[i];

		// ���ڲ���������, �Բ�����������Ŷ��Է�ֹ���ֲ���.
		// �Ŷ������������, �Ŷ���С����ֲ���. �����Ȳ��Ƹ����׽���.
		float noiseValue = noise2d((gl_FragCoord.xy + xyp.xy) * noiseFrequency) - 0.5;

		// ע�����ɫΪ��Ԥ��ɫ.
		vec4 color = texture2D(iChannel[0], fTextureCoord + (xyp.xy + noiseValue) * d);

		// �ٽ���ɫ��ɫ����ͱ���ɫ��Զ��̶Ⱦ����˼�Ȩ�ı���.
		float colorDiffWeight = pow(max(getColorDiff(color) - colorDiffThreshold, 0), 4.0);

		// ��������Ȩ��.
		float colorWeight = xyp.z * colorDiffWeight;

		totalColor += color.rgb * colorWeight;
		totalWeight += colorWeight;
	}

	// ��ѯ�����Ƚ��ٽ���ǰ��ɫ.
	return totalColor / totalWeight;
}

// ��ȡ��ɫ uv ֵ.
vec2 getUV(vec3 color) {
	float u = -0.169 * color.g - 0.33 * color.r + 0.5 * color.b;
	float v = 0.5 * color.g - 0.419 * color.r - 0.081 * color.b;

	return vec2(u, v);
}

// Spill ����, ����ɫ��������, ������Ե�ͷ����������ɫ.
// �������� rb ͨ����ƽ�������� g ͨ��������, ͬʱ������ƽ��.
// ����������ѡ��:
//     ����ԭɫ - �����ǰ��ɫ�����ڽ�ɫ�ӽ�, ���Һ���ͨ����Ƚϴ�, ��ʱ����ɫ����������ܴ��ɫ��ı�.
//     ƫ���ڽ�ɫ - �����ǰ��ɫ���ڽ�ɫ�㹻�ӽ�.
vec3 spillColor(vec3 currentColor, vec3 neighborColor) {

	// ����ɫͨ��������Сͨ��ʱ�Ŵ���.
	if (currentColor.r > min(currentColor.g, currentColor.b)) {

		float newGreen = currentColor.r;


		// ��ȡ�ڽ�ɫ��ɫͨ���ں���ͨ���е�����λ��.
		// ע����������ǳ��������, ������Ҫ�������Ƶ� -1~1.
		// ���൱��ĳ�������Сֵ�˲�.
		float neighborGreenFactor = getLinearFactor(neighborColor.r, neighborColor.g, neighborColor.b);
		neighborGreenFactor = clamp(neighborGreenFactor, -1.0, 1.0);

		// ����ǰ��ɫ����ɫͨ����ֵ, ����ǽ����ɫ����ڽ�ɫ��ӽ�����ɫ.
		// ��������ʵ�ǰ��� HSL ��ɫ�ռ��е�ɫ��.
		float neighborLikeGreen = getValueByLinearFactor(neighborGreenFactor, currentColor.g, currentColor.b);

		// ��������Ĳ���С, ��ô����ϵ�������Ŷȹ���, ��ʱΪ������ʵ��ĺ�����ֵ.
		float centralGreen = (currentColor.g + currentColor.b) / 2.0;
		float factorConfidence = linearStep(abs(neighborColor.g - neighborColor.b), 0.05, 0.15);
		neighborLikeGreen = mix(centralGreen, neighborLikeGreen, factorConfidence);

		// ���ڽ�ɫ�ľ���.
		float neighborColorDiff = distance(getUV(currentColor), getUV(neighborColor));

		// ���������ɫ�� UV ����ܴ�, ��ô���ٿ����ڽ�ɫ.
		float neighborColorDiffRate = 1.0 - linearStep(neighborColorDiff, 0.15, 0.25);
		
		// �����ٽ�ɫ.
		newGreen = mix(newGreen, neighborLikeGreen, neighborColorDiffRate);

		
		// ����������ɫ�����Ĺ���仯, ��ɫ�������ֻ��ı� 0.15.
		float greenDiff = abs(currentColor.r - newGreen);
		float greenScaleBack = min(0.15 / greenDiff, 1.0);
		newGreen = currentColor.r + (newGreen - currentColor.r) * greenScaleBack;

		// ������ɫ���ȵ���.
		// ��ѵķ����Ǵ������ɫ�仯��ͼ, Ȼ����ƽ��, ��������һ����������, ����ʹ����򵥵���ֵ�취.
		newGreen = min(newGreen, max(currentColor.g, currentColor.b));
		

		currentColor.r = newGreen;
	}

	return currentColor;
}

void main() {

	// ��ȡ��ǰ��ɫ.
	vec4 currentColor = texture2D(iChannel[0], fTextureCoord);

	// ��ȡ�����˸�˹ģ������ɫ.
	vec4 blurredColor = getGaussianBlurredColor();

	// ���ɫ���.
	float colorDiff = getColorDiff(blurredColor);

	// ���Բ�ֵ��ù��ɵ� Alpha.
	// colorDiffThreshold * 2 - 1.0, colorDiffThreshold, 1.0 �պóɵȲ�����.
	float start = mix(colorDiffThreshold * 2 - 1.0, colorDiffThreshold, smoothStartRate);
	float end = mix(colorDiffThreshold, 1.0, smoothEndRate);
	float alpha = linearStep(colorDiff, start, end);
	
	// ����ٽ���ɫ.
	vec3 neighborColor = getNeighborColor();

	// ������ɫͨ��.
	vec4 color = vec4(spillColor(currentColor.rgb, neighborColor), 1.0);
	color *= alpha;

	// �����ɫ.
	gl_FragColor = color;
}
)!!";


const std::string sEdgeIncrease = R"!!(
vec2 fTextureCoord = gl_FragCoord.xy / iResolution.xy;

uniform float edge;

float caldist(vec2 coord){
		float Sx[9];
		Sx[6] = -1.; Sx[7] = 0.; Sx[8] = 1.;
		Sx[3] = -2.; Sx[4] = 0.; Sx[5] = 2.;
		Sx[0] = -1.; Sx[1] = 0.; Sx[2] = 1.;
		
		float Sy[9];
		Sy[6] = -1.; Sy[7] = -2.; Sy[8] = -1.;
		Sy[3] = 0.; Sy[4] = 0.; Sy[5] = 0.;
		Sy[0] = 1.; Sy[1] = 2.; Sy[2] = 1.;
		
		float dx = 1.0 / float(iResolution.x);
		float dy = 1.0 / float(iResolution.y);
		float s00 = texture2D( iChannel[0],
		coord + vec2(-dx,dy)).a ;
		float s10 = texture2D( iChannel[0],
		coord + vec2(-dx,0.0) ).a ;
		float s20 = texture2D( iChannel[0],
		coord + vec2(-dx,-dy) ).a ;
		float s01 = texture2D( iChannel[0],
		coord + vec2(0.0,dy) ).a ;
		float s21 = texture2D( iChannel[0],
		coord + vec2(0.0,-dy) ).a ;
		float s02 = texture2D( iChannel[0],
		coord + vec2(dx, dy) ).a ;
		float s12 = texture2D( iChannel[0],
		coord + vec2(dx, 0.0) ).a ;
		float s22 = texture2D( iChannel[0],
		coord + vec2(dx, -dy) ).a ;
		
		float sx = s00 + 2 * s10 + s20 - (s02 + 2 * s12 + s22);
		float sy = s00 + 2 * s01 + s02 - (s20 + 2 * s21 + s22);
		float dist = sx * sx + sy * sy;
		return dist;
}


void main()
{
	vec2 uv = gl_FragCoord.xy / iResolution.xy;
	vec3 color = texture2D(iChannel[0], uv).rgb;

	float mini = 10;
	int d = 1;
	for(int i = -d; i <= d; i++){
		for(int j = -d; j <= d; j++){
			float a = texture2D(iChannel[0], uv + vec2(i,j) / iResolution.xy).a;
			mini = a < mini ? a : mini;
		}
	}
	
	if(mini < edge/100.){
		gl_FragColor = vec4(texture2D(iChannel[0], uv ).xyz, 0);
	}
	else if(edge <= 0 && 0.1 > texture2D(iChannel[0], uv ).a){
		if(edge == 0){
			gl_FragColor = texture2D(iChannel[0], uv);
			return;
		}
		float maxDist = 0;
		int dl = int(clamp(-edge, 0, 3));
		for(int i = -dl; i <= dl; i++){
			for(int j = -dl; j <= dl; j++){
				float dist = caldist(uv + vec2(i,j) / iResolution.xy);
				if (dist > maxDist)
					maxDist = dist;
			}
		}
		if(maxDist > 0){
			gl_FragColor = vec4(texture2D(iChannel[0], uv ).xyz, 1);
		}
		else{
			gl_FragColor = vec4(texture2D(iChannel[0], uv ).xyz, 0);
		}
	}
	else{
		gl_FragColor = texture2D(iChannel[0], uv );
	}
}
)!!";

const std::string sEdgeBlur = R"!!(uniform float blurradius;
vec2 fTextureCoord = gl_FragCoord.xy / iResolution.xy;

float gaussBlurAlphaSampling(vec2 samplingRadius) {
    float alpha = 0.0;
	float samplingCount = 32.0;
	vec2 d = samplingRadius / iResolution.xy / samplingCount;

	alpha += texture(iChannel[0], fTextureCoord - 16.0 * d).a * 0.0138;
	alpha += texture(iChannel[0], fTextureCoord - 15.0 * d).a * 0.0158;
	alpha += texture(iChannel[0], fTextureCoord - 14.0 * d).a * 0.018;
	alpha += texture(iChannel[0], fTextureCoord - 13.0 * d).a * 0.0203;
	alpha += texture(iChannel[0], fTextureCoord - 12.0 * d).a * 0.0226;
	alpha += texture(iChannel[0], fTextureCoord - 11.0 * d).a * 0.025;
	alpha += texture(iChannel[0], fTextureCoord - 10.0 * d).a * 0.0274;
	alpha += texture(iChannel[0], fTextureCoord - 9.0 * d).a * 0.0298;
	alpha += texture(iChannel[0], fTextureCoord - 8.0 * d).a * 0.0321;
	alpha += texture(iChannel[0], fTextureCoord - 7.0 * d).a * 0.0343;
	alpha += texture(iChannel[0], fTextureCoord - 6.0 * d).a * 0.0364;
	alpha += texture(iChannel[0], fTextureCoord - 5.0 * d).a * 0.0382;
	alpha += texture(iChannel[0], fTextureCoord - 4.0 * d).a * 0.0397;
	alpha += texture(iChannel[0], fTextureCoord - 3.0 * d).a * 0.0409;
	alpha += texture(iChannel[0], fTextureCoord - 2.0 * d).a * 0.0418;
	alpha += texture(iChannel[0], fTextureCoord - 1.0 * d).a * 0.0424;
	alpha += texture(iChannel[0], fTextureCoord + 0.0 * d).a * 0.0426;
	alpha += texture(iChannel[0], fTextureCoord + 1.0 * d).a * 0.0424;
	alpha += texture(iChannel[0], fTextureCoord + 2.0 * d).a * 0.0418;
	alpha += texture(iChannel[0], fTextureCoord + 3.0 * d).a * 0.0409;
	alpha += texture(iChannel[0], fTextureCoord + 4.0 * d).a * 0.0397;
	alpha += texture(iChannel[0], fTextureCoord + 5.0 * d).a * 0.0382;
	alpha += texture(iChannel[0], fTextureCoord + 6.0 * d).a * 0.0364;
	alpha += texture(iChannel[0], fTextureCoord + 7.0 * d).a * 0.0343;
	alpha += texture(iChannel[0], fTextureCoord + 8.0 * d).a * 0.0321;
	alpha += texture(iChannel[0], fTextureCoord + 9.0 * d).a * 0.0298;
	alpha += texture(iChannel[0], fTextureCoord + 10.0 * d).a * 0.0274;
	alpha += texture(iChannel[0], fTextureCoord + 11.0 * d).a * 0.025;
	alpha += texture(iChannel[0], fTextureCoord + 12.0 * d).a * 0.0226;
	alpha += texture(iChannel[0], fTextureCoord + 13.0 * d).a * 0.0203;
	alpha += texture(iChannel[0], fTextureCoord + 14.0 * d).a * 0.018;
	alpha += texture(iChannel[0], fTextureCoord + 15.0 * d).a * 0.0158;
	alpha += texture(iChannel[0], fTextureCoord + 16.0 * d).a * 0.0138;

    return alpha;
}

float gauss(){
	float totalalpha = 0;
	for (int i = 0; i < 2 ; i++) {
		if (i % 2 == 0) {
			totalalpha += gaussBlurAlphaSampling(vec2(blurradius,0.));
		}else{
			totalalpha += gaussBlurAlphaSampling(vec2(blurradius,1.));
		}
	}
	return totalalpha/2.;
}


void main()
{
	vec2 uv = gl_FragCoord.xy / iResolution.xy;

	gl_FragColor = texture2D(iChannel[0], uv) * (gauss() * 2.0 - 1.0);
}
)!!";