#pragma once
#include<string>

const std::string sChromakeyShader = R"!!(
#ifdef GL_ES
precision mediump float;
#endif


vec2 fTextureCoord = gl_FragCoord.xy / iResolution.xy;

// 绿色和红色通道的均值差达到此值时, 开始将透明度从 0 过渡到 1.
// 取值范围大多介于 0.5~0.8 之间. 值越大, 被抠除的内容越多.

// 如何自动估计此阈值:
// 	   首先统计 `1.0 - (g - r) * 2` 的颜色分布.
//     产生的统计图是一个左侧 (背景) 和右侧 (前景) 分别很大, 中间是一个沟的图像.
//     选择沟底的中间即可.
uniform float colorDiffThreshold = 0.8;

// 在开始进行平滑时的过渡位置.
// 0~1, 一般对静态环境设置为 1, 动态设置为 0.5.
// 如果等于 0, 会刚好在背景色的边缘位置开始过渡, 会带入背景色.
// 如果等于 1, 会刚好在色差分布的阈值处, 完全不带入背景色, 但是也有可能会忽略运动模糊的部分.
float smoothStartRate = 0.5;

// 在开始平滑到 1 的此线性位置处结束平滑.
// 0~1, 一般对静态环境设置为 0.8, 动态设置为 1.
// 如果等于 0, 会刚好在色差分布的阈值处结束过渡, 过渡范围很小.
// 如果等于 1, 会过渡到直到绿色完全不占优, 过渡范围最大.
float smoothEndRate = 0.75;

// 进行高斯加权采样时的像素半径以获取平滑的 Alpha, 默认为 2, 只需要保证边缘足够顺滑即可.
float alphaSamplingRadius = 2.0;

// 进行高斯加权采样时的像素半径以查找临近的颜色, 默认为 8, 只需要边缘的绿色大致处理干净即可.
float neighboorSamplingRadius = 8.0;


// 线性插值, 根据 x 的范围返回 0~1 的结果.
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

// 高斯采样的坐标偏移和权重.
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

// 获取色差键值.
float getColorDiff(vec4 color) {
	float r = color.r;
	float g = color.g;
	float b = color.b;

	// 获得绿色通道和另外两个通道均值的差.
	// 它的整体量化范围为 0~1, 并且和最后的透明度正相关.
	return 1.0 - 2 * (g - max(r, b));
}

// 进行 5x5 范围的高斯模糊采样.
// 它可以明显加强小的细节例如文字细节边缘的平滑度.
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

// 获取线性插值的系数.
float getLinearFactor(float value, float from, float to) {
	return (value - from) / (to - from);
}

// 根据线性系数进行插值.
float getValueByLinearFactor(float factor, float from, float to) {
	return (to - from) * factor + from;
}

// 获取临近的颜色值, 未考虑预乘.
vec3 getNeighborColor() {
	vec3 totalColor = vec3(0.0);
	float totalWeight = 0.0;
	vec2 d = neighboorSamplingRadius / iResolution.xy / 2.0;

	// 噪声的变化频率.
	float noiseFrequency = 2.0;

	for (int i = 0; i < 25; i++) {
		vec3 xyp = guassianXYP25[i];

		// 由于采样数不足, 对采样坐标进行扰动以防止出现波纹.
		// 扰动过大会出现噪点, 扰动过小会出现波纹. 噪点相比波纹更容易接受.
		float noiseValue = noise2d((gl_FragCoord.xy + xyp.xy) * noiseFrequency) - 0.5;

		// 注意此颜色为非预乘色.
		vec4 color = texture2D(iChannel[0], fTextureCoord + (xyp.xy + noiseValue) * d);

		// 临近颜色的色差键和背景色的远离程度决定了加权的比例.
		float colorDiffWeight = pow(max(getColorDiff(color) - colorDiffThreshold, 0), 4.0);

		// 计算最终权重.
		float colorWeight = xyp.z * colorDiffWeight;

		totalColor += color.rgb * colorWeight;
		totalWeight += colorWeight;
	}

	// 查询附近比较临近的前景色.
	return totalColor / totalWeight;
}

