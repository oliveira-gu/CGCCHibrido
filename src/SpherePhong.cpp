/* * Código Integrado - Atividade Vivencial 2 (Módulo 4)
 * Iluminação de 3 Pontos (Phong) com Atenuação em uma Esfera.
 */

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <assert.h>

using namespace std;

// GLAD e GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// STB Image (Usando aspas duplas pois o arquivo está na pasta local)
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace glm;

// --- PROTÓTIPOS ---
void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode);
int setupShader();
GLuint loadTexture(string filePath);
GLuint generateSphere(float radius, int latSegments, int lonSegments, int &nVertices);

// --- VARIÁVEIS GLOBAIS ---
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Controles das 3 luzes
bool keyLightOn = true;   // Tecla 1
bool fillLightOn = true;  // Tecla 2
bool backLightOn = true;  // Tecla 3

// --- SHADERS ---
const GLchar *vertexShaderSource = R"(
#version 400
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 texc;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 texCoord;
out vec3 vNormal;
out vec4 fragPos; 
out vec4 vColor;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    fragPos = model * vec4(position, 1.0);
    
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vNormal = normalMatrix * normal;
    
    texCoord = texc;
    vColor = vec4(color, 1.0);
})";

const GLchar *fragmentShaderSource = R"(
#version 400
in vec2 texCoord;
in vec4 fragPos;
in vec3 vNormal;
in vec4 vColor;

out vec4 color;

uniform sampler2D texBuff;
uniform vec3 camPos;

// Propriedades do Material da Esfera
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float q;

// Estrutura das Luzes
struct Light {
    vec3 position;
    vec3 color;
    bool enabled;
};
uniform Light lights[3];

void main()
{
    vec3 N = normalize(vNormal);
    vec3 V = normalize(camPos - vec3(fragPos));
    
    vec4 objectColor = texture(texBuff, texCoord);
    if(objectColor.a < 0.1) objectColor = vColor; 

    vec3 finalLight = vec3(0.0);

    for(int i = 0; i < 3; i++) {
        if(!lights[i].enabled) continue;

        vec3 L = normalize(lights[i].position - vec3(fragPos));
        float dist = length(lights[i].position - vec3(fragPos));
        
        // Fator de atenuação
        float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * (dist * dist));

        // Parcela Ambiente
        vec3 ambient = ka * lights[i].color;

        // Parcela Difusa (COM ATENUAÇÃO, conforme exigido no enunciado)
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = kd * diff * lights[i].color * attenuation;

        // Parcela Especular
        vec3 R = normalize(reflect(-L, N));
        float spec = max(dot(R, V), 0.0);
        spec = pow(spec, max(q, 1.0));
        vec3 specular = ks * spec * lights[i].color; 

        finalLight += (ambient + diffuse) * vec3(objectColor) + specular;
    }

    color = vec4(finalLight, 1.0);
})";

// --- FUNÇÃO MAIN ---
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(WIDTH, HEIGHT, "Gustavo Oliveira", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = setupShader();

    // Gera a geometria da Esfera
    int nVertices;
    GLuint VAO = generateSphere(0.5f, 32, 32, nVertices);

    // Tenta carregar uma textura (se não achar, a esfera fica com a cor base laranja definida na função)
    GLuint texID = loadTexture("pixelWall.png");

    glUseProgram(shaderID);

    // Configura a Câmera e a Projeção
    glm::vec3 camPos = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::mat4 view = glm::lookAt(camPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH/(float)HEIGHT, 0.1f, 100.0f);
    
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3fv(glGetUniformLocation(shaderID, "camPos"), 1, glm::value_ptr(camPos));
    glUniform1i(glGetUniformLocation(shaderID, "texBuff"), 0);

    // Posição base do objeto principal (A esfera)
    glm::vec3 objPos = glm::vec3(0.0f, 0.0f, 0.0f);
    float angleY = 0.0f;

    // Propriedades do Material de Phong para a Esfera
    glm::vec3 ka = glm::vec3(0.1f);
    glm::vec3 kd = glm::vec3(0.6f);
    glm::vec3 ks = glm::vec3(0.5f);
    float q = 32.0f; // Brilho especular

    glUniform3fv(glGetUniformLocation(shaderID, "ka"), 1, glm::value_ptr(ka));
    glUniform3fv(glGetUniformLocation(shaderID, "kd"), 1, glm::value_ptr(kd));
    glUniform3fv(glGetUniformLocation(shaderID, "ks"), 1, glm::value_ptr(ks));
    glUniform1f(glGetUniformLocation(shaderID, "q"), q);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glClearColor(0.05f, 0.05f, 0.05f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- CONFIGURAÇÃO DAS 3 LUZES ---
        
        // 1. Key Light (Principal): Intensa, vindo da frente/diagonal
        glm::vec3 keyPos = objPos + glm::vec3(2.0f, 2.0f, 2.0f);
        glUniform3fv(glGetUniformLocation(shaderID, "lights[0].position"), 1, glm::value_ptr(keyPos));
        glUniform3f(glGetUniformLocation(shaderID, "lights[0].color"), 1.0f, 1.0f, 1.0f);
        glUniform1i(glGetUniformLocation(shaderID, "lights[0].enabled"), keyLightOn);

        // 2. Fill Light (Preenchimento): Mais fraca, lado oposto
        glm::vec3 fillPos = objPos + glm::vec3(-2.0f, 1.0f, 2.0f);
        glUniform3fv(glGetUniformLocation(shaderID, "lights[1].position"), 1, glm::value_ptr(fillPos));
        glUniform3f(glGetUniformLocation(shaderID, "lights[1].color"), 0.3f, 0.3f, 0.4f); // Tom azulado para preencher a sombra
        glUniform1i(glGetUniformLocation(shaderID, "lights[1].enabled"), fillLightOn);

        // 3. Back Light (Fundo): Ilumina as bordas por trás
        glm::vec3 backPos = objPos + glm::vec3(0.0f, 2.0f, -3.0f);
        glUniform3fv(glGetUniformLocation(shaderID, "lights[2].position"), 1, glm::value_ptr(backPos));
        glUniform3f(glGetUniformLocation(shaderID, "lights[2].color"), 0.8f, 0.8f, 0.8f);
        glUniform1i(glGetUniformLocation(shaderID, "lights[2].enabled"), backLightOn);

        // --- DESENHANDO A ESFERA ---
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, objPos);
        angleY += 0.01f; // Rotaciona a esfera continuamente
        model = glm::rotate(model, angleY, glm::vec3(0.0f, 1.0f, 0.0f));
        
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texID);
        
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, nVertices);
        glBindVertexArray(0);

        glfwSwapBuffers(window);
    }
    
    glDeleteVertexArrays(1, &VAO);
    glfwTerminate();
    return 0;
}

