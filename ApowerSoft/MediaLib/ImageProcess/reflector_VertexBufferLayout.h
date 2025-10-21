#pragma once
#include <vector>
#include "../glew/glew.h"

struct VertexBufferElement{

	unsigned int type=0;
	unsigned int count=0;
	unsigned char normalized=0;
};

class VertexBufferLayout
{
private:
	std::vector<VertexBufferElement> m_Elements;
	unsigned int m_Stride;
public:
	VertexBufferLayout() {
		m_Stride = 0;
		m_Elements.clear();
	};
	~VertexBufferLayout() {};


	void Push_float(unsigned int count)
	{
		m_Elements.push_back({ GL_FLOAT,count, GL_FALSE });
		m_Stride += count * sizeof(GLfloat);
	}


	void Push_uint(unsigned int count)
	{
		m_Elements.push_back({ GL_UNSIGNED_INT,count, GL_FALSE});
		m_Stride += count * sizeof(GLuint);
	}


	void Push_uchar(unsigned int count)
	{
		m_Elements.push_back({ GL_UNSIGNED_BYTE,count, GL_TRUE});
		m_Stride += count * sizeof(GLchar);
	}

	
	inline unsigned int GetStride()const { return m_Stride; }
	inline  std::vector<VertexBufferElement> GetElements()const { return m_Elements; }


};

