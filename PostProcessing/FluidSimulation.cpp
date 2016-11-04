#include "stdafx.h"
#include <chrono>
#include <thread>

#include "FluidSimulation.h"
#include "TextureHandler.h"
#include "Obstacle.h"

#define CellSize (1.25f)
#define ViewportWidth (800)
#define ViewportHeight (600)
#define GridWidth (ViewportWidth / 2)
#define GridHeight (ViewportHeight / 2)

// Function prototypes
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void do_movement();

// Window dimensions
const GLuint WIDTH = 800, HEIGHT = 600;

// Camera
Camera  camera(glm::vec3(0.0f, 0.0f, 3.0f));
GLfloat lastX = WIDTH / 2.0;
GLfloat lastY = HEIGHT / 2.0;
bool    keys[1024];

// Deltatime
GLfloat deltaTime = 0.0f;	// Time between current frame and last frame
GLfloat lastFrame = 0.0f;  	// Time of last frame

static GLuint QuadVao;
static PingPongTexture velocity, density, pressure;
static Surface divergence, obstacle, gravity;

static void ResetState()
{
	glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, 0);
	glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, 0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glDisable(GL_BLEND);
}

GLuint CreateQuad()
{
	short positions[] = {
		-1, -1,
		1, -1,
		-1,  1,
		1,  1,
	};

	// Create the VAO:
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Create the VBO:
	GLuint vbo;
	GLsizeiptr size = sizeof(positions);
	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, size, positions, GL_STATIC_DRAW);

	// Set up the vertex layout:
	GLsizeiptr stride = 2 * sizeof(positions[0]);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_SHORT, GL_FALSE, stride, 0);

	return vao;
}

void createGravityField()
{
	Shader makeGravity("defaultVS.vs", "gravityField.fs");
	makeGravity.Use();

	glBindFramebuffer(GL_FRAMEBUFFER, gravity.FboHandle);

	GLint gravityLoc = glGetUniformLocation(makeGravity.Program, "gravity");
	glUniform2f(gravityLoc, 0.0f, 1.0f);  // X = 0.0, Y = 1.0
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	ResetState();
}

void initDensity()
{
	Shader makeDensity("defaultVS.vs", "densityField.fs");
	makeDensity.Use();

	glBindFramebuffer(GL_FRAMEBUFFER, density.Ping.FboHandle);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindFramebuffer(GL_FRAMEBUFFER, density.Pong.FboHandle);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	ResetState();
}

void SwapSurfaces(PingPongTexture* slab)
{
	Surface temp = slab->Ping;
	slab->Ping = slab->Pong;
	slab->Pong = temp;
}

void ClearSurface(Surface s, float v)
{
	glBindFramebuffer(GL_FRAMEBUFFER, s.FboHandle);
	glClearColor(v, v, v, v);
	glClear(GL_COLOR_BUFFER_BIT);
}

void Advect(Surface velocity, Surface source, Surface obstacles, Surface dest, float dissipation)
{
	Shader advect("defaultVS.vs", "advect.fs");

	GLuint p = advect.Program;
	glUseProgram(p);
	float TimeStep = 0.125f;

	GLint inverseSize = glGetUniformLocation(p, "InverseSize");
	GLint timeStep = glGetUniformLocation(p, "TimeStep");
	GLint dissLoc = glGetUniformLocation(p, "Dissipation");
	GLint sourceTexture = glGetUniformLocation(p, "SourceTexture");
	GLint obstaclesTexture = glGetUniformLocation(p, "Obstacles");

	glUniform2f(inverseSize, 1.0f / GridWidth * 0.5f, 1.0f / GridHeight * 0.5f);
	glUniform1f(timeStep, TimeStep);
	glUniform1f(dissLoc, dissipation);
	glUniform1i(sourceTexture, 1);
	glUniform1i(obstaclesTexture, 2);

	glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velocity.TextureHandle);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, source.TextureHandle);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, obstacles.TextureHandle);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	ResetState();
}

