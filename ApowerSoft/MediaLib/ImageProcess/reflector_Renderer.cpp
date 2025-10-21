#include "reflector_Renderer.h"
#include <iostream>
#include <wxlog.h>


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

static cache::lru_cache<std::string, std::shared_ptr<Texture>> s_innerTextures(10);

void GLClearError()
{
	while (glGetError() != GL_NO_ERROR);
}

bool GLLogCall(const char* function, const char* file, int line)
{
	while (GLenum error = glGetError())
	{
		//Log_info("[OpenGL Error] (%d) %s \n %s \n %d", error, function, file, line);
		return false;
	}
	return true;
}


float positions[] = {
	-1, -1, 0.0f, 0.0f,
	 1,-1, 1.0f, 0.0f,
	 1, 1, 1.0f, 1.0f,
	-1, 1, 0.0f, 1.0f
};



Renderer& Renderer::Instance()
{
	static Renderer render;
	return render;
}

void Renderer::UploadTexture(int slot, FilterFrame frame)
{
	if (frame.type == FRAME_3D)
		InputTexture[slot]->setTextureType(Texture::e_textureType::TEXTURE_3D);

	if(frame.wrap == FRAME_CLAMP_TO_EDGE)
		InputTexture[slot]->setTextureWrap(Texture::e_textureWrap::CLAMP_TO_EDGE);

	InputTexture[slot ]->UpdateSize(frame.width, frame.height, frame.deep);
	InputTexture[ slot]->BindForInput(slot);
	InputTexture[ slot]->Upload(frame.data, frame.width, frame.height, frame.width * 4, frame.deep);
}

void Renderer::UploadTextureFromFBO(int slot, int width, int height)
{
	InputTexture[ slot]->UpdateSize(width, height);
	InputTexture[ slot]->BindForInput(slot);
	GLCall(glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, width, height));
}

Renderer::Renderer()
{
	unsigned int indices[] = {
		0,1,2,
		2,3,0
	};

	vb = new VertexBuffer((const void*)positions, 4 * 4 * sizeof(float));
	layout.Push_float(2);
	layout.Push_float(2);
	va.AddBuffer(vb, layout);
	ib = new IndexBuffer(indices, 6);

	m_progress = 0;

	for (size_t i = 0; i < 9; i++)
	{
		InputTexture[i] = new Texture(640, 360, false, i);
	}
	va.Bind();
	ib->Bind();
}


void Renderer::Render(std::vector< FilterFrame> frames, std::vector<Shader*> shaders, const std::vector<float>& sizes, int _baseslot, void* output)
{
	auto base_width = sizes[0];
	auto base_height = sizes[1];

	auto key = std::string{} + std::to_string(base_width) + "_" + std::to_string(base_height);

	std::shared_ptr<Texture>innerTexture = nullptr;
	if (s_innerTextures.exists(key)) {
		innerTexture = s_innerTextures.get(key);
	}else {
		innerTexture = std::make_shared<Texture>(base_width, base_height, true);
		s_innerTextures.put(key, innerTexture);
	}
	innerTexture->UpdateSize(base_width, base_height);
	baseslot = _baseslot;

	glViewport(0, 0, base_width, base_height);

	//似乎framebuffer尺寸必须和texture一致, 否则就会上传失败
	int inputbaseslot = 0;

	for (int i = 0; i < frames.size(); i++)
		UploadTexture(baseslot + i, frames[i]);

	for (int i = 0; i < shaders.size(); i++)
	{
		if (1)
		{
			CurrentShader = shaders[i];
			CurrentShader->Bind();
			CurrentShader->SetUniform2f("iResolution", base_width, base_height);
			CurrentShader->SetTexture("iChannel", { 0,1,2,3,4,5,6,7 });
			CurrentShader->SetTexture("iChannel_3D", { 8 });
		}

		CurrentShader->SetUniform3fv("iChannelResolution", 8, sizes.data());
		CurrentShader->SetUniform1f("iProgress", CurrentShader->GetTime() / CurrentShader->GetDuration());
		CurrentShader->SetUniform1f("iDuration", CurrentShader->GetDuration());
		CurrentShader->SetUniform1f("iTime", CurrentShader->GetTime());
		if (false && i == shaders.size() - 1)
		{//用来渲染到屏幕
			GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));
			CurrentShader->Render(ib->GetCount());

			GLCall(glBindFramebuffer(GL_READ_FRAMEBUFFER, 0));
			GLCall(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, innerTexture->textureframebuffer));
			GLfloat temp[4] = { 0.0, 0.0, 0.0, 1.0 };
			GLCall(glClearBufferfv(GL_COLOR, 0, temp));
			GLCall(glBlitFramebuffer(0, 0, base_width, base_height, 0, 0, base_width, base_height, GL_COLOR_BUFFER_BIT, GL_LINEAR));


			GLCall(glActiveTexture(GL_TEXTURE0));
			GLCall(glBindTexture(GL_TEXTURE_2D, innerTexture->m_RendererID));
		}
		else
		{
			innerTexture->UpdateSize(base_width, base_height);
			innerTexture->BindForFBO();

			GLCall(glDrawBuffer(GL_COLOR_ATTACHMENT0));
			GLuint clearColor[4] = { 0,0,0,0 };
			GLCall(glClearBufferuiv(GL_COLOR, 0, clearColor));
			CurrentShader->Render(ib->GetCount());
			innerTexture->BindForInput(inputbaseslot);
		}
	}

	if (output != NULL)
	{
		GLCall(glReadPixels(0, 0, base_width, base_height, GL_RGBA, GL_UNSIGNED_BYTE, output));
	}

	glFinish();

	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, NULL));
}
