#include "stdafx.h"
#include "Obstacle.h"

void createObstacles(Surface dest, int width, int height)
{
	glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
	glViewport(0, 0, width, height);
	glClearColor(0, 0, 0, 0);
	glClear(GL_COLOR_BUFFER_BIT);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);
	Shader program("defaultVS.vs", "fill.fs");

	program.Use();

	const int DrawBorder = 1;
	if (DrawBorder) 
	{
#define T 0.9999f
		float positions[] = { -T, -T, T, -T, T,  T, -T,  T, -T, -T };
#undef T
		GLuint vbo;
		GLsizeiptr size = sizeof(positions);
		glGenBuffers(1, &vbo);
		glBindBuffer(GL_ARRAY_BUFFER, vbo);
		glBufferData(GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW);
		GLsizeiptr stride = 2 * sizeof(positions[0]);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, 0);
		glDrawArrays(GL_LINE_STRIP, 0, 5);
		glDeleteBuffers(1, &vbo);
	}

	// Cleanup
	glDeleteProgram(program.Program);
	glDeleteVertexArrays(1, &vao);
}