void ComputeDivergence(Surface velocity, Surface obstacles, Surface dest)
{
	Shader computeDivergence("defaultVS.vs", "computeDivergence.fs");

	GLuint p = computeDivergence.Program;
	glUseProgram(p);

	GLint halfCell = glGetUniformLocation(p, "HalfInverseCellSize");
	glUniform1f(halfCell, 0.5f / CellSize);
	GLint sampler = glGetUniformLocation(p, "Obstacles");
	glUniform1i(sampler, 1);

	glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velocity.TextureHandle);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, obstacles.TextureHandle);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	ResetState();
}

void Jacobi(Surface pressure, Surface divergence, Surface obstacles, Surface dest)
{
	Shader jacobi("defaultVS.vs", "jacobi.fs");

	GLuint p = jacobi.Program;
	glUseProgram(p);

	GLint alpha = glGetUniformLocation(p, "Alpha");
	GLint inverseBeta = glGetUniformLocation(p, "InverseBeta");
	GLint dSampler = glGetUniformLocation(p, "Divergence");
	GLint oSampler = glGetUniformLocation(p, "Obstacles");

	glUniform1f(alpha, -CellSize * CellSize);
	glUniform1f(inverseBeta, 0.25f);
	glUniform1i(dSampler, 1);
	glUniform1i(oSampler, 2);

	glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, pressure.TextureHandle);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, divergence.TextureHandle);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, obstacles.TextureHandle);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	ResetState();
}

void SubtractGradient(Surface velocity, Surface pressure, Surface obstacles, Surface dest)
{
	float GradientScale = 1.125f / CellSize;

	Shader subtractGradient("defaultVS.vs", "subtractGradient.fs");

	GLuint p = subtractGradient.Program;
	glUseProgram(p);

	GLint gradientScale = glGetUniformLocation(p, "GradientScale");
	glUniform1f(gradientScale, GradientScale);
	GLint halfCell = glGetUniformLocation(p, "HalfInverseCellSize");
	glUniform1f(halfCell, 0.5f / CellSize);
	GLint sampler = glGetUniformLocation(p, "Pressure");
	glUniform1i(sampler, 1);
	sampler = glGetUniformLocation(p, "Obstacles");
	glUniform1i(sampler, 2);

	glBindFramebuffer(GL_FRAMEBUFFER, dest.FboHandle);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velocity.TextureHandle);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, pressure.TextureHandle);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, obstacles.TextureHandle);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	ResetState();
}

void AddForce(Surface velocity)
{
	Shader makeGravity("defaultVS.vs", "addGravity.fs");
	makeGravity.Use();

	GLint inverseSize = glGetUniformLocation(makeGravity.Program, "InverseSize");
	glUniform2f(inverseSize, 1.0f / GridWidth * 0.5, 1.0f / GridHeight * 0.5);

	glBindFramebuffer(GL_FRAMEBUFFER, velocity.FboHandle);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velocity.TextureHandle);

	GLint gravityLoc = glGetUniformLocation(makeGravity.Program, "gravity");
	glUniform2f(gravityLoc, 0.0f, 1.0f);  // X = 0.0, Y = 1.0
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	ResetState();
}

void initialize()
{
	QuadVao = CreateQuad();
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	velocity = createPingPongTexture(WIDTH, HEIGHT, 2);
	density = createPingPongTexture(WIDTH, HEIGHT, 1);
	pressure = createPingPongTexture(WIDTH, HEIGHT, 1);

	divergence = createSurface(WIDTH, HEIGHT, 3);
	obstacle = createSurface(WIDTH, HEIGHT, 3);
	gravity = createSurface(WIDTH, HEIGHT, 2);

	createGravityField();
	initDensity();

	createObstacles(obstacle, WIDTH, HEIGHT);
	ResetState();
}

