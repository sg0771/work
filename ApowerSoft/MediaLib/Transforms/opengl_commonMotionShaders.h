#pragma once
#include <map>
const char move_right2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 x, 0, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2-2, iProgress*2-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char move_left2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 -x, 0, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2-2, iProgress*2-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char move_up2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 0, x, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2-2, iProgress*2-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char move_down2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 0, -x, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2-2, iProgress*2-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char move_left_bottom2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 -x, -x, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2-2, iProgress*2-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char move_left_top2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 -x, x, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2-2, iProgress*2-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char move_right_up2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 x, x, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2-2, iProgress*2-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char move_right_down2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 x, -x, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2-2, iProgress*2-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char push_right1_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 x, 0, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2, iProgress*2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char push_left1_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 -x, 0, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2, iProgress*2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char push_up1_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 0, x, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2, iProgress*2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char push_down1_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 0, -x, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2, iProgress*2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char push_right2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 x, 0, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2-2, iProgress*2-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char push_left2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 -x, 0, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2-2, iProgress*2-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char push_up2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 0, x, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2-2, iProgress*2-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char push_down2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 0, -x, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*2-2, iProgress*2-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char roll_right2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 x, 0, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*4-2, iProgress*4-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char roll_left2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 -x, 0, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*4-2, iProgress*4-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char roll_up2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 0, x, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*4-2, iProgress*4-2) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char roll_down2_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 0, -x, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress*4-2, iProgress*4-2) * position;
	v_TexCoord =  texCoord;
}
)!!";

const char rotate_counter_clock_360_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform vec2      iResolution; 
uniform float iProgress;
float aspect0 = iResolution.x / iResolution.y;
const float iStartAngle = 0.0;
const float iEndAngle = 360.0;
float iSpeed = abs(iEndAngle - iStartAngle);
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 cos(x), sin(x), 0, 0,
		     -sin(x), cos(x), 0, 0,
			 0, 0, 1, 0,
			 0, 0, 0, 1
		)*m_mat;
	}
void main()
{
	vec2 uv = position.xy;
	uv.y /= aspect0;
	uv = vec4(scale(initmat, radians(iStartAngle + iProgress * iSpeed), radians(iStartAngle + iProgress * iSpeed)) * vec4(uv, 0, 1)).xy;
	uv.y *= aspect0;
	gl_Position = vec4(uv, 0, 1);
	v_TexCoord =  texCoord;
}
)!!";
const char rotate_counter_clock_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform vec2      iResolution; 
uniform float iProgress;
float aspect0 = iResolution.x / iResolution.y;
const float iStartAngle = 315.0;
const float iEndAngle = 360.0;
float iSpeed = abs(iEndAngle - iStartAngle);
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 cos(x), sin(x), 0, 0,
		     -sin(x), cos(x), 0, 0,
			 0, 0, 1, 0,
			 0, 0, 0, 1
		)*m_mat;
	}
void main()
{
	vec2 uv = position.xy;
	uv.y /= aspect0;
	uv = vec4(scale(initmat, radians(iStartAngle + iProgress * iSpeed), radians(iStartAngle + iProgress * iSpeed)) * vec4(uv, 0, 1)).xy;
	uv.y *= aspect0;
	gl_Position = vec4(uv, 0, 1);
	v_TexCoord =  texCoord;
}
)!!";
const char rotate_clock_360_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform vec2      iResolution; 
uniform float iProgress;
float aspect0 = iResolution.x / iResolution.y;
const float iStartAngle = 0.0;
const float iEndAngle = 360.0;
float iSpeed = abs(iEndAngle - iStartAngle);
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 cos(x), -sin(x), 0, 0,
		     sin(x), cos(x), 0, 0,
			 0, 0, 1, 0,
			 0, 0, 0, 1
		)*m_mat;
	}
void main()
{
	vec2 uv = position.xy;
	uv.x *= aspect0;
	uv /= aspect0;
	uv = vec4(scale(initmat, radians(iStartAngle + iProgress * iSpeed), radians(iStartAngle + iProgress * iSpeed)) * vec4(uv, 0, 1)).xy;
	uv.y *= aspect0;
	gl_Position = vec4(uv, 0, 1);
	v_TexCoord =  texCoord;
})!!";
const char rotate_clock_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform vec2      iResolution; 
uniform float iProgress;
float aspect0 = iResolution.x / iResolution.y;
const float iStartAngle = 315.0;
const float iEndAngle = 360.0;
float iSpeed = abs(iEndAngle - iStartAngle);
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 cos(x), -sin(x), 0, 0,
		     sin(x), cos(x), 0, 0,
			 0, 0, 1, 0,
			 0, 0, 0, 1
		)*m_mat;
	}
void main()
{
	vec2 uv = position.xy;
	uv.x *= aspect0;
	uv /= aspect0;
	uv = vec4(scale(initmat, radians(iStartAngle + iProgress * iSpeed), radians(iStartAngle + iProgress * iSpeed)) * vec4(uv, 0, 1)).xy;
	uv.y *= aspect0;
	gl_Position = vec4(uv, 0, 1);
	v_TexCoord =  texCoord;
})!!";

