#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#define M_PI 3.141592654f

unsigned int g_windowWidth = 600;
unsigned int g_windowHeight = 600;
char* g_windowName = "HW3-Rasterization";

GLFWwindow* g_window;

const int g_image_width = g_windowWidth;
const int g_image_height = g_windowHeight;

std::vector<float> g_image;

struct color
{
	unsigned char r, g, b;
};

int ReadLine(FILE *fp, int size, char *buffer)
{
	int i;
	for (i = 0; i < size; i++) {
		buffer[i] = fgetc(fp);
		if (feof(fp) || buffer[i] == '\n' || buffer[i] == '\r') {
			buffer[i] = '\0';
			return i + 1;
		}
	}
	return i;
}

//-------------------------------------------------------------------------------

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
}

void render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glDrawPixels(g_image_width, g_image_height, GL_LUMINANCE, GL_FLOAT, &g_image[0]);
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

void initImage()
{
	g_image.resize(g_image_width * g_image_height);
}

bool writeImage()
{
	std::vector<color> tmpData;
	tmpData.resize(g_image_width * g_image_height);

	for (int i = 0; i < g_image_height; i++)
	{
		for (int j = 0; j < g_image_width; j++)
		{
			// make sure the value will not be larger than 1 or smaller than 0, which might cause problem when converting to unsigned char
			float tmp = g_image[i* g_image_width + j];
			if (tmp < 0.0f)	tmp = 0.0f;
			if (tmp > 1.0f)	tmp = 1.0f;

			tmpData[(g_image_height - i - 1)* g_image_width + j].r = unsigned char(tmp * 255.0);
			tmpData[(g_image_height - i - 1)* g_image_width + j].g = unsigned char(tmp * 255.0);
			tmpData[(g_image_height - i - 1)* g_image_width + j].b = unsigned char(tmp * 255.0);
		}
	}

	FILE *fp = fopen("data/out.ppm", "wb");
	if (!fp) return false;

	fprintf(fp, "P6\r");
	fprintf(fp, "%d %d\r", g_image_width, g_image_height);
	fprintf(fp, "255\r");
	fwrite(tmpData.data(), sizeof(color), g_image_width * g_image_height, fp);

	return true;
}

//-------------------------------------------------------------------------------

void putPixel(int x, int y)
{
	// clamp
	if (x >= g_image_width || x < 0 || y >= g_image_height || y < 0) return;

	// write
	g_image[y* g_image_width + x] = 1.0f;
}

void drawLine(int x1, int y1, int x2, int y2)
{
	// Task 1
	int dx = x2 - x1;
	int dy = y2 - y1;

	if (abs(dx) > abs(dy))
	{
		//Make sure we start from the correct point
		if (x2 < x1)
		{
			int temp = x1;
			x1 = x2;
			x2 = temp;

			temp = y1;
			y1 = y2;
			y2 = temp;
		}

		int slopeFactor = 1;
		if (y2 < y1)
			slopeFactor = -1;

		int D = 2 * dy - dx * slopeFactor;
		int inc0 = 2 * dy;
		int inc1 = 2 * (dy - dx * slopeFactor);

		putPixel(x1, y1);
		while (x1 < x2)
		{
			if ((D <= 0 && slopeFactor == 1) || (D >= 0 && slopeFactor == -1))
			{
				D += inc0;
			}
			else
			{
				D += inc1;
				y1 += slopeFactor;
			}
			x1++;
			putPixel(x1, y1);
		}
	}
	
	else
	{
		//Make sure we start from the correct point
		if (y2 < y1)
		{
			int temp = x1;
			x1 = x2;
			x2 = temp;

			temp = y1;
			y1 = y2;
			y2 = temp;
		}

		int slopeFactor = 1;
		if (x2 < x1)
			slopeFactor = -1;

		int D = 2 * dx - dy * slopeFactor;
		int inc0 = 2 * dx;
		int inc1 = 2 * (dx - dy * slopeFactor);

		putPixel(x1, y1);
		while (y1 < y2)
		{
			if ((D <= 0 && slopeFactor == 1) || (D >= 0 && slopeFactor == -1))
			{
				D += inc0;
			}
			else
			{
				D += inc1;
				x1 += slopeFactor;
			}
			y1++;
			putPixel(x1, y1);
		}
	}
}

void CirclePoints(int x, int y, int centerx, int centery, int radius)
{
	putPixel(centerx + x, centery + y);
	putPixel(centerx + x, centery - y);
	putPixel(centerx - x, centery + y);
	putPixel(centerx - x, centery - y);
	putPixel(centerx + y, centery + x);
	putPixel(centerx + y, centery - x);
	putPixel(centerx - y, centery + x);
	putPixel(centerx - y, centery - x);
}

void drawCircle(int x0, int y0, int R)
{
	// Task 2
	int x1 = 0;
	int y1 = R;
	int D = 1 - R;

	CirclePoints(x1, y1, x0, y0, R);
	while (y1 > x1)
	{
		if (D < 0)
		{
			D += 2 * x1 + 3;
		}
		else
		{
			D += 2 * (x1 - y1) + 5;
			y1--;
		}
		x1++;
		CirclePoints(x1, y1, x0, y0, R);
	}
}

void drawImage()
{	
	drawLine(150, 10, 450, 10);
	drawLine(150, 310, 450, 310);
	drawLine(150, 10, 150, 310);
	drawLine(450, 10, 450, 310);
	drawLine(150, 310, 300, 410);
	drawLine(300, 410, 450, 310);

	drawCircle(500, 500, 50);
}

int main()
{
	initImage();
	drawImage();
	writeImage();

	// render loop
	initWindow();
	initGL();
	renderLoop();

	return 0;
}
