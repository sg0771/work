#include "reflector_VertexArray.h"
#include "reflector_VertexBufferLayout.h"
#include "reflector_Renderer.h"
VertexArray::VertexArray()
{
	GLCall(glGenVertexArrays(1, &m_RendererID));
	
}

VertexArray::~VertexArray()
{
}

void VertexArray::AddBuffer(const VertexBuffer*  vb, const VertexBufferLayout & layout)
{
	Bind();
	vb->Bind();
	const auto& elements = layout.GetElements();
	uintptr_t offset = 0;
	for (unsigned int i =0; i<elements.size(); i++ )
	{
		const auto& element = elements[i];
		GLCall(glEnableVertexAttribArray(i));
		
		GLCall(glVertexAttribPointer(i, element.count , element.type, element.normalized, layout.GetStride(), (void*)offset));
		offset += element.count* sizeof(element.type);
	}
	
}

void VertexArray::Bind() const
{
	GLCall(glBindVertexArray(m_RendererID));
}

void VertexArray::Unbind()
{
	GLCall(glBindVertexArray(0));
}
