#pragma once
#include "reflector_VertexBuffer.h"

class VertexBufferLayout;

class VertexArray {
		
private:
	unsigned int m_RendererID=0;
public:

	VertexArray();
	~VertexArray();
	void AddBuffer(const VertexBuffer* vb, const VertexBufferLayout& layout);
	void Bind() const;
	void Unbind();
};