#include "ImageProcess_ShaderManager.h"

#include <glew/glew.h>
#include <GLFW/glfw3.h>
#include <memory>


//µ¥ÀýÄ£ÐÍ
ShaderManager& ShaderManager::GetInst() {
	static ShaderManager s_gpShaderManager;
	return s_gpShaderManager;
}

ShaderManager::ShaderManager()
{
	if (glfwInit()== GLFW_TRUE)
	{
		auto ctx = glfwGetCurrentContext();
		glfwDefaultWindowHints();
		glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
		const auto hwnd = glfwCreateWindow(1, 1, "", NULL, ctx);
		if (hwnd != nullptr)
		{
			glfwMakeContextCurrent(hwnd);
		}
		GLint err = glewInit();
	}
}
int ShaderManager::InitFrameBuffer(int width, int height)
{
	GLuint renderbuffer;
	GLuint framebuffer;
	glGenRenderbuffers(1, &renderbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, renderbuffer);
	glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, width, height);

	glGenFramebuffers(1, &framebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
	glFramebufferRenderbuffer(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbuffer);
	return 0;
}


ShaderManager::~ShaderManager()
{
}
