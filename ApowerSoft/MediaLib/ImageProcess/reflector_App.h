#pragma once

#include<iostream>
#include "../glew/glew.h"
#include "../glfw/glfw3.h"
#include "reflector_Core.h"

class OPENGL_API App
{
public:
	 App();
	~ App();
	GLFWwindow* window;

};
