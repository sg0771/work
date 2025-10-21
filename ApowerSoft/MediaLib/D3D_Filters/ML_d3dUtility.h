//////////////////////////////////////////////////////////////////////////////////////////////////
// 
// File: d3dUtility.h
// 
// Author: Frank Luna (C) All Rights Reserved
//
// System: AMD Athlon 1800+ XP, 512 DDR, Geforce 3, Windows XP, MSVC++ 7.0 
//
// Desc: Provides utility functions for simplifying common tasks.
//          
//////////////////////////////////////////////////////////////////////////////////////////////////

#ifndef __d3dUtilityH__
#define __d3dUtilityH__

#include <d3dx9/d3dx9.h>
#include <string>
#include <d3dx9/DxErr.h>
#include <iostream>
#include <atlbase.h>
#include <Utils.hpp>
#include <wxlog.h>

#define DXCall(x)  hr=x; if(FAILED(hr)){WXLogA("%08x",hr);};

namespace d3d
{



	HRESULT InitD3D(int width, int height,IDirect3DDevice9Ex** device);


	HRESULT Reset(IDirect3DDevice9Ex*device, int width, int height);


	
}




#endif // __d3dUtilityH__