#include "mycommon.h"
#include "NetworkServices.h"
#include "../utils/CurlFile.h"


#include <SDL_Image/SDL_image.h>
#include <WXBase.h>

static WXLocker s_renderer;
static WXLocker s_MirrorMutex;
SDL_Renderer *mirror_renderer = NULL;
//
static WXLocker s_ParentwindowMutex;
std::map<uint64_t, SDL_Window*> g_mapIpToWindow;
std::map<uint64_t, SDL_Renderer*> g_mapIpToRender;

extern "C" void WriteLog(const char *arcLog)
{
	WXLogWriteNew(arcLog);
}

extern "C" void WriteErrorLog(const char *arcLog)
{
	WXLogWriteNew(arcLog);
}


extern "C" HWND GetParentHandle()
{
	static int i = 0;
	if (i > 0)
	{
		return 0;
	}
	i++;
	return CNetworkServices::Get().m_iParentHandle;
}

extern "C" SDL_Window * GetParentWindow()
{
	static SDL_Window *mywindow = NULL;
	if (mywindow != NULL)
	{
		return mywindow;
	}
	else
	{
		mywindow = SDL_CreateWindowFrom((void*)GetParentHandle());
		return mywindow;
	}
}


extern "C" SDL_Renderer * GetSDLRender(SDL_Window *window)
{
	WXAutoLock oLock(s_renderer);
	static SDL_Renderer *myrender = NULL;
	if (myrender != NULL)
	{
		SDL_DestroyRenderer(myrender);
		myrender = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
		return myrender;
	}
	else
	{
		myrender = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
		return myrender;
	}
}

extern "C" SDL_Renderer * GetCurSDLRender(SDL_Window *window, const char* arcIp)
{
	WXAutoLock oLock(s_renderer);
	uint64_t uniqueid = 0;
	memcpy(&uniqueid, arcIp, 4);
	SDL_Renderer *myrender = NULL;
	if (g_mapIpToRender.find(uniqueid) != g_mapIpToRender.end())
	{
		SDL_DestroyRenderer(g_mapIpToRender[uniqueid]);
		
	}
	myrender = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	g_mapIpToRender[uniqueid] = myrender;
	return myrender;
}

extern "C" void SetMirrorRender(SDL_Renderer *render)
{
	WXAutoLock oLock(s_MirrorMutex);
	mirror_renderer = render;
}

extern "C" void LockSDLCommon()
{
	WXAutoLock oLock(s_renderer);
	if (g_mutex == NULL)
	{
		g_mutex = SDL_CreateMutex();
	}
	SDL_LockMutex(g_mutex);
}

extern "C" void UnlockSDLCommon()
{
	SDL_UnlockMutex(g_mutex);
}

extern "C" void ReportPlayWH(int eWindowStatus, int iScreenW, int iScreenH, const char* arcIp)
{
	uint64_t uniqueid = 0;
	memcpy(&uniqueid, arcIp, 4);
	if (CNetworkServices::Get().m_stAirplay.m_CallBackWindowStatus != NULL)
	{
		WindowShowStruct stWindowShow;
		stWindowShow.eWindowStatus = (WINDOWSHOWSTATUS)eWindowStatus;
		stWindowShow.iScreenW = iScreenW;
		stWindowShow.iScreenH = iScreenH;
		if (CNetworkServices::Get().m_mapModel[uniqueid].find("iPhone") != -1)
		{
			stWindowShow.iIPad = 0;
		}
		else
		{
			stWindowShow.iIPad = 1;
		}
		CNetworkServices::Get().m_stAirplay.m_CallBackWindowStatus(stWindowShow, uniqueid);
	}
}

extern "C" SDL_Renderer* GetMirrorRender()
{
	WXAutoLock oLock(s_MirrorMutex);
	return mirror_renderer;
}

