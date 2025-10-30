#pragma once
#include "ML_Shader.h"

#include <vector>
#include "./ML_Texture.h"
#include "Utils.hpp"

#include <atlcomcli.h>


class ID3DFilter {
public:
	ID3DFilter(IDirect3DDevice9Ex *device);
	virtual void Render(std::vector< FilterFrame> frames, int width, int height, unsigned char* output = NULL) =0;
	virtual HRESULT  SetParameter(void * param) { return 0; }
	inline static ML_Texture*GetOutputTexture() {
		return  OutputTexture;
	}
	virtual bool IsInvalid() { return false; };
	virtual HRESULT Release();
	static  HRESULT Init(IDirect3DDevice9Ex* device);
	static  HRESULT UnInit(IDirect3DDevice9Ex* device);
protected:
	static int innerTextureIndex;
	static ML_Texture* innerTexture[2];
	static ML_Texture* InputTexture[8];
	static ML_Texture* lutTexture;
	static ML_Texture* OutputTexture;
	static ML_Texture* StagTexture;
	static ML_Shader* CurrentFilterSegment;
	static IDirect3DVertexBuffer9* Quad;
	static IDirect3DDevice9Ex* m_Device;
};

class IMulD3DFilter : public ID3DFilter
{
public:
	IMulD3DFilter(IDirect3DDevice9Ex *device):ID3DFilter(device) {}

	virtual void Render(std::vector< FilterFrame> frames, int width, int height, unsigned char* output = NULL) {
		childFilters[0]->Render(frames, width, height, output);
		for (size_t i = 1; i < childFilters.size(); i++)
		{
			childFilters[i]->Render({}, width, height, output);
		}
	};
	std::vector<ID3DFilter*> childFilters;
	virtual void ApplyParameter() {	}
	virtual HRESULT  SetParameter(void * param) ;
	void AddFilter(ID3DFilter* filter) {
		childFilters.push_back(filter);
	}
};

 class D3DFilter:public ID3DFilter
{
public:
	//D3DFilter(IDirect3DDevice9Ex*device, std::vector<std::string> &shaders);
	D3DFilter(IDirect3DDevice9Ex*device, ML_Shader* shader) :D3DFilter(device) {
		AddShader(shader);
	}
	D3DFilter(IDirect3DDevice9Ex*device) :ID3DFilter(device) {}
	~D3DFilter();
	virtual void Render(std::vector<FilterFrame> frames, int width, int height, unsigned char* output= NULL) override;
	bool IsInvalid() override
	{
		return filters[0]->IsInvalid();
	}
protected:	
	std::vector<ML_Shader*> filters;
	int baseslot;
	
public:
	void AddShader(ML_Shader* shader) {
		filters.push_back(shader);
	}
	HRESULT virtual SetParameter(void * param) override
	{
		for (size_t i = 0; i < filters.size(); i++)
		{
			filters[i]->SetParameter(param);
		}

		return 0;
	}
};