// --- FUNÇÕES AUXILIARES ---

void key_callback(GLFWwindow *window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // Controles para ligar e desligar as luzes
    if (key == GLFW_KEY_1 && action == GLFW_PRESS) keyLightOn = !keyLightOn;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) fillLightOn = !fillLightOn;
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) backLightOn = !backLightOn;
}

int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

GLuint loadTexture(string filePath) {
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);   
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); 
    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0);
    
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "Imagem de textura não encontrada (" << filePath << "). Renderizando com a cor sólida." << std::endl;
    }
    stbi_image_free(data);
    return textureID;
}

GLuint generateSphere(float radius, int latSegments, int lonSegments, int &nVertices) {
    vector<GLfloat> vBuffer; 

    // Cor base caso não haja textura carregada (Laranja)
    vec3 color = vec3(1.0f, 0.5f, 0.0f); 

    auto calcPosUVNormal = [&](int lat, int lon, vec3& pos, vec2& uv, vec3& normal) {
        float theta = lat * pi<float>() / latSegments;
        float phi = lon * 2.0f * pi<float>() / lonSegments;

        pos = vec3(
            radius * cos(phi) * sin(theta),
            radius * cos(theta),
            radius * sin(phi) * sin(theta)
        );

        uv = vec2(
            phi / (2.0f * pi<float>()), 
            theta / pi<float>()          
        );

        normal = normalize(pos);
    };

    for (int i = 0; i < latSegments; ++i) {
        for (int j = 0; j < lonSegments; ++j) {
            vec3 v0, v1, v2, v3;
            vec2 uv0, uv1, uv2, uv3;
            vec3 n0, n1, n2, n3;

            calcPosUVNormal(i, j, v0, uv0, n0);
            calcPosUVNormal(i + 1, j, v1, uv1, n1);
            calcPosUVNormal(i, j + 1, v2, uv2, n2);
            calcPosUVNormal(i + 1, j + 1, v3, uv3, n3);

            // Primeiro triângulo
            vBuffer.insert(vBuffer.end(), { v0.x, v0.y, v0.z, color.r, color.g, color.b, n0.x, n0.y, n0.z, uv0.x, uv0.y });
            vBuffer.insert(vBuffer.end(), { v1.x, v1.y, v1.z, color.r, color.g, color.b, n1.x, n1.y, n1.z, uv1.x, uv1.y });
            vBuffer.insert(vBuffer.end(), { v2.x, v2.y, v2.z, color.r, color.g, color.b, n2.x, n2.y, n2.z, uv2.x, uv2.y });

            // Segundo triângulo
            vBuffer.insert(vBuffer.end(), { v1.x, v1.y, v1.z, color.r, color.g, color.b, n1.x, n1.y, n1.z, uv1.x, uv1.y });
            vBuffer.insert(vBuffer.end(), { v3.x, v3.y, v3.z, color.r, color.g, color.b, n3.x, n3.y, n3.z, uv3.x, uv3.y });
            vBuffer.insert(vBuffer.end(), { v2.x, v2.y, v2.z, color.r, color.g, color.b, n2.x, n2.y, n2.z, uv2.x, uv2.y });
        }
    }

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);

    // Layout da posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(0));
    glEnableVertexAttribArray(0);

    // Layout da cor
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Layout da normal
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    // Layout da textura
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 11 * sizeof(GLfloat), (GLvoid*)(9 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
    nVertices = vBuffer.size() / 11;

    return VAO;
}