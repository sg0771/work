#pragma once
#include <string>
//#include "reflector_CRC.h"
#include "../glew/glew.h"

class  Texture
{
public:
	enum e_textureType
	{
		TEXTURE_2D = GL_TEXTURE_2D,
		TEXTURE_3D = GL_TEXTURE_3D
	};
	enum e_textureWrap
	{
		MIRRORED_REPEAT = GL_MIRRORED_REPEAT,
		CLAMP_TO_EDGE = GL_CLAMP_TO_EDGE
	};
public:
	Texture(bool rtt);
	 //rtt 是否用于render to texture
	 Texture(int width, int height, bool rtt, int slot=0);
	~ Texture();
	void BindForInput(unsigned int slot=0) ;
	void BindForFBO() const;
	void Unbind()const;
	inline int GetWidth()const { return m_width; }
	inline int GetHeight()const { return m_height; }
	void UpdateSize(int _width, int _height, int _deep = 0);
	void Upload(unsigned char* buf, int width, int height, int pitch, int _deep = 0);
	unsigned int GetTexture();
	static unsigned char * BufferCache[8];
	
	unsigned int m_RendererID;// = 0;
	unsigned int framebufferName=0;
	unsigned int textureframebuffer=0;
	unsigned int colorrenderbuffer=0;
private:
	unsigned int m_Slot = 0;
	unsigned int m_CRC = 0;
	std::string m_FilePath="";
	unsigned char* m_LocalBuffer=NULL;
	int m_width=0, m_height=0, m_BPP=0, m_RTT=0, m_deep = 0;
// 纹理属性相关操作
public:
	// 设置纹理类型
	void setTextureType(e_textureType e_type);
	// 设置纹理环绕类型
	void setTextureWrap(e_textureWrap e_type);
private:
	e_textureType m_textureType;
	e_textureWrap m_textureWrap;
};

