#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <math.h>

unsigned int g_windowWidth = 800;
unsigned int g_windowHeight = 600;
char* g_windowName = "HW1-OpenGL-Basics";

GLFWwindow* g_window;

// model data
std::vector<float> g_meshVertices;
std::vector<float> g_meshNormals;
std::vector<unsigned int> g_meshIndices;

GLfloat g_modelViewMatrix[16];

float rotationSpeed = 0.0f;
float lastTime = 0.0f;
float currentAngle;

// auxiliary math functions
float dotProduct(const float* a, const float* b)
{
	return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

void crossProduct(const float* a, const float* b, float* r)
{
	r[0] = a[1] * b[2] - a[2] * b[1];
	r[1] = a[2] * b[0] - a[0] * b[2];
	r[2] = a[0] * b[1] - a[1] * b[0];
}

void normalize(float* a)
{
	const float len = sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);

	a[0] /= len;
	a[1] /= len;
	a[2] /= len;
}

void computeNormals()
{
	g_meshNormals.resize(g_meshVertices.size());

	// the code below sets all normals to point in the z-axis, so we get a boring constant gray color
	// the following should be replaced with your code for normal computation (Task 1)

	std::vector<float> faceNormals;
	for (int i = 0; i < g_meshIndices.size(); i += 3)
	{
		float edge1 [3];
		float edge2 [3];

		edge1[0] = g_meshVertices[g_meshIndices[i] * 3] - g_meshVertices[g_meshIndices[i + 1] * 3];
		edge1[1] = g_meshVertices[g_meshIndices[i] * 3 + 1] - g_meshVertices[g_meshIndices[i + 1] * 3 + 1];
		edge1[2] = g_meshVertices[g_meshIndices[i] * 3 + 2] - g_meshVertices[g_meshIndices[i + 1] * 3 + 2];

		edge2[0] = g_meshVertices[g_meshIndices[i] * 3] - g_meshVertices[g_meshIndices[i + 2] * 3];
		edge2[1] = g_meshVertices[g_meshIndices[i] * 3 + 1] - g_meshVertices[g_meshIndices[i + 2] * 3 + 1];
		edge2[2] = g_meshVertices[g_meshIndices[i] * 3 + 2] - g_meshVertices[g_meshIndices[i + 2] * 3 + 2];

		float normal [3];
		crossProduct(edge1, edge2, normal);
		normalize(normal);
		faceNormals.push_back(normal[0]);
		faceNormals.push_back(normal[1]);
		faceNormals.push_back(normal[2]);

	}
	
	for (int vertexIndex = 0; vertexIndex < g_meshVertices.size() / 3; vertexIndex++)
	{
		float divisor = 0.f;
		float sum[3] = { 0.f, 0.f, 0.f };
		for (int faceIndex = 0; faceIndex < g_meshIndices.size(); faceIndex += 3)
		{
			if (g_meshIndices[faceIndex] == vertexIndex || 
				g_meshIndices[faceIndex + 1] == vertexIndex || 
				g_meshIndices[faceIndex + 2] == vertexIndex)
			{
				sum[0] += faceNormals[faceIndex];
				sum[1] += faceNormals[faceIndex + 1];
				sum[2] += faceNormals[faceIndex + 2];
				divisor++;
			}
		}

		float vertNormal[3];
		vertNormal[0] = sum[0] / divisor;
		vertNormal[1] = sum[1] / divisor;
		vertNormal[2] = sum[2] / divisor;
		normalize(vertNormal);

		g_meshNormals[3 * vertexIndex] = vertNormal[0];
		g_meshNormals[3 * vertexIndex + 1] = vertNormal[1];
		g_meshNormals[3 * vertexIndex + 2] = vertNormal[2];
	}

	/*
	for (int v = 0; v < g_meshNormals.size() / 3; ++v)
	{
		g_meshNormals[3 * v + 2] = 1.0;
	}
	*/
}

void loadObj(std::string p_path)
{
	std::ifstream nfile;
	nfile.open(p_path);
	std::string s;

	while (nfile >> s)
	{
		if (s.compare("v") == 0)
		{
			float x, y, z;
			nfile >> x >> y >> z;
			g_meshVertices.push_back(x);
			g_meshVertices.push_back(y);
			g_meshVertices.push_back(z);
		}		
		else if (s.compare("f") == 0)
		{
			std::string sa, sb, sc;
			unsigned int a, b, c;
			nfile >> sa >> sb >> sc;

			a = std::stoi(sa);
			b = std::stoi(sb);
			c = std::stoi(sc);

			g_meshIndices.push_back(a - 1);
			g_meshIndices.push_back(b - 1);
			g_meshIndices.push_back(c - 1);
		}
		else
		{
			std::getline(nfile, s);
		}
	}

	computeNormals();

	std::cout << p_path << " loaded. Vertices: " << g_meshVertices.size() / 3 << " Triangles: " << g_meshIndices.size() / 3 << std::endl;
}

