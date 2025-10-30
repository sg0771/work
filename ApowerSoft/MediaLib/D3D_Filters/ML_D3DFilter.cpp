#include "ML_D3DFilter.h"

struct Vertex
{
	Vertex() {}
	Vertex(
		float x, float y, float z,
		float nx, float ny, float nz,
		float u, float v)
	{
		_x = x;  _y = y;  _z = z;
		_nx = nx; _ny = ny; _nz = nz;
		_u = u;  _v = v;
	}
	float _x, _y, _z;
	float _nx, _ny, _nz;
	float _u, _v;
	static const DWORD FVF;

};
const DWORD Vertex::FVF = D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1;

ML_Texture*ID3DFilter::innerTexture[2];
int ID3DFilter::innerTextureIndex;
ML_Texture*ID3DFilter::InputTexture[8];
ML_Texture*ID3DFilter::lutTexture;
ML_Texture*ID3DFilter::OutputTexture=0;
ML_Texture*ID3DFilter::StagTexture=0;
ML_Shader* ID3DFilter::CurrentFilterSegment=0;
IDirect3DVertexBuffer9* ID3DFilter::Quad = 0;
IDirect3DDevice9Ex* ID3DFilter::m_Device=0;


const static uint8_t VertexShaderContent[164] = {
0x00, 0x03, 0xFE, 0xFF, 0xFE, 0xFF, 0x14, 0x00, 0x43, 0x54, 0x41, 0x42, 0x1C, 0x00, 0x00, 0x00,
0x23, 0x00, 0x00, 0x00, 0x00, 0x03, 0xFE, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x00, 0x01, 0x00, 0x00, 0x1C, 0x00, 0x00, 0x00, 0x76, 0x73, 0x5F, 0x33, 0x5F, 0x30, 0x00, 0x4D,
0x69, 0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x20, 0x28, 0x52, 0x29, 0x20, 0x48, 0x4C, 0x53,
0x4C, 0x20, 0x53, 0x68, 0x61, 0x64, 0x65, 0x72, 0x20, 0x43, 0x6F, 0x6D, 0x70, 0x69, 0x6C, 0x65,
0x72, 0x20, 0x31, 0x30, 0x2E, 0x31, 0x00, 0xAB, 0x1F, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x80,
0x00, 0x00, 0x0F, 0x90, 0x1F, 0x00, 0x00, 0x02, 0x05, 0x00, 0x00, 0x80, 0x01, 0x00, 0x0F, 0x90,
0x1F, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x0F, 0xE0, 0x1F, 0x00, 0x00, 0x02,
0x05, 0x00, 0x00, 0x80, 0x01, 0x00, 0x0F, 0xE0, 0x01, 0x00, 0x00, 0x02, 0x00, 0x00, 0x0F, 0xE0,
0x00, 0x00, 0xE4, 0x90, 0x01, 0x00, 0x00, 0x02, 0x01, 0x00, 0x0F, 0xE0, 0x01, 0x00, 0xE4, 0x90,
0xFF, 0xFF, 0x00, 0x00
};




HRESULT IMulD3DFilter::SetParameter(void * param)
{
	for (size_t i = 0; i < childFilters.size(); i++)
	{
		childFilters[i]->SetParameter(param);
	}
	return 0;
}



D3DFilter::~D3DFilter()
{
	filters.clear();
}

