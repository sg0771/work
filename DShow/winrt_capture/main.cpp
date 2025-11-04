
#include "winrt_capture.h"

#include <Windows.h>
#include <stdio.h>

#include <thread>

static WgcCapture* g_capture = nullptr;

void ThreadMain() {
	int nFps = 100;
	DWORD tid = GetCurrentThreadId();
	g_capture = wgc_capture_create(nFps);
	int width = wgc_capture_width(g_capture);
	int height = wgc_capture_height(g_capture);
	printf("[%d]WinRT capture %dx%d\r\n", (int)tid, width, height);
	wgc_capture_start(g_capture);//启动采集线程
}

int main()
{
	timeEndPeriod(1);
	std::thread new_th(&ThreadMain);
	new_th.detach();

	//ThreadMain();
	system("pause");

	wgc_capture_stop(g_capture);//启动采集线程

	system("pause");
	printf("WinRT capture  Stop\r\n");
  	system("pause");
	return 0;
}
