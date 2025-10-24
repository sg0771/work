#include "ML_Texture.h"

ML_Texture::ML_Texture(IDirect3DDevice9Ex* Device, int _width, int _height, int Usages , D3DPOOL d3dpool )
	:m_Device(Device), m_usages(Usages), m_d3dpool(d3dpool), m_textureType(TEXTURE_2D)
{
	UpdateSize(_width, _height);
}

void ML_Texture::UpdateSize(int _width, int _height, int _depth)
{
	if (_width == m_width&& _height == m_height)
	{
		return;
	}

	m_width = _width;
	m_height = _height;

	if (m_textureType != TEXTURE_3D)
	{
		if (texture) texture = nullptr; // need new texture
		surface = nullptr;

		if (m_height == 1)
		{
			//D3DFMT_X8R8G8B8
			LibInst::GetInst().mD3DXCreateTexture(m_Device, m_width, m_height, 1, m_usages, D3DFMT_R32F, m_d3dpool, &texture);
		}
		else
		{
			//D3DFMT_A8R8G8B8
			//(D3DFORMAT)MAKEFOURCC('B', 'G', 'R', 'A')
			LibInst::GetInst().mD3DXCreateTexture(m_Device, m_width, m_height, 1, m_usages, (D3DFORMAT)MAKEFOURCC('B', 'G', 'R', 'A'), m_d3dpool, &texture);
		}
		//if (m_usages == D3DUSAGE_RENDERTARGET)
		//{
		texture->GetSurfaceLevel(0, &surface);
		//}
	}
	else
	{
		LibInst::GetInst().mD3DXCreateVolumeTexture(m_Device, m_width, m_height, _depth, 0, m_usages, (D3DFORMAT)MAKEFOURCC('B', 'G', 'R', 'A'), m_d3dpool, &texture_3D);
		// set wrap
		m_Device->SetSamplerState(1, D3DSAMP_ADDRESSU, D3DTADDRESS_MIRROR);
		m_Device->SetSamplerState(1, D3DSAMP_ADDRESSV, D3DTADDRESS_MIRROR);
		m_Device->SetSamplerState(1, D3DSAMP_ADDRESSW, D3DTADDRESS_MIRROR);
		// 设置放大缩小的插值方式为线性插值
		m_Device->SetSamplerState(1, D3DSAMP_MINFILTER, D3DTEXF_LINEAR);
		m_Device->SetSamplerState(1, D3DSAMP_MAGFILTER, D3DTEXF_LINEAR);

	}
}

void ML_Texture::setTextureType(e_textureType e_type)
{
	m_textureType = e_type;
}

ML_Texture::~ML_Texture()
{
	surface = NULL;
	texture = NULL;
	texture_3D = NULL;
}

HRESULT ML_Texture::Upload(unsigned char* buf, int width, int height, int pitch, int depth)
{
	if (m_textureType != TEXTURE_3D)
	{
		D3DLOCKED_RECT inputtex_rect;
		if (S_OK == texture->LockRect(0, &inputtex_rect, 0, 0))
		{

			int texturepitch = inputtex_rect.Pitch;
			BYTE* targetfubber = (BYTE*)inputtex_rect.pBits;

			for (size_t y = 0; y < height; y++)
			{

				int offset = pitch > 0 ? y * abs(pitch) :   (height - y - 1)* abs(pitch);
					memcpy(targetfubber + y * texturepitch, buf + offset, abs(pitch));
				
			}
			texture->UnlockRect(0);
			return  S_OK;
		}
		return S_FALSE;
	}
	else
	{
		D3DLOCKED_BOX locked_box;
		auto hr = texture_3D->LockBox(0, &locked_box, NULL, 0);

		if (FAILED(hr)) {
			//std::cout << "Error: " << DXGetErrorString(hr) << " error description: " << DXGetErrorDescription(hr) << std::endl;
			return S_FALSE;
		}

		memcpy((BYTE*)locked_box.pBits, buf, height * pitch * depth);
		texture_3D->UnlockBox(0);
		return  S_OK;
	}
}


HRESULT ML_Texture::Download(unsigned char* buf)
{
	int pitch = m_width * 4;
	D3DLOCKED_RECT inputtex_rect;
	HRESULT hr = S_OK;
	if (S_OK == (hr = texture->LockRect(0, &inputtex_rect, 0, 0)))
	{
		int texturepitch = inputtex_rect.Pitch;

		BYTE* targetfubber = buf;
		BYTE* sourcebuffer= (BYTE*)inputtex_rect.pBits;

		for (size_t y = 0; y < m_height; y++)
		{
			memcpy(targetfubber + y * texturepitch, sourcebuffer + y * pitch, m_width * 4);
		}
		texture->UnlockRect(0);
		return  S_OK;
	}
	return S_FALSE;
}