//将frame渲染到目标surface上
//outputtexture轮流渲染
 void D3DFilter::Render(std::vector<FilterFrame> frames, int width, int height, unsigned char* output)
{
	 HRESULT hr;

	 for (size_t i = 0; i < frames.size(); i++)
	 {
		 if (frames[i].type == FRAME_3D)
			 InputTexture[i]->setTextureType(ML_Texture::e_textureType::TEXTURE_3D);
		 else
			 InputTexture[i]->setTextureType(ML_Texture::e_textureType::TEXTURE_2D);// InputTexture is static !

		 InputTexture[i]->UpdateSize(frames[i].width, frames[i].height, frames[i].deep);
		 InputTexture[i]->Upload(frames[i].data, frames[i].width, frames[i].height, frames[i].pitch, frames[i].deep);
		 filters[0]->SetTexture(frames[i].texturename, InputTexture[i]->GetTexture());
	 }
	 

	 OutputTexture->UpdateSize(width, height);
	 innerTexture[0]->UpdateSize(width, height);

	 //DXCall(m_Device->BeginScene());
	 ML_Shader::CurrentFilterSegment = NULL;

	 
	 for (int i = 0; i < filters.size(); i++)
	 {
		 filters[i]->SetSize(width, height);

		 if (ML_Shader::CurrentFilterSegment != filters[i])
		 {
			 ML_Shader::CurrentFilterSegment = filters[i];
			 filters[i]->Bind();
		 }

		 DXCall(m_Device->SetRenderTarget(0, OutputTexture->GetSurface()));
		 DXCall(m_Device->BeginScene());
		 filters[i]->Render();
		 DXCall(m_Device->EndScene());

		 RECT rcsrc{ 0,0,width, height };
		 DXCall(m_Device->StretchRect(OutputTexture->GetSurface(), &rcsrc, innerTexture[0]->GetSurface(), &rcsrc, D3DTEXF_LINEAR));
		 filters[0]->SetTexture("inputtexture", innerTexture[0]->GetTexture());


		 ////当渲染到最后一步, 绘制到输出画板
		 ////否则, 绘制到中转画板, 然后将中转画板绑定到 inputtexture 插槽
		 //if (i == filters.size()-1&& output ==  NULL)
		 //{
			// OutputTexture->UpdateSize(width, height);
			// DXCall(m_Device->SetRenderTarget(0, OutputTexture->GetSurface()));
			// filters[i]->Render();
		 //}
		 //else
		 //{
			// innerTexture[innerTextureIndex]->UpdateSize(width, height);
			// DXCall(m_Device->SetRenderTarget(0, innerTexture[innerTextureIndex]->GetSurface()));
			// filters[i]->Render();
			// filters[i]->SetTexture("inputtexture", innerTexture[innerTextureIndex]->GetTexture());
			// innerTextureIndex = (innerTextureIndex+1)%2;


			// //渲染结果送入
			// 
			// //CopyResource()
		 //}
		 
			
	 }
	 //DXCall(m_Device->EndScene());
	 //渲染中间状态, 将数据导出到cpu
	 if (output)
	 {
		 StagTexture->UpdateSize(width, height);
		 DXCall(m_Device->GetRenderTargetData(innerTexture[0]->GetSurface(), StagTexture->GetSurface()));
		 StagTexture->Download(output);
	 }
}

 HRESULT ID3DFilter::Release()
 {
	 return 0;
}
 HRESULT  ID3DFilter::UnInit(IDirect3DDevice9Ex* device)
 {
	 delete InputTexture[0];
	 delete InputTexture[1];
	 delete InputTexture[2];
	 delete InputTexture[3];
	 delete	innerTexture[0];
	 delete innerTexture[1];
	 delete	 OutputTexture;
	 delete	 StagTexture;
	 delete	 lutTexture;

	 Quad->Release();

	 return S_OK;
 }

 HRESULT  ID3DFilter::Init(IDirect3DDevice9Ex* device)
 {
	 m_Device = device;
	 innerTextureIndex = 0;
	 D3DVERTEXELEMENT9 decl[] = {
		 {0, 0, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 0},
		 {0, 12, D3DDECLTYPE_FLOAT3, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_POSITION, 1},
		 {0, 24, D3DDECLTYPE_FLOAT2, D3DDECLMETHOD_DEFAULT, D3DDECLUSAGE_TEXCOORD, 0},
		 D3DDECL_END()
	 };

	 IDirect3DVertexDeclaration9* _decl = 0;
	 m_Device->CreateVertexDeclaration(decl, &_decl);
	 m_Device->SetVertexDeclaration(_decl);

	 HRESULT hr;
	 DXCall(m_Device->CreateVertexBuffer(6 * sizeof(Vertex), D3DUSAGE_WRITEONLY, 0, D3DPOOL_DEFAULT, &Quad, 0));

	 Vertex* v;
	 DXCall(Quad->Lock(0, 0, (void**)&v, 0));
	 v[0] = Vertex(-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1);
	 v[1] = Vertex(-1.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f);
	 v[2] = Vertex(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1, 0.0f);

	 v[3] = Vertex(-1.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1);
	 v[4] = Vertex(1.0f, 1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1, 0.0f);
	 v[5] = Vertex(1.0f, -1.0f, 1.0f, 0.0f, 0.0f, -1.0f, 1, 1);

	 DXCall(Quad->Unlock());
	 m_Device->SetStreamSource(0, Quad, 0, 6 * sizeof(Vertex));
	 DXCall(m_Device->SetRenderState(D3DRS_LIGHTING, false));
	 m_Device->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	 m_Device->SetRenderState(D3DRS_LIGHTING, FALSE);
	 m_Device->SetRenderState(D3DRS_ZENABLE, FALSE);
	 m_Device->SetRenderState(D3DRS_STENCILENABLE, FALSE);
	 m_Device->SetRenderState(D3DRS_ALPHABLENDENABLE, FALSE);
	 m_Device->SetRenderState(D3DRS_ALPHATESTENABLE, FALSE);
	 m_Device->SetRenderState(D3DRS_SCISSORTESTENABLE, FALSE);
	 m_Device->SetRenderState(D3DRS_COLORWRITEENABLE, D3DCOLORWRITEENABLE_ALPHA | D3DCOLORWRITEENABLE_BLUE | D3DCOLORWRITEENABLE_GREEN | D3DCOLORWRITEENABLE_RED);
	 m_Device->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);
	 m_Device->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
	 m_Device->SetSamplerState(0, D3DSAMP_MIPFILTER, D3DTEXF_NONE);
	 m_Device->SetSamplerState(0, D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP);
	 m_Device->SetSamplerState(0, D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP);

	 DXCall(m_Device->SetStreamSource(0, Quad, 0, sizeof(Vertex)));
	 DXCall(m_Device->SetFVF(Vertex::FVF));
	 D3DXMATRIX proj;
	 LibInst::GetInst().mD3DXMatrixPerspectiveFovLH(&proj, D3DX_PI * 0.5f, 1.0f, 1.0f, 100.0f);
	 DXCall(m_Device->SetTransform(D3DTS_PROJECTION, &proj));

	 InputTexture[0] = new ML_Texture(m_Device, 1280, 720, D3DUSAGE_DYNAMIC);
	 InputTexture[1] = new ML_Texture(m_Device, 640, 360, D3DUSAGE_DYNAMIC);
	 InputTexture[2] = new ML_Texture(m_Device, 640, 360, D3DUSAGE_DYNAMIC);
	 InputTexture[3] = new ML_Texture(m_Device, 640, 360, D3DUSAGE_DYNAMIC);


	 innerTexture[0] = new ML_Texture(m_Device, 640, 360, D3DUSAGE_RENDERTARGET);
	 innerTexture[1] = new ML_Texture(m_Device, 640, 360, D3DUSAGE_RENDERTARGET);
	 OutputTexture = new ML_Texture(m_Device, 1280, 720, D3DUSAGE_RENDERTARGET);
	 StagTexture = new ML_Texture(m_Device, 640, 360, D3DUSAGE_DYNAMIC, D3DPOOL_SYSTEMMEM);
	 lutTexture = new ML_Texture(m_Device, 256, 1, D3DUSAGE_DYNAMIC);


	 IDirect3DVertexShader9* vertexshader = 0;

	 DXCall(m_Device->CreateVertexShader((DWORD*)VertexShaderContent, &vertexshader));
	 DXCall(m_Device->SetVertexShader(vertexshader));
	 return true;
	 return S_OK;
 }

//1 对device开始一个scene
//2 获取 Texture backbuffer 
//3 设置这个backbuffer 作为rendertarget
//4 清除 backbuffer数据
//写入数据到texture
	//texture.lock
	// copy buffer 到texture (memcpy data to d3dlock_rect.pbits) 三个texture
	//设置顶点数组
	//设置pixelshader SetPixelShader
	//将填充好数据的texture设置到direct中, SetTexture(0/1/2, YTex)
	//设置顶点格式 SetFVF

ID3DFilter::ID3DFilter(IDirect3DDevice9Ex *device)
{
	m_Device = device;

}
