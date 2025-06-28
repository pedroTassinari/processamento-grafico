#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <string.h>
#include <time.h>
// #define GL_LOG_FILE "gl.log"
#include <iostream>
#include <vector>
#include <fstream>

#include <glad/glad.h> // Carregamento dos ponteiros para funções OpenGL

// STB_IMAGE
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include "gl_utils.h"
#include "TileMap.h"
#include "DiamondView.h"
#include "SlideView.h"
#include "ltMath.h"
#include <algorithm>
#include <unordered_set>
#include <sstream>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

using namespace std;
using namespace glm;

int g_gl_width = 800;
int g_gl_height = 800;
float xi = -1.0f;
float xf = 1.0f;
float yi = -1.0f;
float yf = 1.0f;
float w = xf - xi;
float h = yf - yi;
float tw, th, tw2, th2;
int tileSetCols = 7, tileSetRows = 1;
float tileW, tileW2;
float tileH, tileH2;
int cx = -1, cy = -1;

int prev_cx = -1, prev_cy = -1;
int prev_tid = -1;

// offset para centralizar ou ajustar o mapa na tela
const float screenOffsetX = 0.9f;
const float screenOffsetY = 0.7f;

int coinsCollected = 0;

TilemapView *tview = new DiamondView();
TileMap *tmap = NULL;

GLFWwindow *g_window = NULL;

std::unordered_set<int> walkingTiles;
std::unordered_set<int> blockTiles;
std::unordered_set<int> deathTiles;
std::unordered_set<int> coinTiles;

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void tryMove(GLFWwindow *window, int newX, int newY);
bool canWalk(int tileId);
bool isBlocked(int tileId);
bool isDeadly(int tileId);
bool isCoin(int tileId);
void getNextPosition(int direction, int &newX, int &newY);

struct Character
{
	int x;
	int y;
	GLuint texture;
};

Character player;

std::unordered_set<int> readTiles(const char *filename)
{
	std::ifstream arq(filename);
	std::unordered_set<int> tiles;
	if (!arq.is_open())
	{
		printf("ERROR: Could not open file %s\n", filename);
		return tiles;
	}

	std::string line;
	while (std::getline(arq, line))
	{
		if (line.empty())
			continue; // pular linhas vazias

		std::stringstream ss(line);
		int tid;
		while (ss >> tid)
		{
			tiles.insert(tid);
			printf("%d ", tid); // debug para mostrar os tiles lidos
		}
		printf("\n");
	}

	arq.close();
	return tiles;
}

TileMap *readMap(const char *filename)
{
	ifstream arq(filename);
	int w, h;
	arq >> w >> h;
	TileMap *tmap = new TileMap(w, h, 0);
	for (int r = 0; r < h; r++)
	{
		for (int c = 0; c < w; c++)
		{
			int tid;
			arq >> tid;
			cout << tid << " ";
			tmap->setTile(c, h - r - 1, tid);
		}
		cout << endl;
	}
	arq.close();
	return tmap;
}

void loadTexture(unsigned int &texture, const char *filename)
{
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	GLfloat max_aniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &max_aniso);
	// set the maximum!
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, max_aniso);

	int width, height, nrChannels;

	unsigned char *data = stbi_load(filename, &width, &height, &nrChannels, 0);
	if (data)
	{
		if (nrChannels == 4)
		{
			cout << "Alpha channel" << endl;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		}
		else
		{
			cout << "Without Alpha channel" << endl;
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		}
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		printf("Error loading texture: %s\n", filename);
	}
	stbi_image_free(data);
}

void SRD2SRU(double &mx, double &my, float &x, float &y)
{
	x = xi + (mx / g_gl_width) * w;
	y = yi + (1 - (my / g_gl_height)) * h;
}