double getTime()
{
	return glfwGetTime();
}

void glfwErrorCallback(int error, const char* description)
{
	std::cerr << "GLFW Error " << error << ": " << description << std::endl;
	exit(1);
}

void glfwKeyCallback(GLFWwindow* p_window, int p_key, int p_scancode, int p_action, int p_mods)
{
	if (p_key == GLFW_KEY_ESCAPE && p_action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(g_window, GL_TRUE);
	}
}

void initWindow()
{
	// initialize GLFW
	glfwSetErrorCallback(glfwErrorCallback);
	if (!glfwInit())
	{
		std::cerr << "GLFW Error: Could not initialize GLFW library" << std::endl;
		exit(1);
	}

	g_window = glfwCreateWindow(g_windowWidth, g_windowHeight, g_windowName, NULL, NULL);
	if (!g_window)
	{
		glfwTerminate();
		std::cerr << "GLFW Error: Could not initialize window" << std::endl;
		exit(1);
	}

	// callbacks
	glfwSetKeyCallback(g_window, glfwKeyCallback);

	// Make the window's context current
	glfwMakeContextCurrent(g_window);

	// turn on VSYNC
	glfwSwapInterval(1);
}

void initGL()
{
	glClearColor(1.f, 1.f, 1.f, 1.0f);
	
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_DEPTH_TEST);
	glShadeModel(GL_SMOOTH);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, (GLfloat) g_windowWidth / (GLfloat)g_windowHeight, 0.1f, 10.0f);
}

void clearModelViewMatrix()
{
	for (int i = 0; i < 4; ++i)
	{
		for (int j = 0; j < 4; ++j)
		{
			g_modelViewMatrix[4 * i + j] = 0.0f;
		}
	}
}

void updateModelViewMatrix()
{	
	clearModelViewMatrix();
	
	// the following code sets a static modelView matrix
	// this should be replaced with code implementing Task 2 (teapot rotation)

	
	rotationSpeed += 0.005f;
	if (rotationSpeed > 1.0f)
		rotationSpeed = 1.0f;

	float currentTime = getTime();
	float deltaTime = currentTime - lastTime;
	currentAngle += rotationSpeed * deltaTime;
	
	
	g_modelViewMatrix[0] = cos(currentAngle);
	g_modelViewMatrix[2] = -sin(currentAngle);
	g_modelViewMatrix[8] = sin(currentAngle);
	g_modelViewMatrix[10] = cos(currentAngle);
	g_modelViewMatrix[5] = 1.0f;

	g_modelViewMatrix[14] = -5.0f;
	g_modelViewMatrix[15] = 1.0f;

	lastTime = currentTime;
}

void setModelViewMatrix()
{
	glMatrixMode(GL_MODELVIEW);
	updateModelViewMatrix();
	glLoadMatrixf(g_modelViewMatrix);
}

void render()
{
	setModelViewMatrix();

	glBegin(GL_TRIANGLES);

	for (size_t f = 0; f < g_meshIndices.size(); ++f)
	{
		const float scale = 0.1f;
		const unsigned int idx = g_meshIndices[f];
		const float x = scale * g_meshVertices[3 * idx + 0];
		const float y = scale * g_meshVertices[3 * idx + 1];
		const float z = scale * g_meshVertices[3 * idx + 2];

		const float nx = g_meshNormals[3 * idx + 0];
		const float ny = g_meshNormals[3 * idx + 1];
		const float nz = g_meshNormals[3 * idx + 2];
				
		glNormal3f(nx, ny, nz);
		glVertex3f(x, y, z);
	}

	glEnd();
	
}

void renderLoop()
{
	while (!glfwWindowShouldClose(g_window))
	{
		// clear buffers
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		render();

		// Swap front and back buffers
		glfwSwapBuffers(g_window);

		// Poll for and process events
		glfwPollEvents();
	}
}

int main()
{
	initWindow();
	initGL();
	loadObj("data/teapot.obj");
	renderLoop();
}
