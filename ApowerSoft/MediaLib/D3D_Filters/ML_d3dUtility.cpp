//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: d3dUtility.cpp
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Provides utility functions for simplifying common tasks.
//          
//////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
#include "../ML_stdafx.h"
#include "ML_d3dUtility.h"

#include <wxlog.h>

class WXDXFilter {
public:
	bool  m_bOpen = false;

	int   m_iBackWidth = 0;
	int   m_iBackHeight = 0;

	WXLocker m_mutex;
	HWND  m_hWnd = nullptr; //Display Windows
	D3DPRESENT_PARAMETERS m_d3dpp;
	CComPtr<IDirect3D9Ex>		m_pD3D = nullptr;//D3D Object
	CComPtr<IDirect3DDevice9Ex>	m_pDev = nullptr;//D3D Device
	int mAdapter = 0;
	D3DDEVTYPE mDeviceType = D3DDEVTYPE_HAL;
	D3DDEVTYPE m_arrDeviceType[4] = {
		D3DDEVTYPE_HAL ,
		D3DDEVTYPE_REF,
		D3DDEVTYPE_SW,
		D3DDEVTYPE_NULLREF,
	};

	HRESULT  CreateD3D(int index, D3DDEVTYPE nDeviceType) {
		HRESULT hr = S_OK;
		D3DCAPS9 caps;
		hr |= m_pD3D->GetDeviceCaps(index, nDeviceType, &caps);
		if (FAILED(hr)) {
			WXLogW(L"[%ws]m_pD3D->GetDeviceCaps error[index=%d][type=%d][%08x]", __FUNCTIONW__, index, nDeviceType, hr);
			return hr;
		}

		int vp = D3DCREATE_MULTITHREADED | D3DCREATE_FPU_PRESERVE;
		if (caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT)
			vp |= D3DCREATE_HARDWARE_VERTEXPROCESSING;
		else
			vp = D3DCREATE_SOFTWARE_VERTEXPROCESSING;

		ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
		m_d3dpp.Windowed = TRUE;
		m_d3dpp.hDeviceWindow = m_hWnd;
		m_d3dpp.SwapEffect = D3DSWAPEFFECT_COPY;
		m_d3dpp.Flags = D3DPRESENTFLAG_VIDEO;
		m_d3dpp.BackBufferCount = 1;
		m_d3dpp.BackBufferWidth = m_iBackWidth;
		m_d3dpp.BackBufferHeight = m_iBackHeight;
		m_d3dpp.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;

		m_pDev = nullptr;
		hr = m_pD3D->CreateDeviceEx(index, nDeviceType, m_hWnd, vp, &m_d3dpp, nullptr, &m_pDev);
		if (FAILED(hr)) {
			WXLogW(L"[%ws]m_pD3D->CreateDeviceEx error[index=%d][type=%d][%08x]",__FUNCTIONW__, index, nDeviceType, hr);
			return hr;
		}
		return hr;
	}
public:
	//创建播放对象
	bool  Open(HWND hwnd, int width, int height) {
		WXAutoLock al(m_mutex);
		if (LibInst::GetInst().m_libD3D9 == nullptr || LibInst::GetInst().m_libD3DX9 == nullptr)
			return false;
		m_pD3D = nullptr;
		LibInst::GetInst().mDirect3DCreate9Ex(D3D_SDK_VERSION, &m_pD3D);
		if (!m_pD3D) {
			WXLogW(L"Direct3DCreate9Ex error");
			return FALSE;
		}
		m_hWnd = hwnd ? hwnd : GetDesktopWindow();
		m_iBackWidth = width;
		m_iBackHeight = height;
		int nCount = m_pD3D->GetAdapterCount(); //有时候多显卡会返回1

		for (int nAdapter = 0; nAdapter < nCount; nAdapter++) { //多显卡处理
			for (int nDeviceIndex = 0; nDeviceIndex < 4; nDeviceIndex++)
			{
				if (SUCCEEDED(CreateD3D(nAdapter, m_arrDeviceType[nDeviceIndex]))) {//d3dx9 
					WXLogW(L"%ws Adapter=%d DeviceType=%d", 
						__FUNCTIONW__, nAdapter, m_arrDeviceType[nDeviceIndex]);
					mAdapter = nAdapter;
					mDeviceType = m_arrDeviceType[nDeviceIndex];
					m_bOpen = true;
					break;
				}
			}
		}
		if (!m_bOpen) {
			WXLogW(L"%ws Error",__FUNCTIONW__);
		}
		return m_bOpen;
	}

	//关闭
	void  Close() {
		WXAutoLock al(m_mutex);
		if (m_bOpen) {
			m_bOpen = false;
		}
	}
};
static WXDXFilter s_filter;

HRESULT d3d::Reset(IDirect3DDevice9Ex*device, int width, int height)
{
	s_filter.m_d3dpp.BackBufferWidth = width;
	s_filter.m_d3dpp.BackBufferHeight = height;
	return s_filter.m_pDev->Reset(&s_filter.m_d3dpp);
}

HRESULT d3d::InitD3D(int width, int height, IDirect3DDevice9Ex** device)
{
	bool bOpen = s_filter.Open(0, width, height);
	if (bOpen) {
		*device = s_filter.m_pDev;
		return S_OK;
	}
	return E_FAIL;
}



