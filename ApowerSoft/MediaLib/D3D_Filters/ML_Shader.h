#pragma once
#include <d3d9.h>
#include <string>
#include <unordered_map>
#include "./ML_d3dUtility.h"
#include "./ML_D3DRender.h"
//一个独立的滤镜插件
//
class ML_Shader
{
	
	IDirect3DPixelShader9 *pixelshader = 0;
	std::string pixelshaderpath;
	std::unordered_map<std::string, D3DXHANDLE> m_UniformLocationCache;
	std::string m_FilePath;
	 IDirect3DDevice9Ex *Device;
	ID3DXConstantTable* constanttable = 0;
protected:
	float TextureSize[2];
public:
	ML_Shader(IDirect3DDevice9Ex*device, const std::string &pixelshader);
	ML_Shader(IDirect3DDevice9Ex*device, const uint8_t* pixelshadercontent );
	virtual ~ML_Shader();
	void Bind()const;
	void Unbind()const;
	void SetTexture(const std::string& name, IDirect3DBaseTexture9* slot);
	void SetFloat(const std::string& name, float value);
	void SetFloat4(const std::string& name, WX_D3DXVECTOR4* value, int count);
	void SetFloatArray(const std::string& name, float* value, int count);
	void SetIntArray(const std::string& name, int* value, int count);
	void SetSize(float width, float height) {
		TextureSize[0] = width;
		TextureSize[1] = height;
	}
	void virtual Render();
	HRESULT virtual ApplyParameter() { return 0; };
	HRESULT virtual SetParameter(void * param) { return 0; };
	static ML_Shader* CurrentFilterSegment;
	bool IsInvalid() { return pixelshader == NULL; }
private:
	unsigned int LoadShader(const std::string & path);
	unsigned int LoadShader(const uint8_t* pixelshadercontent);
	D3DXHANDLE GetUniformLocation(const std::string& name);
};
