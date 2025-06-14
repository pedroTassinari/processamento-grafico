#include <glad/glad.h>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <string>
#include <cmath>

const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// Protótipo da função de callback de teclado
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
void drawBackgroundLayer(GLuint texture, float offsetX, GLint modelLoc);

float characterXPos = 0.0f;
bool lookingRight = true;

float backgroundFarOffsetX = 0.0f;	// camadas distantes (ex: céu)
float backgroundMidOffsetX = 0.0f;	// camadas intermediárias (ex: árvores)
float backgroundNearOffsetX = 0.0f; // camadas próximas (ex: chão)

float vertices[] = {
	// positions     // texCoords
	1.0f,
	1.0f,
	1.0f,
	1.0f,
	1.0f,
	-1.0f,
	1.0f,
	0.0f,
	-1.0f,
	1.0f,
	0.0f,
	1.0f,
	-1.0f,
	-1.0f,
	0.0f,
	0.0f,
};

// Vertex shader embutido
const char *vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    layout (location = 1) in vec2 aTexCoord;

    uniform mat4 model;

    out vec2 TexCoord;

    void main() {
        gl_Position = model * vec4(aPos, 0.0, 1.0);
        TexCoord = aTexCoord;
    }
)";

// Fragment shader embutido
const char *fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;

    in vec2 TexCoord;

    uniform sampler2D texture1;

    void main() {
        FragColor = texture(texture1, TexCoord);
    }
)";

unsigned int loadTexture(const char *path)
{
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Parâmetros
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Carregamento da imagem
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(path, &width, &height, &nrChannels, 0);
	if (data)
	{
		GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cerr << "Erro ao carregar imagem: " << path << std::endl;
	}
	stbi_image_free(data);
	return textureID;
}

unsigned int compileShader(unsigned int type, const char *source)
{
	unsigned int shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, NULL);
	glCompileShader(shader);

	int success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetShaderInfoLog(shader, 512, NULL, infoLog);
		std::cerr << "Erro ao compilar shader:\n"
				  << infoLog << std::endl;
	}

	return shader;
}

unsigned int createShaderProgram()
{
	unsigned int vertexShader = compileShader(GL_VERTEX_SHADER, vertexShaderSource);
	unsigned int fragmentShader = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

	unsigned int shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	int success;
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	if (!success)
	{
		char infoLog[512];
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cerr << "Erro ao linkar programa:\n"
				  << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

int main()
{
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	GLFWwindow *window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Vivencial 2", NULL, NULL);
	if (!window)
	{
		std::cerr << "Erro ao criar janela GLFW\n";
		glfwTerminate();
		return -1;
	}
	glfwMakeContextCurrent(window);

	// Fazendo o registro da função de callback para a janela GLFW
	glfwSetKeyCallback(window, key_callback);

	gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);

	unsigned int shaderProgram = createShaderProgram();

	unsigned int VBO, VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	// posição
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
	glEnableVertexAttribArray(0);
	// coordenadas de textura
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	// Carregando texturas
	unsigned int skyBg = loadTexture("../assets/vivencial-2/Layers/Sky.png");
	unsigned int bgDecorBg = loadTexture("../assets/vivencial-2/Layers/BG_Decor.png");
	unsigned int middleDecorBg = loadTexture("../assets/vivencial-2/Layers/Middle_Decor.png");
	unsigned int foregroundBg = loadTexture("../assets/vivencial-2/Layers/Foreground.png");
	unsigned int groundBg = loadTexture("../assets/vivencial-2/Layers/Ground.png");
	unsigned int necromancer = loadTexture("../assets/vivencial-2/necromancer.png");

	glUseProgram(shaderProgram);
	glUniform1i(glGetUniformLocation(shaderProgram, "texture1"), 0);

	auto wrap = [](float &offset, float scaleX)
	{
		offset = std::fmod(offset, scaleX);
		if (offset < 0)
			offset += scaleX;
	};

	while (!glfwWindowShouldClose(window))
	{
		glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);

		glUseProgram(shaderProgram);
		glBindVertexArray(VAO);

		int modelLoc = glGetUniformLocation(shaderProgram, "model");

		// Sprite 1: Sky (mais distante)
		drawBackgroundLayer(skyBg, backgroundFarOffsetX, modelLoc);

		// Sprite 2: BG_Decor (intermediário)
		drawBackgroundLayer(bgDecorBg, backgroundMidOffsetX, modelLoc);

		// Sprite 3: Middle_Decor (intermediário)
		drawBackgroundLayer(middleDecorBg, backgroundMidOffsetX, modelLoc);

		// Sprite 4: Foreground (quase junto do personagem)
		drawBackgroundLayer(foregroundBg, backgroundNearOffsetX, modelLoc);

		// Sprite 5: Ground (nível do personagem)
		drawBackgroundLayer(groundBg, backgroundNearOffsetX, modelLoc);

		// character Necromancer
		glBindTexture(GL_TEXTURE_2D, necromancer);
		glm::mat4 model6 = glm::mat4(1.0f);
		float flipX = lookingRight ? 1.0f : -1.0f;
		model6 = glm::translate(model6, glm::vec3(characterXPos, -0.6f, 0.0f));
		model6 = glm::scale(model6, glm::vec3(0.5f * flipX, 0.5f, 1.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model6));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

		wrap(backgroundFarOffsetX, 2.0f);
		wrap(backgroundMidOffsetX, 2.0f);
		wrap(backgroundNearOffsetX, 2.0f);

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteProgram(shaderProgram);
	glfwTerminate();
	return 0;
}

// Função de callback de teclado - só pode ter uma instância (deve ser estática se
// estiver dentro de uma classe) - É chamada sempre que uma tecla for pressionada
// ou solta via GLFW
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
	float velocity = 0.1f; // velocidade do personagem
	if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
	{
		lookingRight = true;
		/* characterXPos += velocity; */
		backgroundFarOffsetX -= velocity * 0.1f;  // céu — bem lento
		backgroundMidOffsetX -= velocity * 0.5f;  // árvores — médio
		backgroundNearOffsetX -= velocity * 1.0f; // chão — acompanha o personagem
	}
	else if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
	{
		lookingRight = false;
		/* characterXPos -= velocity; */
		backgroundFarOffsetX += velocity * 0.1f;
		backgroundMidOffsetX += velocity * 0.5f;
		backgroundNearOffsetX += velocity * 1.0f;
	}
	else if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}
}

void drawBackgroundLayer(GLuint texture, float offsetX, GLint modelLoc)
{
	const float texWidth = 2.0f;

	// Wrap manual (refinado)
	offsetX = std::fmod(offsetX, texWidth);
	if (offsetX < 0)
		offsetX += texWidth;

	glBindTexture(GL_TEXTURE_2D, texture);
	for (int i = -1; i <= 1; ++i)
	{
		glm::mat4 model = glm::mat4(1.0f);
		model = glm::translate(model, glm::vec3(offsetX + i * texWidth, 0.0f, 0.0f));
		model = glm::scale(model, glm::vec3(texWidth, 1.0f, 1.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
}