void mouse(double &mx, double &my)
{

	// cout << "DEBUG => mouse click" << endl;

	// 1) Definição do tile candidato ao clique
	float y = 0;
	float x = 0;
	SRD2SRU(mx, my, x, y);

	int c, r;
	tview->computeMouseMap(c, r, tw, th, x, y);
	c += (tmap->getWidth() - 1) / 2;
	r += (tmap->getHeight() - 1) / 2;
	// cout << "\tDEBUG => r: " << r << " c: " << c << endl;

	// 2) Verificar se o ponto pertence ao tile indicado:

	// 2.1) Normalização do clique:
	float x0, y0;
	tview->computeDrawPosition(c, r, tw, th, x0, y0);
	x0 += xi;

	// cout << "\tDEBUG => mx: " << x  << " my: " << y  << endl;
	// cout << "\tDEBUG => x0: " << x0 << " y0: " << y0 << endl;
	// cout << "\tDEBUG => tw: " << tw << endl;

	float point[] = {x, y};

	// 2.2) Verifica se o ponto está dentro do triângulo da esquerda ou da direita do losangulo (metades)
	//      Implementação via cálculo de área dos triangulos: area(ABC) == area(ABp)+area(ACp)+area(BCp)
	// triangulo ABC:
	float *abc = new float[6];

	// 2.2.1) Define metade da esquerda ou da direita
	bool left = x < (x0 + tw / 2.0f);
	// cout << "\tDEBUG => mx: " << x << " midx: " << (x0 + tw/2.0f) << endl;

	if (left)
	{ // left
		abc[0] = x0;
		abc[1] = y0 + th / 2.0f;
		abc[2] = x0 + tw / 2.0f;
		abc[3] = y0 + th;
		abc[4] = x0 + tw / 2.0f;
		abc[5] = y0;
		// cout << "DEBUG => TRG LFT [(x,y),...] = ([" << abc[0] << "," << abc[1] << "], "
		// << "[" << abc[2] << "," << abc[3] << "], "
		// << "[" << abc[4] << "," << abc[5] << "])" << endl;
	}
	else
	{ // right
		abc[0] = x0 + tw / 2.0f;
		abc[1] = y0;
		abc[2] = x0 + tw / 2.0f;
		abc[3] = y0 + th;
		abc[4] = x0 + tw;
		abc[5] = y0 + th / 2.0f;
		// cout << "DEBUG => TRG RGHT [(x,y),...] = ([" << abc[0] << "," << abc[1] << "], "
		// << "[" << abc[2] << "," << abc[3] << "], "
		// << "[" << abc[4] << "," << abc[5] << "])" << endl;
	}

	// 2.3) Calcular colisão do ponto com o triangulo
	bool collide = triangleCollidePoint2D(abc, point);

	if (!collide)
	{
		// 2.4) Em caso "erro" de cálculo, deve ser feito o tileWalking para tile certo!
		cout << "tileWalking " << endl;
		if (left)
		{
			tview->computeTileWalking(c, r, DIRECTION_WEST);
		}
		else
		{
			tview->computeTileWalking(c, r, DIRECTION_EAST);
		}
	}

	if ((c < 0) || (c >= tmap->getWidth()) || (r < 0) || (r >= tmap->getHeight()))
	{
		cout << "wrong click position: " << c << ", " << r << endl;
		return; // posição inválida!
	}

	cout << "SELECIONADO c=" << c << "," << r << endl;
	cx = c;
	cy = r;
}

