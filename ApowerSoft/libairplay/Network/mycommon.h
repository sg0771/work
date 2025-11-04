#pragma once
#include <stdint.h>
#include "SDL2/SDL.h"
#include "AirPlayServer.h"

static SDL_mutex *g_mutex = NULL;

extern "C" int ReadNew(uint8_t* buf, int size);


extern "C" int SeekNew(int64_t offset, int whence);

extern "C" int MySeekNew(int64_t offset, int whence);


extern "C" int64_t GetLengthNew();

extern "C" int DownLoad(const char* url, const char *outfilename);

extern "C" int interrupt_cb_new();

extern "C" HWND GetParentHandle();

extern "C" SDL_Window * GetParentWindow();

extern "C" SDL_Renderer * GetSDLRender(SDL_Window *window);

extern "C" void SetMirrorRender(SDL_Renderer *render);

extern "C" void LockSDLCommon();

extern "C" void UnlockSDLCommon();

extern "C" void ReportPlayWH(int eWindowStatus, int iScreenW, int iScreenH, const char* arcIp);

extern "C" SDL_Renderer* GetMirrorRender();

extern "C" bool GetMirrorPicture(const char *arcPath);

extern "C" SDL_Window * GetCurrentWindow(const char* arcIp, bool bReCreate = false);

extern "C" void SetWindowFinalize(const char* arcIp);

extern "C" SDL_Renderer * GetCurSDLRender(SDL_Window *window, const char* arcIp);