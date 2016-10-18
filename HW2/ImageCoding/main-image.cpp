#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <string>
#include <vector>
#include <iostream>
#include <fstream>

#define M_PI 3.141592654f

unsigned int g_windowWidth = 600;
unsigned int g_windowHeight = 600;
char* g_windowName = "HW2-Transform-Coding-Image";

#define IMAGE_FILE "data/cameraman.ppm"

GLFWwindow* g_window;

int g_image_width;
int g_image_height;

std::vector<float> g_luminance_data;
std::vector<float> g_compressed_luminance_data;

struct color
{
	unsigned char r, g, b;
};

bool g_draw_origin = true;

// auxiliary math functions
float dotProduct(const float* a, const float* b, int size)
{
	float sum = 0;
	for (int i = 0; i < size; i++)
	{
		sum += a[i] * b[i];
	}
	return sum;
}

void normalize(float* a, int size)
{
	float len = 0;
	for (int i = 0; i < size; i++)
	{
		len += a[i] * a[i];
	}
	len = sqrt(len);
	for (int i = 0; i < size; i++)
	{
		a[i] = a[i] / len;
	}
}

// input (size x 1) vector a and b, output (size x size ) outerProduct matrix r
void outerProduct(const float* a, const float* b, float* r, int size)
{
	for (int i = 0; i < size; i++)
	{
		for (int j = 0; j < size; j++)
		{
			r[i * size + j] = a[i] * b[j];
		}
	}
}

void ZeroBased(const float* original, float* modified)
{
	for (int i = 0; i < 64; i++)
	{
		modified[i] = original[i] - 0.5f;
	}
}

void UndoZeroBase(float* array)
{
	for (int i = 0; i < 64; i++)
	{
		array[i] += + 0.5f;
	}
}


void DCTvector(int N, int k, float* q)
{
	for (int i = 0; i < N; i++)
	{
		q[i] = cos((M_PI / 16) * (k) * (2 * i + 1));
	}

}

void DCTmatrix(int N, int i, int k, float* q)
{
	float dcti[8];
	float dctk[8];
	DCTvector(8, i, dcti);
	DCTvector(8, k, dctk);

	outerProduct(dcti, dctk, q, 8);

}

void Quantize(float* B, int m)
{
	for (int i = 0; i < 8; i++)
	{
		for (int j = 0; j < 8; j++)
		{
			if (i + j > m)
				B[8 * i + j] = 0;
		}
	}
}

void ClearOut(float* B)
{
	for (int i = 0; i < 64; i++)
	{
		B[i] = 0.0f;
	}
}

void CompressBlock(const float* A, float* B, int m)
{
	// TODO: Homework Task 2 (see the PDF description)
	
	float dctM [64];
	float vals[64];
	float dctVals[64];
	
	//moves the values of A into vals and changes to base zero
	ZeroBased(A, vals);

	//run DCT
	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			DCTmatrix(8, y, x, dctM);
			normalize(dctM, 64);
			dctVals[y * 8 + x] = dotProduct(dctM, vals, 64);
		}
	}
	//Quantize
	Quantize(dctVals, m);

	//Clear out B
	ClearOut(B);

	//undo DCT
	for (int dct_y = 0; dct_y < 8; dct_y++)
	{
		for (int dct_x = 0; dct_x < 8; dct_x++)
		{
			DCTmatrix(8, dct_y, dct_x, dctM);
			normalize(dctM, 64);
			float c = dctVals[8 * dct_y + dct_x];
			for (int y = 0; y < 8; y++)
			{
				for (int x = 0; x < 8; x++)
				{
					B[8 * y + x] += c * dctM[8 * y + x];
				}
			}
		}
	}
	UndoZeroBase(B);
}

void TakeInBlock(const std::vector<float> source, float* piece, int block_y, int block_x)
{
	int topLeftIndex = 8 * g_image_width * block_y + 8 * block_x;
	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			piece[y * 8 + x] = source[topLeftIndex + g_image_width * y + x];
		}
	}
	
}

void PutOutBlock(std::vector<float>& output, const float* compressed_piece, int block_y, int block_x)
{
	int topLeftIndex = 8 * g_image_width * block_y + 8 * block_x;
	for (int y = 0; y < 8; y++)
	{
		for (int x = 0; x < 8; x++)
		{
			output[topLeftIndex + g_image_width * y + x] = compressed_piece[y * 8 + x];
		}
	}
}


void CompressImage(const std::vector<float> I, std::vector<float>& O, int m)
{
	// TODO: Homework Task 2 (see the PDF description)
	float In[64];
	float Out[64];

	for (int blockx = 0; blockx < g_image_width / 8; blockx++)
	{
		for (int blocky = 0; blocky < g_image_height / 8; blocky++)
		{
			TakeInBlock(I, In, blocky, blockx);
			CompressBlock(In, Out, m);
			PutOutBlock(O, Out, blocky, blockx);
		}
	}
	
}

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

bool LoadPPM(FILE *fp, int &width, int &height, std::vector<color> &data)
{
	const int bufferSize = 1024;
	char buffer[bufferSize];
	ReadLine(fp, bufferSize, buffer);
	if (buffer[0] != 'P' && buffer[1] != '6') return false;

	ReadLine(fp, bufferSize, buffer);
	while (buffer[0] == '#') ReadLine(fp, bufferSize, buffer);  // skip comments

	sscanf(buffer, "%d %d", &width, &height);

	ReadLine(fp, bufferSize, buffer);
	while (buffer[0] == '#') ReadLine(fp, bufferSize, buffer);  // skip comments

	data.resize(width*height);
	fread(data.data(), sizeof(color), width*height, fp);

	return true;
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
	else if (p_action == GLFW_PRESS)
	{
		switch (p_key)
		{
		case 49:	// press '1'
			g_draw_origin = true;
			break;
		case 50:	// press '2'
			g_draw_origin = false;
			break;
		default:
			break;
		}
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
	if (g_draw_origin)
		glDrawPixels(g_image_width, g_image_height, GL_LUMINANCE, GL_FLOAT, &g_luminance_data[0]);
	else
		glDrawPixels(g_image_width, g_image_height, GL_LUMINANCE, GL_FLOAT, &g_compressed_luminance_data[0]);
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

bool loadImage()
{
	std::vector<color> g_image_data;
	g_image_data.clear();
	g_image_width = 0;
	g_image_height = 0;
	FILE *fp = fopen(IMAGE_FILE, "rb");
	if (!fp) return false;

	bool success = false;
	success = LoadPPM(fp, g_image_width, g_image_height, g_image_data);

	g_luminance_data.resize(g_image_width * g_image_height);
	g_compressed_luminance_data.resize(g_image_width * g_image_height);
	for (int i = 0; i < g_image_height; i++)
	{
		for (int j = 0; j < g_image_width; j++)
		{
			// the index are not matching because of the difference between image space and OpenGl screen space
			g_luminance_data[i* g_image_width + j] = g_image_data[(g_image_height - i - 1)* g_image_width + j].r / 255.0f;
		}
	}

	g_windowWidth = g_image_width;
	g_windowHeight = g_image_height;

	return success;
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
			float tmp = g_compressed_luminance_data[i* g_image_width + j];
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

int main()
{
	loadImage();

	int n = 16;	// TODO: change the parameter n from 1 to 16 to see different image quality
	CompressImage(g_luminance_data, g_compressed_luminance_data, n);	

	writeImage();

	// render loop
	initWindow();
	initGL();
	renderLoop();

	return 0;
}