// 获取颜色 uv 值.
vec2 getUV(vec3 color) {
	float u = -0.169 * color.r - 0.33 * color.g + 0.5 * color.b;
	float v = 0.5 * color.r - 0.419 * color.g - 0.081 * color.b;

	return vec2(u, v);
}

// Spill 处理, 将绿色分量降低, 包括边缘和反光产生的绿色.
// 它利用了 rb 通道的平滑来引导 g 通道的修正, 同时保持其平滑.
// 这里有三个选择:
//     保持原色 - 如果当前颜色不和邻近色接近, 并且红蓝通道差比较大, 此时做颜色修正会产生很大的色相改变.
//     偏向邻近色 - 如果当前颜色和邻近色足够接近.
vec3 spillColor(vec3 currentColor, vec3 neighborColor) {

	// 当绿色通道不是最小通道时才处理.
	if (currentColor.g > min(currentColor.r, currentColor.b)) {

		float newGreen = currentColor.g;


		// 获取邻近色绿色通道在红蓝通道中的线性位置.
		// 注意这个参数非常容易溢出, 所以需要进行限制到 -1~1.
		// 它相当于某种最大最小值滤波.
		float neighborGreenFactor = getLinearFactor(neighborColor.g, neighborColor.r, neighborColor.b);
		neighborGreenFactor = clamp(neighborGreenFactor, -1.0, 1.0);

		// 将当前颜色的绿色通道插值, 获得是结果是色相和邻近色最接近的绿色.
		// 本质上其实是搬运 HSL 颜色空间中的色相.
		float neighborLikeGreen = getValueByLinearFactor(neighborGreenFactor, currentColor.r, currentColor.b);

		// 如果红蓝的差距很小, 那么线性系数会置信度过低, 此时为其混入适当的红蓝均值.
		float centralGreen = (currentColor.r + currentColor.b) / 2.0;
		float factorConfidence = linearStep(abs(neighborColor.r - neighborColor.b), 0.05, 0.15);
		neighborLikeGreen = mix(centralGreen, neighborLikeGreen, factorConfidence);

		// 和邻近色的距离.
		float neighborColorDiff = distance(getUV(currentColor), getUV(neighborColor));

		// 如果两个颜色的 UV 距离很大, 那么不再考虑邻近色.
		float neighborColorDiffRate = 1.0 - linearStep(neighborColorDiff, 0.15, 0.25);
		
		// 混入临近色.
		newGreen = mix(newGreen, neighborLikeGreen, neighborColorDiffRate);

		
		// 用于抑制绿色分量的过大变化, 绿色分量最大只会改变 0.15.
		float greenDiff = abs(currentColor.g - newGreen);
		float greenScaleBack = min(0.15 / greenDiff, 1.0);
		newGreen = currentColor.g + (newGreen - currentColor.g) * greenScaleBack;

		// 抑制绿色过度调整.
		// 最佳的方法是处理成绿色变化视图, 然后将其平滑, 但是这是一个单步处理, 所以使用最简单的阈值办法.
		newGreen = min(newGreen, max(currentColor.r, currentColor.b));
		

		currentColor.g = newGreen;
	}

	return currentColor;
}

void main() {

	// 获取当前颜色.
	vec4 currentColor = texture2D(iChannel[0], fTextureCoord);

	// 获取经过了高斯模糊的颜色.
	vec4 blurredColor = getGaussianBlurredColor();

	// 获得色差键.
	float colorDiff = getColorDiff(blurredColor);

	// 线性插值获得过渡的 Alpha.
	// colorDiffThreshold * 2 - 1.0, colorDiffThreshold, 1.0 刚好成等差数列.
	float start = mix(colorDiffThreshold * 2 - 1.0, colorDiffThreshold, smoothStartRate);
	float end = mix(colorDiffThreshold, 1.0, smoothEndRate);
	float alpha = linearStep(colorDiff, start, end);
	
	// 获得临近颜色.
	vec3 neighborColor = getNeighborColor();

	// 调整绿色通道.
	vec4 color = vec4(spillColor(currentColor.rgb, neighborColor), 1.0);
	color *= alpha;

	// 输出颜色.
	gl_FragColor = color;
}

)!!";


