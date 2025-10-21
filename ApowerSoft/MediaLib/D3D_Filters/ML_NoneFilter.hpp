#pragma once
#include "ML_D3DFilter.h"
#include "ML_Shader.h"

class NoneFilter :public D3DFilter
{
public:
	NoneFilter(IDirect3DDevice9Ex* device) :D3DFilter(device)
	{

	}
	virtual void Render(std::vector<FilterFrame> frames, int width, int height, unsigned char* output = NULL) override
	{
		HRESULT hr;
		for (size_t i = 0; i < frames.size(); i++)
		{
			InputTexture[i]->UpdateSize(frames[i].width, frames[i].height);
			InputTexture[i]->Upload(frames[i].data, frames[i].width, frames[i].height, frames[i].pitch);
			m_Device->SetTexture(i, InputTexture[i]->GetTexture());
		}

		OutputTexture->UpdateSize(width, height);
		innerTexture[0]->UpdateSize(width, height);

		DXCall(m_Device->SetRenderTarget(0, OutputTexture->GetSurface()));
		DXCall(m_Device->BeginScene());
		DXCall(m_Device->DrawPrimitive(D3DPT_TRIANGLELIST, 0, 2));
		DXCall(m_Device->EndScene());

		RECT rcsrc{ 0,0,width, height };
		DXCall(m_Device->StretchRect(OutputTexture->GetSurface(), &rcsrc, innerTexture[0]->GetSurface(), &rcsrc, D3DTEXF_LINEAR));
	}
};