int main()
{
	restart_gl_log();
	// all the GLFW and GLEW start-up code is moved to here in gl_utils.cpp
	start_gl();
	// tell GL to only draw onto a pixel if the shape is closer to the viewer
	glEnable(GL_DEPTH_TEST); // enable depth-testing
	glDepthFunc(GL_LESS);

	cout << "Tentando criar tmap" << endl;
	tmap = readMap("../src/final-game/config/map.txt");
	tw = w / (float)tmap->getWidth();
	th = tw / 2.0f;
	tw2 = th;
	th2 = th / 2.0f;
	tileW = 1.0f / (float)tileSetCols;
	tileW2 = tileW / 2.0f;
	tileH = 1.0f / (float)tileSetRows;
	tileH2 = tileH / 2.0f;

	cout << "tw=" << tw << " th=" << th << " tw2=" << tw2 << " th2=" << th2
		 << " tileW=" << tileW << " tileH=" << tileH
		 << " tileW2=" << tileW2 << " tileH2=" << tileH2
		 << endl;

	GLuint tid;
	loadTexture(tid, "../src/final-game/assets/tilesetIso.png");

	GLuint characterTexture;
	loadTexture(characterTexture, "../src/final-game/assets/character.png");
	printf("Character texture loaded: %d\n", characterTexture);

	// Inicializa personagem em uma posição qualquer válida
	player = {1, 1, characterTexture}; // ou outra coordenada válida

	tmap->setTid(tid);
	cout << "Tmap inicializado" << endl;

	walkingTiles = readTiles("../src/final-game/config/walking-tiles.txt");
	blockTiles = readTiles("../src/final-game/config/block-tiles.txt");
	deathTiles = readTiles("../src/final-game/config/death-tiles.txt");
	coinTiles = readTiles("../src/final-game/config/coin-tiles.txt");

	// LOAD TEXTURES

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
		// positions   // texture coords
		xi, yi + th2, 0.0f, tileH2,		  // left
		xi + tw2, yi, tileW2, 0.0f,		  // bottom
		xi + tw, yi + th2, tileW, tileH2, // right
		xi + tw2, yi + th, tileW2, tileH, // top
	};
	unsigned int indices[] = {
		0, 1, 3, // first triangle
		3, 1, 2	 // second triangle
	};

	float characterVertices[] = {
		// x,        y,        s,    t
		-0.0625f, 0.0625f, 0.0f, 0.0f,	// Top-left
		-0.0625f, -0.0625f, 0.0f, 1.0f, // Bottom-left
		0.0625f, 0.0625f, 1.0f, 0.0f,	// Top-right
		0.0625f, -0.0625f, 1.0f, 1.0f	// Bottom-right
	};

	/* float characterVertices[] = {
		// x   y    z    s     t
		0.0,  th/2.0f,   0.0, 0.0,    /2.0f, //A
		tw/2.0f, th,     0.0, ds/2.0f, dt,     //B
		tw/2.0f, 0.0,    0.0, ds/2.0f, 0.0,    //D
		tw,     th/2.0f, 0.0, ds,     dt/2.0f  //C
	}; */

	// Map

	unsigned int VBO,
		VAO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// position attribute
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);
	// texture coord attribute
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Character
	unsigned int characterVAO, characterVBO, characterEBO;
	//

	glGenVertexArrays(1, &characterVAO);
	glGenBuffers(1, &characterVBO);
	glGenBuffers(1, &characterEBO);

	glBindVertexArray(characterVAO);

	glBindBuffer(GL_ARRAY_BUFFER, characterVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(characterVertices), characterVertices, GL_STATIC_DRAW);

	// posição
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);
	// coordenadas de textura
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	////

	/* glGenVertexArrays(1, &characterVAO);
	glGenBuffers(1, &characterVBO);
	glGenBuffers(1, &characterEBO);

	glBindVertexArray(characterVAO);

	glBindBuffer(GL_ARRAY_BUFFER, characterVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(characterVertices), characterVertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, characterEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(characterIndices), characterIndices, GL_STATIC_DRAW);

	// Atributo de posição (location = 0)
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);

	// Atributo de textura (location = 1)
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);
 */
	glBindVertexArray(0); // opcional, por segurança

	char vertex_shader[1024 * 256];
	char fragment_shader[1024 * 256];
	parse_file_into_str("../src/vivencial3/_geral_vs.glsl", vertex_shader, 1024 * 256);
	parse_file_into_str("../src/vivencial3/_geral_fs.glsl", fragment_shader, 1024 * 256);

	GLuint vs = glCreateShader(GL_VERTEX_SHADER);
	const GLchar *p = (const GLchar *)vertex_shader;
	glShaderSource(vs, 1, &p, NULL);
	glCompileShader(vs);

	// check for compile errors
	int params = -1;
	glGetShaderiv(vs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", vs);
		print_shader_info_log(vs);
		return 1; // or exit or something
	}

	GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
	p = (const GLchar *)fragment_shader;
	glShaderSource(fs, 1, &p, NULL);
	glCompileShader(fs);

	// check for compile errors
	glGetShaderiv(fs, GL_COMPILE_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: GL shader index %i did not compile\n", fs);
		print_shader_info_log(fs);
		return 1; // or exit or something
	}

	GLuint shader_programme = glCreateProgram();
	glAttachShader(shader_programme, fs);
	glAttachShader(shader_programme, vs);
	glLinkProgram(shader_programme);

	glGetProgramiv(shader_programme, GL_LINK_STATUS, &params);
	if (GL_TRUE != params)
	{
		fprintf(stderr, "ERROR: could not link shader programme GL index %i\n",
				shader_programme);
		// 		print_programme_info_log( shader_programme );
		return false;
	}

	float previous = glfwGetTime();

	for (int r = 0; r < tmap->getHeight(); r++)
	{
		for (int c = 0; c < tmap->getWidth(); c++)
		{
			unsigned char t_id = tmap->getTile(c, r);
			cout << ((int)t_id) << " ";
		}
		cout << endl;
	}

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glfwSetKeyCallback(g_window, key_callback);

	// glEnable(GL_DEPTH_TEST);
	while (!glfwWindowShouldClose(g_window))
	{
		_update_fps_counter(g_window);
		double current_seconds = glfwGetTime();

		// wipe the drawing surface clear
		glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// glClear(GL_COLOR_BUFFER_BIT);

		glViewport(0, 0, g_gl_width, g_gl_height);

		glUseProgram(shader_programme);

		glBindVertexArray(VAO);
		float x, y;
		int r = 0, c = 0;

		for (int r = 0; r < tmap->getHeight(); r++)
		{
			for (int c = 0; c < tmap->getWidth(); c++)
			{
				int t_id = (int)tmap->getTile(c, r);
				int u = t_id % tileSetCols;
				int v = t_id / tileSetCols;

				tview->computeDrawPosition(c, r, tw, th, x, y);

				glUniform1f(glGetUniformLocation(shader_programme, "offsetx"), u * tileW);
				glUniform1f(glGetUniformLocation(shader_programme, "offsety"), v * tileH);
				glUniform1f(glGetUniformLocation(shader_programme, "tx"), x + screenOffsetX);
				glUniform1f(glGetUniformLocation(shader_programme, "ty"), y + screenOffsetY);
				glUniform1f(glGetUniformLocation(shader_programme, "layer_z"), tmap->getZ());
				glUniform1f(glGetUniformLocation(shader_programme, "weight"), (c == cx) && (r == cy) ? 0.5 : 0.0);

				// bind Texture
				/* glActiveTexture(GL_TEXTURE0); */
				glBindTexture(GL_TEXTURE_2D, tmap->getTileSet());
				glUniform1i(glGetUniformLocation(shader_programme, "sprite"), 0);
				glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
			}
		}

		// ======================
		// Desenhar personagem
		// ======================
		tview->computeDrawPosition(player.x, player.y, tw, th, x, y);

		glUniform1f(glGetUniformLocation(shader_programme, "offsetx"), 0.0f); // ajuste se for spritesheet
		glUniform1f(glGetUniformLocation(shader_programme, "offsety"), 0.0f);
		glUniform1f(glGetUniformLocation(shader_programme, "tx"), x - 0.03 /* + screenOffsetX */);
		glUniform1f(glGetUniformLocation(shader_programme, "ty"), y - 0.22 /* + screenOffsetY */);
		glUniform1f(glGetUniformLocation(shader_programme, "layer_z"), -0.1f);
		glUniform1f(glGetUniformLocation(shader_programme, "weight"), 0.0f); // sem highlight

		glBindVertexArray(characterVAO);
		glBindTexture(GL_TEXTURE_2D, player.texture);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		glfwPollEvents();

		cx = std::clamp(cx, 0, tmap->getWidth() - 1);
		cy = std::clamp(cy, 0, tmap->getHeight() - 1);

		// Restaura o tile anterior
		if (prev_cx != -1 && prev_cy != -1 && prev_tid != -1)
		{
			tmap->setTile(prev_cx, prev_cy, prev_tid);
		}

		// Salva a posição e o tid atuais como "anteriores"
		prev_cx = cx;
		prev_cy = cy;
		prev_tid = tmap->getTile(cx, cy);

		// Modifica o tile atual
		tmap->setTile(cx, cy, 6);

		player.x = cx;
		player.y = cy;

		double mx, my;
		glfwGetCursorPos(g_window, &mx, &my);

		const int state = glfwGetMouseButton(g_window, GLFW_MOUSE_BUTTON_LEFT);

		if (state == GLFW_PRESS)
		{
			mouse(mx, my);
		}

		// put the stuff we've been drawing onto the display
		glfwSwapBuffers(g_window);
	}

	// close GL context and any other GLFW resources
	glfwTerminate();
	delete tmap;
	return 0;
}

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
		return;
	}

	int newX, newY;

	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_W:
			getNextPosition(4, newX, newY); // Norte
			break;
		case GLFW_KEY_S:
			getNextPosition(0, newX, newY); // Sul
			break;
		case GLFW_KEY_D:
			getNextPosition(2, newX, newY); // Leste
			break;
		case GLFW_KEY_A:
			getNextPosition(6, newX, newY); // Oeste
			break;
		case GLFW_KEY_E:
			getNextPosition(3, newX, newY); // Sudeste
			break;
		case GLFW_KEY_Q:
			getNextPosition(7, newX, newY); // Noroeste
			break;
		case GLFW_KEY_Z:
			getNextPosition(5, newX, newY); // Sudoeste
			break;
		case GLFW_KEY_C:
			getNextPosition(1, newX, newY); // Nordeste
			break;
		default:
			break;
		}

		tryMove(window, newX, newY); // ← decide se move ou não
	}
}

