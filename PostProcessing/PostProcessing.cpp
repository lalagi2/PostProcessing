#include "stdafx.h"
#include <iostream>
#include <GL/glew.h> 
#include <GLFW/glfw3.h> 
#include "FreeImage.h"

void loadImage(const char *filename)
{
	FIBITMAP *dib1 = nullptr;

	FREE_IMAGE_FORMAT fif = FreeImage_GetFIFFromFilename(filename);

	dib1 = FreeImage_Load(fif, filename, HDR_DEFAULT);

	if (!dib1)
	{
		std::cout << "Failed to load this image: " << filename << std::endl;
		exit(0);
	}

	int nWidth = FreeImage_GetWidth(dib1);
	int nHeight = FreeImage_GetHeight(dib1);
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, nWidth, nHeight, 0, GL_RGB, GL_FLOAT, (void*)FreeImage_GetBits(dib1));

	FreeImage_Unload(dib1);

	return;
}

int main() 
{
	// Start GL context and O/S window using the GLFW helper library
	if (!glfwInit()) 
	{
		std::cout << "ERROR: could not start GLFW3";

		return 1;
	}

	GLFWwindow* window = glfwCreateWindow(1366, 768, "Post Processing", NULL, NULL);
	glfwWindowHint(GLFW_DEPTH_BITS, 32);

	if (!window) 
	{
		std::cout << "ERROR: could not open window with GLFW3" << std::endl;
		glfwTerminate();

		return 1;
	}

	glfwMakeContextCurrent(window);

	// Start GLEW extension handler
	glewExperimental = GL_TRUE;
	glewInit();
	 
	// Get version info
	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	printf("Renderer: %s\n", renderer);
	printf("OpenGL version supported %s\n", version);

	glEnable(GL_DEPTH_TEST);

	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	GLuint vbo = 0;
	glGenBuffers(1, &vbo);

	GLfloat vertices[] = 
	{
		//  Position          Color          Texcoords
		-1.0f, 1.0f, 0.5f, 	1.0f, 0.0f, 0.0f,	1.0f, 1.0f, // Top-left
		1.0f, 1.0f, 0.5f,	0.0f, 1.0f, 0.0f,	0.0f, 1.0f, // Top-right
		1.0f, -1.0f, 0.5f,	0.0f, 0.0f, 1.0f,	0.0f, 0.0f, // Bottom-right
		-1.0f, -1.0f, 0.5f,	1.0f, 1.0f, 1.0f,	1.0f, 0.0f  // Bottom-left
	};

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// Create an element array
	GLuint ebo;
	glGenBuffers(1, &ebo);

	GLuint elements[] = 
	{
		0, 1, 2,
		2, 3, 0
	};

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(elements), elements, GL_STATIC_DRAW);

	const char* vertex_shader =
		"#version 450\n" 

		"in vec3 position;"
		"in vec3 color;"
		"in vec2 texcoord;"
		"out vec3 Color;"
		"out vec2 Texcoord;"
		
		"void main()"
		"{"
		"    Color = color;"
		"    Texcoord = texcoord;"
		"    gl_Position = vec4(position, 1.0);"
		"}";

	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertex_shader, NULL);
	glCompileShader(vertexShader);

	GLint success;
	GLchar infoLog[512];
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILAfTION_FAILED\n" << infoLog << std::endl;
	}

	const char* fragment_shader =
		"#version 450\n"
		
		"in vec3 Color;"
		"in vec2 Texcoord;"
		"out vec4 outColor;"
		"uniform sampler2D text;"

		"void main()"
		"{"
		"    outColor = texture(text, Texcoord);"
		"}";

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragment_shader, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, fragmentShader);
	glAttachShader(shaderProgram, vertexShader);
	glLinkProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success) 
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::LINKING\n" << infoLog << std::endl;
	}

	glUseProgram(shaderProgram);

	// Specify the layout of the vertex data
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), 0);

	GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
	glEnableVertexAttribArray(colAttrib);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

	GLint texAttrib = glGetAttribLocation(shaderProgram, "texcoord");
	glEnableVertexAttribArray(texAttrib);
	glVertexAttribPointer(texAttrib, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));

	// Load textures
	GLuint textures[1];
	glGenTextures(1, textures);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, textures[0]);
		loadImage("PaperMill_E_3k.hdr");
	glUniform1i(glGetUniformLocation(shaderProgram, "text"), 0);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	while (!glfwWindowShouldClose(window)) 
	{
		// Clear the screen to black
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Draw a rectangle from the 2 triangles using 6 indices
		glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

		glfwPollEvents();

		// Put the stuff we've been drawing onto the display
		glfwSwapBuffers(window);
	}

	glDeleteTextures(1, textures);

	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);

	glDeleteBuffers(1, &ebo);
	glDeleteBuffers(1, &vbo);

	glDeleteVertexArrays(1, &vao);

	// Close GL context and any other GLFW resources
	glfwTerminate();

	return 0;
}