extern "C" bool GetMirrorPicture(const char *arcPath)
{
	return true;

	//need convert to utf-8 char
	char* arcRealPath = NULL;
	wchar_t* dest = NULL;
	int dwMinSize = 0;
	SDL_Surface* saveSurface = NULL;
	bool bRet = true;
	WXLogWriteNew("GetMirrorPicture begin");
	//get current surface
	SDL_Surface *tmpSur = SDL_GetWindowSurface(GetParentWindow());
	if (tmpSur->w % 2 != 0)
	{
		tmpSur->w++;
	}
	if (tmpSur->h % 2 != 0)
	{
		tmpSur->h++;
	}
	unsigned char * pixels = new (std::nothrow) unsigned char[tmpSur->w * tmpSur->h * tmpSur->format->BytesPerPixel];
	int iRet = SDL_RenderReadPixels(GetMirrorRender(), &tmpSur->clip_rect, tmpSur->format->format, pixels, tmpSur->w * tmpSur->format->BytesPerPixel);
	if (iRet != 0)
	{
		WXLogWriteNew("read pixels fail");
		bRet = false;
		goto endproc;
	}
	saveSurface = SDL_CreateRGBSurfaceFrom(pixels, tmpSur->w, tmpSur->h,
		tmpSur->format->BitsPerPixel,
		tmpSur->w * tmpSur->format->BytesPerPixel,
		tmpSur->format->Rmask,
		tmpSur->format->Gmask,
		tmpSur->format->Bmask,
		tmpSur->format->Amask);
	if (saveSurface == NULL)
	{
		WXLogWriteNew("create saveSurface fail");
		bRet = false;
		goto endproc;
	}


	dwMinSize = MultiByteToWideChar(CP_ACP, 0, arcPath, -1, NULL, 0);
	dest = (wchar_t*)malloc(dwMinSize * sizeof(wchar_t));
	MultiByteToWideChar(CP_ACP, 0, arcPath, -1, dest, dwMinSize);
	//unicode to utf-8
	dwMinSize = WideCharToMultiByte(CP_UTF8, 0, dest, -1, NULL, 0, NULL, FALSE);
	arcRealPath = (char*)malloc(dwMinSize);
	WideCharToMultiByte(CP_UTF8, 0, dest, -1, arcRealPath, dwMinSize, NULL, FALSE);
	if (strcmp(arcRealPath + dwMinSize - 5, ".bmp") == 0)
	{
		if (SDL_SaveBMP(saveSurface, arcRealPath) != 0)
		{
			WXLogWriteNew("SDL_SaveBMP fail");
			bRet = false;
		}
	}
	else
	{
		if (IMG_SavePNG(saveSurface, arcRealPath) != 0)
		{
			WXLogWriteNew("IMG_SavePNG fail");
			bRet = false;
		}
	}

endproc:
	if (arcRealPath != NULL)
	{
		free(arcRealPath);
	}
	if (dest != NULL)
	{
		free(dest);
	}
	if (saveSurface != NULL)
	{
		SDL_FreeSurface(saveSurface);
	}
	delete[] pixels;
	WXLogWriteNew("GetMirrorPicture end");
	return bRet;
}
//
extern "C" SDL_Window * GetCurrentWindow(const char* arcIp, bool bReCreate)
{
	WXAutoLock oLock(s_ParentwindowMutex);
	SDL_Window *curwindow = NULL;
	if (arcIp == NULL)
	{
		return curwindow;
	}
	uint64_t uniqueid = 0;
	memcpy(&uniqueid, arcIp, 4);


	if (g_mapIpToWindow.find(uniqueid) != g_mapIpToWindow.end() && !bReCreate)
	{
		curwindow = g_mapIpToWindow[uniqueid];
	}
	else
	{
		curwindow = SDL_CreateWindowFrom((void*)CNetworkServices::Get().m_stAirplay.m_CallBackGetParentWindow(uniqueid));
		g_mapIpToWindow[uniqueid] = curwindow;
	}
	return curwindow;
}
//
extern "C" void SetWindowFinalize(const char* arcIp)
{
	WXAutoLock oLock(s_ParentwindowMutex);
	uint64_t uniqueid = 0;
	memcpy(&uniqueid, arcIp, 4);
	if (arcIp == NULL)
	{
		return;
	}
	if (g_mapIpToWindow.find(uniqueid) != g_mapIpToWindow.end())
	{
		g_mapIpToWindow.erase(g_mapIpToWindow.find(uniqueid));
	}
}
