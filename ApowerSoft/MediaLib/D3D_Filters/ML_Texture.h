#pragma once
#include <d3d9.h>
#include <string>
#include <unordered_map>
#include <atlcomcli.h>
#include "ML_d3dUtility.h"

class  ML_Texture
{
public:
	enum e_textureType
	{
		TEXTURE_2D,
		TEXTURE_3D
	};

public:
	ML_Texture(IDirect3DDevice9Ex* Device, int width, int height, int usages, D3DPOOL d3dpool=D3DPOOL_DEFAULT);
	virtual ~ML_Texture();
	HRESULT Upload(unsigned char* buf, int width, int height, int pitch, int depth = 1);
	HRESULT Download(unsigned char* buf);
	inline int GetWidth()const { return m_width; }
	inline int GetHeight()const { return m_height; }
	inline IDirect3DBaseTexture9* GetTexture()const
	{
		if (m_textureType != TEXTURE_3D)
		{
			return texture;
		}
		else
		{
			return texture_3D;
		}
	}
	inline IDirect3DSurface9* GetSurface()const { return surface; }
	void UpdateSize(int _width, int _height, int _depth = 1);
private:
	CComPtr<IDirect3DTexture9>  texture;
	CComPtr<IDirect3DSurface9>  surface = nullptr;
	CComPtr<IDirect3DVolumeTexture9 >  texture_3D;
	std::string m_FilePath;
	unsigned char* m_LocalBuffer;
	int m_width, m_height, m_BPP;
	int m_usages;
	D3DPOOL m_d3dpool;
	IDirect3DDevice9Ex* m_Device;
	// 纹理属性相关操作
public:
	// 设置纹理类型
	void setTextureType(e_textureType e_type);
private:
	e_textureType m_textureType;
};

