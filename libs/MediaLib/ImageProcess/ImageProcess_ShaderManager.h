#pragma once
#if !defined(_WIN32) && (defined(__unix__) || defined(__unix) || (defined(__APPLE__) && defined(__MACH__)))
/* UNIX-style OS. ------------------------------------------- */
#else
#include <Windows.h>
#endif


#include <iostream>
#include <iterator> 
#include <sstream>

class ShaderManager
{
public:
	static ShaderManager& GetInst();
	ShaderManager();
	~ShaderManager();
	int InitFrameBuffer(int width, int height);
};

