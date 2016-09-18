#pragma once

#include "stdafx.h"
#include <iostream>
#include <cmath>

// GLEW
#include <GL/glew.h>

// GLFW
#include <GLFW/glfw3.h>

// GLM Mathematics
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Other includes
#include "Shader.h"
#include "Camera.h"
#include "framebuffer.hpp"


typedef struct Surface_ {
	GLuint FboHandle;
	GLuint TextureHandle;
	int NumComponents;
} Surface;

typedef struct PingPongTexture_ {
	Surface Ping;
	Surface Pong;
} PingPongTexture;

typedef struct Vector2_ {
	int X;
	int Y;
} Vector2;
