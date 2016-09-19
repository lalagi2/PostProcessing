#include "stdafx.h"

#include "TextureHandler.h"

Surface createSurface(GLsizei width, GLsizei height, int numComponents)
{
	GLuint fboHandle;
	glGenFramebuffers(1, &fboHandle);
	glBindFramebuffer(GL_FRAMEBUFFER, fboHandle);

	GLuint textureHandle;
	glGenTextures(1, &textureHandle);
	glBindTexture(GL_TEXTURE_2D, textureHandle);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	const int UseHalfFloats = 1;
	if (UseHalfFloats) 
	{
		switch (numComponents)
		{
		case 1: glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width, height, 0, GL_RED, GL_HALF_FLOAT, 0); break;
		case 2: glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, width, height, 0, GL_RG, GL_HALF_FLOAT, 0); break;
		case 3: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_HALF_FLOAT, 0); break;
		case 4: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_HALF_FLOAT, 0); break;
		default: std::cout << "Illegal slab format.";
		}
	}
	else 
	{
		switch (numComponents) 
		{
		case 1: glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, 0); break;
		case 2: glTexImage2D(GL_TEXTURE_2D, 0, GL_RG32F, width, height, 0, GL_RG, GL_FLOAT, 0); break;
		case 3: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, 0); break;
		case 4: glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, 0); break;
		default: std::cout << "Illegal slab format.";
		}
	}

	if (GL_NO_ERROR != glGetError()) std::cout << "Unable to create normals texture";

	GLuint colorbuffer;
	glGenRenderbuffers(1, &colorbuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, colorbuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureHandle, 0);
	if (GL_NO_ERROR != glGetError()) std::cout << "Unable to attach color buffer";

	if (GL_FRAMEBUFFER_COMPLETE != glCheckFramebufferStatus(GL_FRAMEBUFFER)) std::cout << "Unable to create FBO.";
	Surface surface = { fboHandle, textureHandle, numComponents };

	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return surface;
}

PingPongTexture createPingPongTexture(GLsizei width, GLsizei height, int numComponents)
{
	PingPongTexture pingPong;
	pingPong.Ping = createSurface(width, height, numComponents);
	pingPong.Pong = createSurface(width, height, numComponents);

	return pingPong;
}