const char zoom_in_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 x, 0, 0, 0,
		     0, y, 0, 0,
			 0, 0, 1, 0,
			 0, 0, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress, iProgress) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char zoom_out_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;
mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 x, 0, 0, 0,
		     0, y, 0, 0,
			 0, 0, 1, 0,
			 0, 0, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, 1-iProgress, 1-iProgress) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char base_vs[] = R"!!(
#version 130 core
in vec4 position;
in vec2 texCoord;
out vec2 v_TexCoord;
uniform float iProgress;

mat4 initmat = mat4(
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1);
mat4 scale( mat4 m_mat,float x, float y) {
		return mat4(
			 1, 0, 0, 0,
		     0, 1, 0, 0,
			 0, 0, 1, 0,
			 0, 0, 0, 1
		)*m_mat;
	}
void main()
{
	gl_Position = scale(initmat, iProgress, iProgress) * position;
	v_TexCoord =  texCoord;
}
)!!";
const char base_fs[] = R"!!(
#version 130 core
out vec4 color;
in vec2 v_TexCoord;
uniform vec2      iResolution; 
uniform sampler2D iChannel[8]; 
uniform vec4      motionRect; 
uniform float iGlobalTime;
void main()
{
	gl_FragColor = texture2D(iChannel[0], v_TexCoord);
}
)!!";
const char base2_fs[] = R"!!(
#version 130 core
out vec4 color;
in vec2 v_TexCoord;
uniform vec2      iResolution; 
uniform sampler2D iChannel[8]; 
uniform vec4      motionRect; 
uniform float iGlobalTime;
uniform vec2      iChannelResolution[2];

float iAspect = iResolution.x / iResolution.y;

float inBounds(vec2 p, vec4 rect) {
	vec2 pt = iResolution.xy * p;
	vec2 lt_out = floor(iResolution.xy * rect.xy) - vec2(1.0);
	vec2 rb_out = ceil(iResolution.xy * (rect.xy + rect.zw)) + vec2(1.0);

	if (all(lessThanEqual(lt_out, pt)) && all(lessThanEqual(pt, rb_out))) {
		vec2 lt_in = lt_out + vec2(1.0);
		vec2 rb_in = rb_out - vec2(1.0);
		
		if (all(lessThanEqual(lt_in, pt)) && all(lessThanEqual(pt, rb_in))) {
			return 1.0;
		} else {
			vec2 center = (lt_in + rb_in) / 2.0;
			float dx = 0.0;
			float dy = 0.0;

			if (pt.x <= center.x) {
				if (pt.y <= center.y) {
					dx = clamp(lt_in.x - pt.x, 0.0, 1.0);
					dy = clamp(lt_in.y - pt.y, 0.0, 1.0);
				} else {
					dx = clamp(lt_in.x - pt.x, 0.0, 1.0);
					dy = clamp(pt.y - rb_in.y, 0.0, 1.0);
				}
			} else {
				if (pt.y <= center.y) {
					dx = clamp(pt.x - rb_in.x, 0.0, 1.0);
					dy = clamp(lt_in.y - pt.y, 0.0, 1.0);
				} else {
					dx = clamp(pt.x - rb_in.x, 0.0, 1.0);
					dy = clamp(pt.y - rb_in.y, 0.0, 1.0);
				}
			}
			
			return (1.0 - dx) * (1.0 - dy);
		}
	} else {
		return 0.0;
	}
}

float inBounds(vec2 p) {
	return inBounds(p, vec4(0.0, 0.0, 1.0, 1.0));
}
int mode=1;
void arrange(inout vec4 rect, vec2 sizeInner, vec2 sizeOutter)
{
	if (mode == 0) {
		rect = vec4(0.0, 0.0, 1.0, 1.0);
	} else {
		float aspectInner = sizeInner.x / sizeInner.y;

		if (iAspect > aspectInner) {
			rect.z = aspectInner / iAspect;
			rect.w = 1.0;
		} else {
			rect.z = 1.0;
			rect.w = iAspect / aspectInner;
		}
		rect.x = 0.5 - rect.z * 0.5;
		rect.y = 0.5 - rect.w * 0.5;
	}
}


void main()
{
	//gl_FragColor = texture2D(iChannel[1], v_TexCoord);


	vec2 uv = v_TexCoord;
	vec4 col = vec4(0.0);

	vec4 rect;
	arrange(rect, iChannelResolution[1].xy, iResolution.xy);

	vec4 col0 = texture2D(iChannel[0], uv);
	vec4 col1 = texture2D(iChannel[1], (uv - rect.xy) / rect.zw);
	col1 = mix(col0, col1, col1.a);
	col = mix(col0, col1, inBounds(uv, rect));
	gl_FragColor = col;
}
)!!";