const std::string sChromakeyShaderB = R"!!(
#ifdef GL_ES
precision mediump float;
#endif

vec2 fTextureCoord = gl_FragCoord.xy / iResolution.xy;



// 绿色和红色通道的均值差达到此值时, 开始将透明度从 0 过渡到 1.
// 取值范围大多介于 0.5~0.8 之间. 值越大, 被抠除的内容越多.

// 如何自动估计此阈值:
// 	   首先统计 `1.0 - (g - r) * 2` 的颜色分布.
//     产生的统计图是一个左侧 (背景) 和右侧 (前景) 分别很大, 中间是一个沟的图像.
//     选择沟底的中间即可.
uniform float colorDiffThreshold = 0.8;

// 在开始进行平滑时的过渡位置.
// 0~1, 一般对静态环境设置为 1, 动态设置为 0.5.
// 如果等于 0, 会刚好在背景色的边缘位置开始过渡, 会带入背景色.
// 如果等于 1, 会刚好在色差分布的阈值处, 完全不带入背景色, 但是也有可能会忽略运动模糊的部分.
float smoothStartRate = 0.5;

// 在开始平滑到 1 的此线性位置处结束平滑.
// 0~1, 一般对静态环境设置为 0.8, 动态设置为 1.
// 如果等于 0, 会刚好在色差分布的阈值处结束过渡, 过渡范围很小.
// 如果等于 1, 会过渡到直到绿色完全不占优, 过渡范围最大.
float smoothEndRate = 0.75;

// 进行高斯加权采样时的像素半径以获取平滑的 Alpha, 默认为 2, 只需要保证边缘足够顺滑即可.
float alphaSamplingRadius = 2.0;

// 进行高斯加权采样时的像素半径以查找临近的颜色, 默认为 8, 只需要边缘的绿色大致处理干净即可.
float neighboorSamplingRadius = 8.0;

// 线性插值, 根据 x 的范围返回 0~1 的结果.
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

// 高斯采样的坐标偏移和权重.
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

// 获取色差键值.
float getColorDiff(vec4 color) {
	float r = color.g;
	float g = color.r;
	float b = color.b;

	// 获得绿色通道和另外两个通道均值的差.
	// 它的整体量化范围为 0~1, 并且和最后的透明度正相关.
	return 1.0 - 2 * (g - max(r, b));
}

// 进行 5x5 范围的高斯模糊采样.
// 它可以明显加强小的细节例如文字细节边缘的平滑度.
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

// 获取线性插值的系数.
float getLinearFactor(float value, float from, float to) {
	return (value - from) / (to - from);
}

// 根据线性系数进行插值.
float getValueByLinearFactor(float factor, float from, float to) {
	return (to - from) * factor + from;
}

// 获取临近的颜色值, 未考虑预乘.
vec3 getNeighborColor() {
	vec3 totalColor = vec3(0.0);
	float totalWeight = 0.0;
	vec2 d = neighboorSamplingRadius / iResolution.xy / 2.0;

	// 噪声的变化频率.
	float noiseFrequency = 2.0;

	for (int i = 0; i < 25; i++) {
		vec3 xyp = guassianXYP25[i];

		// 由于采样数不足, 对采样坐标进行扰动以防止出现波纹.
		// 扰动过大会出现噪点, 扰动过小会出现波纹. 噪点相比波纹更容易接受.
		float noiseValue = noise2d((gl_FragCoord.xy + xyp.xy) * noiseFrequency) - 0.5;

		// 注意此颜色为非预乘色.
		vec4 color = texture2D(iChannel[0], fTextureCoord + (xyp.xy + noiseValue) * d);

		// 临近颜色的色差键和背景色的远离程度决定了加权的比例.
		float colorDiffWeight = pow(max(getColorDiff(color) - colorDiffThreshold, 0), 4.0);

		// 计算最终权重.
		float colorWeight = xyp.z * colorDiffWeight;

		totalColor += color.rgb * colorWeight;
		totalWeight += colorWeight;
	}

	// 查询附近比较临近的前景色.
	return totalColor / totalWeight;
}

