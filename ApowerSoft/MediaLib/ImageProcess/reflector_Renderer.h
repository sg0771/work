#pragma once
#include "../glew/glew.h"
#include "reflector_VertexArray.h"
#include "reflector_VertexBuffer.h"
#include "reflector_IndexBuffer.h"
#include "reflector_Shader.h"
#include <Utils.hpp>
#include "reflector_texture.h"
#include "reflector_VertexBufferLayout.h"
#include <vector>
#include "reflector_Core.h"


class Renderer
{ 
	//ת��, �˾�����Ҫ�߼�ʹ��0,1��texture slot, 
	//��Ч,, ʹ��4,5��texture slot
	int baseslot = 0;
	void* m_outbuffer;
	GLuint pboIds[2];
private:
	float m_progress=0;
	void UploadTexture(int slot, FilterFrame frame);
	void UploadTextureFromFBO(int slot, int width, int height);
	Texture* innerTexture;
	Texture* InputTexture[9];
	Shader* CurrentShader;
	IndexBuffer* ib=NULL;
	VertexArray va;
	VertexBuffer* vb= NULL;
	VertexBufferLayout layout;

public:
	Renderer();
	static Renderer& Instance();
	virtual void Render(std::vector< FilterFrame> frames, std::vector<Shader*> shaders, const std::vector<float>& sizes, int _baseslot = 0, void* output = NULL);
};