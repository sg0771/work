#include "reflector_texture.h"
#include "reflector_Renderer.h"


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


unsigned char * Texture::BufferCache[8];
Texture::Texture(bool rtt):m_width(0), m_height(0), m_BPP(0), m_RTT(rtt), 
m_textureType(TEXTURE_2D), m_textureWrap(MIRRORED_REPEAT)
{
	m_RendererID = 0;
}


Texture::Texture(int width, int height, bool rtt, int slot) :m_RTT(rtt), m_Slot(slot), m_RendererID(0), 
m_textureType(TEXTURE_2D), m_textureWrap(MIRRORED_REPEAT)
{
	UpdateSize(width, height);
}

Texture::~Texture()
{
	GLCall(glDeleteTextures(m_textureType, &m_RendererID));
}

void Texture::BindForInput(unsigned int slot/*=0*/) 
{
	m_Slot = slot;
	if (m_RTT)
	{
		glBindFramebuffer(GL_READ_FRAMEBUFFER, this->framebufferName);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, this->textureframebuffer);
		GLfloat temp[4] = { 0.0, 0.0, 0.0, 1.0 };
		glClearBufferfv(GL_COLOR, 0, temp);
		glBlitFramebuffer(0, 0, m_width, m_height, 0, 0, m_width, m_height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
		/*glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);*/
	}
	
	GLCall(glActiveTexture(GL_TEXTURE0 + slot));
	GLCall(glBindTexture(m_textureType, m_RendererID));
	

}

void Texture::BindForFBO() const
{
	GLCall(glBindFramebuffer(GL_FRAMEBUFFER, framebufferName));
}

void Texture::Unbind() const
{
	GLCall(glActiveTexture(GL_TEXTURE0 + m_Slot));
	GLCall(glBindTexture(m_textureType, 0));
}

void Texture::UpdateSize(int _width, int _height, int _deep)
{
	if (_width == m_width && _height == m_height)
	{
		return;
	}
	m_width = _width;
	m_height = _height;
	if (m_RendererID > 0)
	{
		glDeleteTextures(1, &m_RendererID);
		m_RendererID = 0;
	}
	if (framebufferName > 0)
	{
		glDeleteFramebuffers(1, &framebufferName);
		framebufferName = 0;
	}



	GLCall(glGenTextures(1, &m_RendererID));
	GLCall(glActiveTexture(GL_TEXTURE0 + m_Slot));
	GLCall(glBindTexture(m_textureType, m_RendererID));

	GLCall(glTexParameteri(m_textureType, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(m_textureType, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
	GLCall(glTexParameteri(m_textureType, GL_TEXTURE_WRAP_T, m_textureWrap));
	GLCall(glTexParameteri(m_textureType, GL_TEXTURE_WRAP_S, m_textureWrap));
	bool flag = (m_textureType == TEXTURE_3D);
	if (flag)
	{
		GLCall(glTexParameteri(m_textureType, GL_TEXTURE_WRAP_R, GL_MIRRORED_REPEAT));
	}
	/*
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
	GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));*/
	if (m_height == 1)
	{
		//GLCall(glTexImage1D(GL_TEXTURE_1D, 0, GL_R32F, m_width, 0, GL_RED, GL_FLOAT, 0));
		GLCall(glTexImage2D(m_textureType, 0, GL_R32F, m_width, m_height, 0, GL_RED, GL_UNSIGNED_BYTE, 0));
	}
	else
	{
		if (!flag)
		{
			GLCall(glTexImage2D(m_textureType, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0));
		}
		else
		{
			GLCall(glTexImage3D(m_textureType, 0, GL_RGBA8, m_width, m_height, _deep, 0, GL_RGBA, GL_UNSIGNED_BYTE, 0));
		}
	}
	if (m_RTT)
	{

		//新渲染方式,渲染到一个renderbuffer, 后面再从renderbuffer 对应的fbo 拷贝到texture对应的fbo
		//create textureframebuffer
		GLCall(glGenFramebuffers(1, &textureframebuffer));
		glBindFramebuffer(GL_FRAMEBUFFER, textureframebuffer);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_RendererID, 0);

		GLCall(glGenFramebuffers(1, &framebufferName));
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, framebufferName));
		
		//glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glGenRenderbuffers(1, &colorrenderbuffer);
		glBindRenderbuffer(GL_RENDERBUFFER, colorrenderbuffer);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA, m_width, m_height);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, colorrenderbuffer);



		//create a framebuffer

		////传统方式, 直接渲染到texture上
		/*glGenRenderbuffers(1, &renderbufferName);
		glBindRenderbuffer(GL_RENDERBUFFER, renderbufferName);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, m_width, m_height);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);*/
		////attach the texture to FBO color attachement point
		//GLCall(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_RendererID, 0));

		//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, renderbufferName);


		// check the FBO status
		GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
		if (status != GL_FRAMEBUFFER_COMPLETE)
		{
			//ASSERT(GLLogCall("glCheckFramebufferStatus", __FILE__, __LINE__)
		}


		GLenum DrawBuffers[1] = { GL_COLOR_ATTACHMENT0 };
		GLCall(glDrawBuffers(1, DrawBuffers));
		GLCall(glBindFramebuffer(GL_FRAMEBUFFER, 0));

		////render to Framebuffer

		//glGenFramebuffers(1, &framebufferMsaaName);
		//glBindFramebuffer(GL_FRAMEBUFFER, framebufferMsaaName);

		//GLCall(glGenRenderbuffers(1, &renderbufferName));
		//glBindRenderbuffer(GL_RENDERBUFFER, renderbufferName);
		//int msaa = 4;
		//glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaa, GL_RGBA, m_width, m_height);
		//glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, renderbufferName);
	}
	
	//GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

void Texture::Upload(unsigned char * buf, int width, int height, int pitch, int _deep)
{
	/*if (BufferCache[m_Slot]!= buf)
	{*/
		//std::uint32_t crc = CRC::Calculate(buf, width*height * 4 / 101, CRC::CRC_32());
		/*if (m_CRC!= crc)
		{
			m_CRC = crc;*/
			if (height == 1)
			{
				GLCall(glTexImage2D(m_textureType, 0, GL_R32F, m_width, m_height, 0, GL_RED, GL_UNSIGNED_BYTE, buf));
			}
			else
			{
				if (m_textureType == TEXTURE_2D)
				{
					GLCall(glTexImage2D(m_textureType, 0, GL_RGBA8, m_width, m_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf));
				}
				else
				{
					GLCall(glTexImage3D(m_textureType, 0, GL_RGBA8, m_width, m_height, _deep, 0, GL_RGBA, GL_UNSIGNED_BYTE, buf));
				}
				
			}
				//}
		
		/*BufferCache[m_Slot] = buf;
	}*/
}

unsigned int Texture::GetTexture()
{
	return m_RendererID;
}

void Texture::setTextureType(e_textureType e_type)
{
	m_textureType = e_type;
}

void Texture::setTextureWrap(e_textureWrap e_type)
{
	m_textureWrap = e_type;
}

