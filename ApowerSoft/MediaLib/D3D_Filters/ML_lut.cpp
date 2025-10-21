#pragma once
#include "ML_lut.h"
#include <math.h>

#include "stb_image.h"

lutFilterSegment::lutFilterSegment(IDirect3DDevice9Ex* device) :ML_Shader(device, lutShader)
, m_pleader(NULL)
{
}


lutFilterSegment::lutFilterSegment(IDirect3DDevice9Ex* device, lutFilter* pLeader) : ML_Shader(device, lutShader)
, m_pleader(pLeader)
{
}


HRESULT lutFilterSegment::ApplyParameter()
{
	return S_OK;
}

HRESULT lutFilterSegment::SetParameter(void* param)
{
	context = *(LutContext*)param;
	return S_OK;
}

lutFilter::lutFilter(IDirect3DDevice9Ex* device) :D3DFilter(device)
{
	filters.push_back(new lutFilterSegment(device, this));
}
bool lutFilter::setLutData(const char* path)
{
	try
	{
		if (!WXBase::Exists(path))
		{
			std::cout << "path路径错误，不打开lut滤镜" << std::endl;
			return true;
		}
	}
	catch (...)
	{
		std::cout << "path路径异常，不打开lut滤镜" << std::endl;
		return false;
	}

	int width = 0, height = 0, channels = 0;
	stbi_set_flip_vertically_on_load(false);
	unsigned char* m_LocalBuffer1 = stbi_load(path, &width, &height, &channels, 4);
	if (NULL != m_LocalBuffer1)
	{
		// 调换R和B的位置
		for (int i = 0; i < width * height * 4; i += 4) {
			auto temp = m_LocalBuffer1[i + 2];
			m_LocalBuffer1[i + 2] = m_LocalBuffer1[i];
			m_LocalBuffer1[i] = temp;
		}

		m_stLutData.width = width;
		m_stLutData.height = sqrt(height);
		m_stLutData.depth = sqrt(height);// 一般lut文件尺寸为a*a*a，因宽可能被拉伸，所以只有depth与height相等
		m_stLutData.m_pLutData.reset(m_LocalBuffer1);
		return true;
	}
	else
	{
		return false;
	}
}
void lutFilter::Render(std::vector<FilterFrame> frames, int width, int height, unsigned char* output)
{
	if (nullptr == m_stLutData.m_pLutData)
	{
		std::cout << "m_pLutData empty!!!" << std::endl;
		return;
	}

	frames.push_back({ m_stLutData.m_pLutData.get(),  m_stLutData.width, m_stLutData.height
	,"inputtextureb", m_stLutData.width*4, false, FRAME_3D,  m_stLutData.depth });
	
	D3DFilter::Render(frames, width, height, output);
}
