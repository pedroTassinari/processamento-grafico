#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>

std::vector<float> vertices;
std::vector<float> colors;
GLuint VBOs[2], VAO;
GLuint shaderProgram;

glm::vec3 randomColor()
{
    return glm::vec3(static_cast<float>(rand()) / RAND_MAX,
                     static_cast<float>(rand()) / RAND_MAX,
                     static_cast<float>(rand()) / RAND_MAX);
}

void mouse_button_callback(GLFWwindow *window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        double xpos, ypos;
        glfwGetCursorPos(window, &xpos, &ypos);

        int width, height;
        glfwGetWindowSize(window, &width, &height);

        float x = 2.0f * xpos / width - 1.0f;
        float y = 1.0f - 2.0f * ypos / height;

        vertices.push_back(x);
        vertices.push_back(y);
        vertices.push_back(0.0f);

        int vertexCount = vertices.size() / 3;

        if (vertexCount % 3 == 1)
        {
            glm::vec3 newColor = randomColor();
            colors.push_back(newColor.r);
            colors.push_back(newColor.g);
            colors.push_back(newColor.b);
        }
        else if (vertexCount % 3 == 2)
        {
            glm::vec3 lastColor(colors.end()[-3], colors.end()[-2], colors.end()[-1]);
            colors.push_back(lastColor.r);
            colors.push_back(lastColor.g);
            colors.push_back(lastColor.b);
        }
        else
        {
            glm::vec3 lastColor(colors.end()[-3], colors.end()[-2], colors.end()[-1]);
            colors.push_back(lastColor.r);
            colors.push_back(lastColor.g);
            colors.push_back(lastColor.b);
        }

        glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
        glBufferData(GL_ARRAY_BUFFER, colors.size() * sizeof(float), colors.data(), GL_STATIC_DRAW);
    }
}

const char *vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aColor;
out vec3 vertexColor;
void main() {
    gl_Position = vec4(aPos, 1.0);
    vertexColor = aColor;
})";

const char *fragmentShaderSource = R"(
#version 330 core
in vec3 vertexColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vertexColor, 1.0f);
})";

void compileShaders()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    shaderProgram = glCreateProgram();

    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

int main()
{
    srand(static_cast<unsigned int>(time(0)));
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(800, 600, "Triângulos por clique", NULL, NULL);
    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc)glfwGetProcAddress);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    compileShaders();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(2, VBOs);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBOs[0]);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(0);

    glBindBuffer(GL_ARRAY_BUFFER, VBOs[1]);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, (void *)0);
    glEnableVertexAttribArray(1);

    while (!glfwWindowShouldClose(window))
    {
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);

        glDrawArrays(GL_TRIANGLES, 0, vertices.size() / 3);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}
