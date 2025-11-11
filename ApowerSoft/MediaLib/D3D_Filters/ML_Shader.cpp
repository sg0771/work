#include "ML_Shader.h"
#include <fstream>
#include <iostream>
ML_Shader* ML_Shader::CurrentFilterSegment;

ML_Shader::ML_Shader(IDirect3DDevice9Ex*device, const std::string &_pixelshaderpath):Device(device),pixelshaderpath(_pixelshaderpath)
{
	HRESULT hr;
	DXCall(LoadShader(pixelshaderpath));
}


ML_Shader::ML_Shader(IDirect3DDevice9Ex*device, const uint8_t* pixelshadercontent):Device(device)
{
	HRESULT hr;
	DXCall(LoadShader(pixelshadercontent));
}

ML_Shader::~ML_Shader()
{
	
}

void ML_Shader::Bind() const
{

	HRESULT hr;
	DXCall(Device->SetPixelShader(pixelshader));
	DXCall(constanttable->SetDefaults(Device));
	
}

void ML_Shader::Unbind() const
{

}

void ML_Shader::SetTexture(const std::string& name, IDirect3DBaseTexture9* slot)
{
	HRESULT hr;
	D3DXCONSTANT_DESC Tex0Desc;
	auto inputTexHandle = GetUniformLocation(name);
	unsigned int count = 0;
	if (inputTexHandle!=0)
	{
		DXCall(constanttable->GetConstantDesc(inputTexHandle, &Tex0Desc, &count));
		DXCall(Device->SetTexture(Tex0Desc.RegisterIndex, slot));
	}
	
}

void ML_Shader::SetFloat(const std::string& name, float value)
{
	HRESULT hr;
	DXCall(constanttable->SetFloat(Device, GetUniformLocation(name), value));
}

void ML_Shader::SetIntArray(const std::string& name, int* value, int count)
{
	HRESULT hr;
	DXCall(constanttable->SetIntArray(Device, GetUniformLocation(name), value, count));
}

void ML_Shader::SetFloat4(const std::string& name, WX_D3DXVECTOR4* value, int count)
{
	HRESULT hr;
	DXCall(constanttable->SetVectorArray(Device, GetUniformLocation(name), (D3DXVECTOR4*)value, count));
}


void ML_Shader::Render()
{
	ApplyParameter();
	HRESULT hr;
	//DXCall(Device->Clear(0, 0, D3DCLEAR_TARGET, 0, 1.0f, 0));
	DXCall(Device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2));
}

void ML_Shader::SetFloatArray(const std::string& name, float* value, int count)
{
	HRESULT hr;
	DXCall(constanttable->SetFloatArray(Device, GetUniformLocation(name), value, count));
}

unsigned int ML_Shader::LoadShader(const std::string & path)
{
	HRESULT hr = S_OK;
	std::ifstream pixelstream(path);
	BYTE szbuf[8192*10] = { 0 };
	pixelstream.read((char*)szbuf, sizeof(char) * 8192 * 10);
	pixelstream.close();

	return  LoadShader(szbuf);
}

unsigned int ML_Shader::LoadShader(const uint8_t* pixelshadercontent)
{
	HRESULT hr = S_OK;
	DXCall(Device->CreatePixelShader((DWORD*)pixelshadercontent, &pixelshader));
	DXCall(LibInst::GetInst().mD3DXGetShaderConstantTable((DWORD*)pixelshadercontent, &constanttable));
	return hr;
}

D3DXHANDLE ML_Shader::GetUniformLocation(const std::string& name)
{
	HRESULT hr;
	if (m_UniformLocationCache.find(name) != m_UniformLocationCache.end())
		return m_UniformLocationCache[name];
	D3DXHANDLE location = constanttable->GetConstantByName(0, name.c_str());
	m_UniformLocationCache[name] = location;
	return location;
}