// 获取颜色 uv 值.
vec2 getUV(vec3 color) {
	float u = -0.169 * color.g - 0.33 * color.r + 0.5 * color.b;
	float v = 0.5 * color.g - 0.419 * color.r - 0.081 * color.b;

	return vec2(u, v);
}

// Spill 处理, 将绿色分量降低, 包括边缘和反光产生的绿色.
// 它利用了 rb 通道的平滑来引导 g 通道的修正, 同时保持其平滑.
// 这里有三个选择:
//     保持原色 - 如果当前颜色不和邻近色接近, 并且红蓝通道差比较大, 此时做颜色修正会产生很大的色相改变.
//     偏向邻近色 - 如果当前颜色和邻近色足够接近.
vec3 spillColor(vec3 currentColor, vec3 neighborColor) {

	// 当绿色通道不是最小通道时才处理.
	if (currentColor.r > min(currentColor.g, currentColor.b)) {

		float newGreen = currentColor.r;


		// 获取邻近色绿色通道在红蓝通道中的线性位置.
		// 注意这个参数非常容易溢出, 所以需要进行限制到 -1~1.
		// 它相当于某种最大最小值滤波.
		float neighborGreenFactor = getLinearFactor(neighborColor.r, neighborColor.g, neighborColor.b);
		neighborGreenFactor = clamp(neighborGreenFactor, -1.0, 1.0);

		// 将当前颜色的绿色通道插值, 获得是结果是色相和邻近色最接近的绿色.
		// 本质上其实是搬运 HSL 颜色空间中的色相.
		float neighborLikeGreen = getValueByLinearFactor(neighborGreenFactor, currentColor.g, currentColor.b);

		// 如果红蓝的差距很小, 那么线性系数会置信度过低, 此时为其混入适当的红蓝均值.
		float centralGreen = (currentColor.g + currentColor.b) / 2.0;
		float factorConfidence = linearStep(abs(neighborColor.g - neighborColor.b), 0.05, 0.15);
		neighborLikeGreen = mix(centralGreen, neighborLikeGreen, factorConfidence);

		// 和邻近色的距离.
		float neighborColorDiff = distance(getUV(currentColor), getUV(neighborColor));

		// 如果两个颜色的 UV 距离很大, 那么不再考虑邻近色.
		float neighborColorDiffRate = 1.0 - linearStep(neighborColorDiff, 0.15, 0.25);
		
		// 混入临近色.
		newGreen = mix(newGreen, neighborLikeGreen, neighborColorDiffRate);

		
		// 用于抑制绿色分量的过大变化, 绿色分量最大只会改变 0.15.
		float greenDiff = abs(currentColor.r - newGreen);
		float greenScaleBack = min(0.15 / greenDiff, 1.0);
		newGreen = currentColor.r + (newGreen - currentColor.r) * greenScaleBack;

		// 抑制绿色过度调整.
		// 最佳的方法是处理成绿色变化视图, 然后将其平滑, 但是这是一个单步处理, 所以使用最简单的阈值办法.
		newGreen = min(newGreen, max(currentColor.g, currentColor.b));
		

		currentColor.r = newGreen;
	}

	return currentColor;
}

void main() {

	// 获取当前颜色.
	vec4 currentColor = texture2D(iChannel[0], fTextureCoord);

	// 获取经过了高斯模糊的颜色.
	vec4 blurredColor = getGaussianBlurredColor();

	// 获得色差键.
	float colorDiff = getColorDiff(blurredColor);

	// 线性插值获得过渡的 Alpha.
	// colorDiffThreshold * 2 - 1.0, colorDiffThreshold, 1.0 刚好成等差数列.
	float start = mix(colorDiffThreshold * 2 - 1.0, colorDiffThreshold, smoothStartRate);
	float end = mix(colorDiffThreshold, 1.0, smoothEndRate);
	float alpha = linearStep(colorDiff, start, end);
	
	// 获得临近颜色.
	vec3 neighborColor = getNeighborColor();

	// 调整绿色通道.
	vec4 color = vec4(spillColor(currentColor.rgb, neighborColor), 1.0);
	color *= alpha;

	// 输出颜色.
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