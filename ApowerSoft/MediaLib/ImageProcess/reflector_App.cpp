#include "reflector_App.h"
#include "Utils.hpp"

//OpenGL32.dll
#define  glewGetProcAddress(name)   LibInst::GetInst().m_wglGetProcAddress((LPCSTR)name)
#define  glGetStringi             LibInst::GetInst().m_glGetStringi
#define  glGetIntegerv            LibInst::GetInst().m_glGetIntegerv
#define  glGetString              LibInst::GetInst().m_glGetString
#define  glClear                  LibInst::GetInst().m_glClear
#define  wglCreateContext LibInst::GetInst().m_wglCreateContext
#define wglDeleteContext LibInst::GetInst().m_wglDeleteContext
#define wglGetProcAddress LibInst::GetInst().m_wglGetProcAddress
#define wglGetCurrentDC LibInst::GetInst().m_wglGetCurrentDC
#define wglGetCurrentContext LibInst::GetInst().m_wglGetCurrentContext
#define wglMakeCurrent LibInst::GetInst().m_wglMakeCurrent
#define wglShareLists  LibInst::GetInst().m_wglShareLists

#define glTexImage2D LibInst::GetInst().m_glTexImage2D
#define glDrawBuffer LibInst::GetInst().m_glDrawBuffer
#define glDrawArrays LibInst::GetInst().m_glDrawArrays
#define glDisable LibInst::GetInst().m_glDisable
#define glDrawElements LibInst::GetInst().m_glDrawElements
#define glFinish LibInst::GetInst().m_glFinish
#define glFlush LibInst::GetInst().m_glFlush
#define glReadBuffer LibInst::GetInst().m_glReadBuffer
#define glReadPixels LibInst::GetInst().m_glReadPixels
#define glTexParameteri LibInst::GetInst().m_glTexParameteri
#define glTexSubImage2D LibInst::GetInst().m_glTexSubImage2D
#define glViewport LibInst::GetInst().m_glViewport
#define glBindTexture LibInst::GetInst().m_glBindTexture
#define glCopyTexSubImage2D LibInst::GetInst().m_glCopyTexSubImage2D
#define glGetError LibInst::GetInst().m_glGetError
#define glDeleteTextures LibInst::GetInst().m_glDeleteTextures
#define glGenTextures LibInst::GetInst().m_glGenTextures
#define glBlendFunc LibInst::GetInst().m_glBlendFunc
#define glEnable LibInst::GetInst().m_glEnable

#define glDrawElements LibInst::GetInst().m_glDrawElements
App::App()
{
	/* Initialize the library */
	if (!glfwInit())
		return;
	const auto ctx = glfwGetCurrentContext();
	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);


	//glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	//glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	//glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


	/* Create a windowed mode window and its OpenGL context */
	window = glfwCreateWindow(1, 1, "Hello World", NULL, ctx);
	if (!window)
	{
		glfwTerminate();
		return;
	}

	/* Make the window's context current */
	glfwMakeContextCurrent(window);

	//glfwSwapInterval(1);

	if (glewInit() != GLEW_OK)
		std::cout << "Error" << std::endl;

	std::cout << glGetString(GL_VERSION) << std::endl;
	{
		//GLCall(glEnable(GL_BLEND));
		//GLCall(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));

		//GLCall(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

		
	}

	GLCall(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

}
App::~App()
{
	glfwTerminate();
}