void update(unsigned int elapsedMs)
{
	float velocityDissipation = 0.99f;
	float densityDissipation = 0.99f;
	float impulseDensity = 1.0f;

	Advect(velocity.Ping, velocity.Ping, obstacle, velocity.Pong, velocityDissipation);
	SwapSurfaces(&velocity);

	/*Advect(velocity.Ping, density.Ping, obstacle, density.Pong, densityDissipation);
	SwapSurfaces(&density);*/

	AddForce(velocity.Ping);
	SwapSurfaces(&velocity);

	/*ComputeDivergence(velocity.Ping, obstacle, divergence);
	ClearSurface(pressure.Ping, 0);

	int numJacobiIterations = 40;
	for (int i = 0; i < numJacobiIterations; i++)
	{
		Jacobi(pressure.Ping, divergence, obstacle, pressure.Pong);
		SwapSurfaces(&pressure);
	}

	SubtractGradient(velocity.Ping, pressure.Ping, obstacle, velocity.Pong);*/
	//SwapSurfaces(&velocity);
}

void render(Shader& visualizeProgram)
{
	visualizeProgram.Use();

	GLint fillColor = glGetUniformLocation(visualizeProgram.Program, "FillColor");
	GLint scale = glGetUniformLocation(visualizeProgram.Program, "Scale");

	glEnable(GL_BLEND);

	glViewport(0, 0, WIDTH, HEIGHT);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, velocity.Ping.TextureHandle);
	glUniform3f(fillColor, 1.0, 1.0, 1.0);
	glUniform2f(scale, 1.0f / WIDTH, 1.0f / HEIGHT);
	glBindVertexArray(QuadVao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glDisable(GL_BLEND);
}

int main()
{
	// Init GLFW
	glfwInit();

	// Set all the required options for GLFW
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	// Create a GLFWwindow object that we can use for GLFW's functions
	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Fluid simulation", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	// Set the required callback functions
	glfwSetKeyCallback(window, key_callback);
	glfwSetCursorPosCallback(window, mouse_callback);
	glfwSetScrollCallback(window, scroll_callback);

	// GLFW Options
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// Set this to true so GLEW knows to use a modern approach to retrieving function pointers and extensions
	glewExperimental = GL_TRUE;
	// Initialize GLEW to setup the OpenGL Function pointers
	glewInit();

	// Define the viewport dimensions
	glViewport(0, 0, WIDTH, HEIGHT);

	// OpenGL options
	//glEnable(GL_DEPTH_TEST);

	initialize();

	Shader vizualizeProgram("defaultVS.vs", "visualize.fs");

	// Game loop
	while (!glfwWindowShouldClose(window))
	{
		// Calculate deltatime of current frame
		GLfloat currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;

		// Check if any events have been activiated (key pressed, mouse moved etc.) and call corresponding response functions
		glfwPollEvents();

		update((unsigned int)(deltaTime * 1000));
		render(vizualizeProgram);

		// Swap the screen buffers
		glfwSwapBuffers(window);

		std::this_thread::sleep_for(std::chrono::milliseconds(300));
		lastFrame = currentFrame;
	}

	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();

	return 0;
}

// Is called whenever a key is pressed/released via GLFW
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);
	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			keys[key] = true;
		else if (action == GLFW_RELEASE)
			keys[key] = false;
	}
}

void do_movement()
{
	// Camera controls
	if (keys[GLFW_KEY_W])
		camera.ProcessKeyboard(FORWARD, deltaTime);
	if (keys[GLFW_KEY_S])
		camera.ProcessKeyboard(BACKWARD, deltaTime);
	if (keys[GLFW_KEY_A])
		camera.ProcessKeyboard(LEFT, deltaTime);
	if (keys[GLFW_KEY_D])
		camera.ProcessKeyboard(RIGHT, deltaTime);
}

bool firstMouse = true;
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (firstMouse)
	{
		lastX = xpos;
		lastY = ypos;
		firstMouse = false;
	}

	GLfloat xoffset = xpos - lastX;
	GLfloat yoffset = ypos - lastY;  // Reversed since y-coordinates go from bottom to left

	lastX = xpos;
	lastY = ypos;

	camera.ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}