void tryMove(GLFWwindow *window, int newX, int newY)
{
	if (newX < 0 || newX >= tmap->getWidth() || newY < 0 || newY >= tmap->getHeight())
	{
		printf("Invalid position: (%d, %d)\n", newX, newY);
		return;
	}

	int tileId = tmap->getTile(newX, newY);

	if (isBlocked(tileId))
	{
		printf("Tile %d is blocked, cannot move there.\n", tileId);
		return;
	}

	if (isDeadly(tileId))
	{
		printf("Tile %d is deadly, game over\n", tileId);
		printf("You collected %d coins before dying.\n", coinsCollected);
		glfwSetWindowShouldClose(window, GL_TRUE);
		return;
	}

	if (canWalk(tileId))
	{
		printf("Walking to tile: %d\n", tileId);
		cx = newX;
		cy = newY;
	}

	if (isCoin(tileId))
	{
		coinsCollected++;
		printf("Coin collected! Total coins: %d\n", coinsCollected);
		tmap->setTile(newX, newY, 1); // Remove the coin tile
		cx = newX;
		cy = newY;
		if (coinsCollected >= 10)
		{
			printf("You collected enough coins! You win!\n");
			glfwSetWindowShouldClose(window, GL_TRUE);
			return;
		}
	}
}

bool canWalk(int tileId)
{
	return walkingTiles.count(tileId) > 0;
}

bool isBlocked(int tileId)
{
	return blockTiles.count(tileId) > 0;
}

bool isDeadly(int tileId)
{
	return deathTiles.count(tileId) > 0;
}

bool isCoin(int tileId)
{
	return coinTiles.count(tileId) > 0;
}

void getNextPosition(int direction, int &newX, int &newY)
{
	newX = cx;
	newY = cy;
	tview->computeTileWalking(newX, newY, direction);
}