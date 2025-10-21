#include <inttypes.h>
#include <stdio.h>
#include <windows.h>
#include "get-graphics-offsets.h"

int get_offsets()
{
	WNDCLASSA wc = { 0 };
	wc.style = CS_OWNDC;
	wc.hInstance = GetModuleHandleA(NULL);
	wc.lpfnWndProc = (WNDPROC)DefWindowProcA;
	wc.lpszClassName = DUMMY_WNDCLASS;

	SetErrorMode(SEM_FAILCRITICALERRORS);

	if (!RegisterClassA(&wc)) {
		printf("failed to register '%s'\n", DUMMY_WNDCLASS);
		return -1;
	}
	int size = sizeof(struct graphics_offsets);
	struct graphics_offsets   offsets;
	memset(&offsets, 0, size);

	get_d3d9_offsets(&offsets.d3d9);
	get_d3d8_offsets(&offsets.d3d8);
	get_dxgi_offsets(&offsets.dxgi);

	HANDLE hShareMem = OpenFileMapping(FILE_MAP_READ | FILE_MAP_WRITE, false, SHARED_OFFSETS);
	if ((hShareMem != INVALID_HANDLE_VALUE) && (hShareMem != NULL)) {
		uint8_t* pShareMem = (uint8_t *)MapViewOfFile(hShareMem, FILE_MAP_ALL_ACCESS, 0, 0, MEMSIZE);
		if (pShareMem) {

			if (sizeof(void*) == 4) {//32bits
				memcpy(pShareMem, &offsets, size);
			}
			else { //64 bits
				memcpy(pShareMem + size, &offsets, size);
			}
			UnmapViewOfFile(pShareMem);
		}
		CloseHandle(hShareMem);		// ¹Ø±Õ¾ä±ú
	}
	return 0;
}
