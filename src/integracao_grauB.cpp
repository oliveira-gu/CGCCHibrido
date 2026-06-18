/* * Código Integrado - Projeto Final Grau B
 * Motor Gráfico Diorama - Tema: Minecraft
 */

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <assert.h>

using namespace std;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Modelo3D {
    GLuint VAO;
    int nVertices;
    GLuint texID;
    glm::vec3 posicao;
    glm::vec3 rotacao; 
    glm::vec3 escala;
    glm::vec3 cor;
    glm::vec3 ka;
    glm::vec3 kd;
    glm::vec3 ks;
    float q;
};

const GLuint WIDTH = 1000, HEIGHT = 1000;
std::vector<Modelo3D> cena;
int objetoSelecionado = 0; 

float moveSpeed = 0.05f;
float rotateSpeed = 2.0f;
float scaleSpeed = 0.05f;

bool keyLightOn = true;   
bool fillLightOn = true;  
bool backLightOn = true;  

glm::vec3 cameraPos   = glm::vec3(0.0f, 2.0f, 5.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

float yaw   = -90.0f;
float pitch =  0.0f;
float lastX =  500.0f;
float lastY =  500.0f;
bool firstMouse = true;

bool carregarCenaConfig(string filePATH, glm::vec3 &outCamPos, glm::vec3 &outLightPos, glm::vec3 &outLightColor);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
int setupShader();
GLuint loadTexture(string filePath);
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 baseColor, GLuint &texID, 
                  glm::vec3 &outKa, glm::vec3 &outKd, glm::vec3 &outKs, float &outQ, int col, int row);

const GLchar* vertexShaderSource = R"(
#version 410
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 tex_coord;

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
    
    texCoord = vec2(tex_coord.x, tex_coord.y);
    vColor = vec4(color, 1.0);
}
)";

const GLchar* fragmentShaderSource = R"(
#version 410
in vec2 texCoord;
in vec4 fragPos;
in vec3 vNormal;
in vec4 vColor;

out vec4 color;

uniform sampler2D tex_buffer;
uniform vec3 camPos;

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float q;

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
    
    vec4 objectColor = texture(tex_buffer, texCoord);

    vec3 finalLight = vec3(0.0);

    for(int i = 0; i < 3; i++) {
        if(!lights[i].enabled) continue;

        vec3 L = normalize(lights[i].position - vec3(fragPos));
        float dist = length(lights[i].position - vec3(fragPos));
        
        float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * (dist * dist));

        vec3 ambient = ka * lights[i].color;

        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = kd * diff * lights[i].color;

        vec3 R = normalize(reflect(-L, N));
        float spec = max(dot(R, V), 0.0);
        spec = pow(spec, max(q, 1.0));
        vec3 specular = ks * spec * lights[i].color;

        finalLight += (ambient + (diffuse + specular) * attenuation) * vec3(objectColor);
    }

    color = vec4(finalLight, 1.0);
}
)";

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Gustavo de Oliveira - Universo Minecraft", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); 

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);
    
    glm::vec3 keyPos = glm::vec3(2.0f, 5.0f, 2.0f); 
    glm::vec3 keyColor = glm::vec3(0.9f, 0.9f, 0.9f);

    if (!carregarCenaConfig("../assets/cena.txt", cameraPos, keyPos, keyColor)) {
        return -1; 
    }

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH/(float)HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(glGetUniformLocation(shaderID, "tex_buffer"), 0);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        float cameraSpeed = 0.04f;
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) cameraPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f); // Fundo levemente azulado (Céu noturno)
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shaderID, "camPos"), 1, glm::value_ptr(cameraPos));

        // Luz Principal (Sol/Lua vindo do arquivo de configuração)
        glUniform3fv(glGetUniformLocation(shaderID, "lights[0].position"), 1, glm::value_ptr(keyPos));
        glUniform3fv(glGetUniformLocation(shaderID, "lights[0].color"), 1, glm::value_ptr(keyColor));
        glUniform1i(glGetUniformLocation(shaderID, "lights[0].enabled"), keyLightOn);

        // Luzes Secundárias de Preenchimento Estáticas
        glm::vec3 fillPos = glm::vec3(-5.0f, 5.0f, 5.0f);
        glUniform3fv(glGetUniformLocation(shaderID, "lights[1].position"), 1, glm::value_ptr(fillPos));
        glUniform3f(glGetUniformLocation(shaderID, "lights[1].color"), 0.2f, 0.2f, 0.25f); 
        glUniform1i(glGetUniformLocation(shaderID, "lights[1].enabled"), fillLightOn);

        glm::vec3 backPos = glm::vec3(0.0f, 5.0f, -5.0f);
        glUniform3fv(glGetUniformLocation(shaderID, "lights[2].position"), 1, glm::value_ptr(backPos));
        glUniform3f(glGetUniformLocation(shaderID, "lights[2].color"), 0.2f, 0.2f, 0.2f);
        glUniform1i(glGetUniformLocation(shaderID, "lights[2].enabled"), backLightOn);

        for (size_t i = 0; i < cena.size(); i++)
        {
            // Curva paramétrica: se ativo, faz o bloco flutuar suavemente no ar (Item coletável)
            if (cena[i].cor.x == 1.0f) {
                float tempo = glfwGetTime();
                cena[i].posicao.y = 2.0f + sin(tempo * 2.0f) * 0.2f; 
                cena[i].rotacao.y = tempo * 45.0f; // Adiciona uma rotação charmosa de item flutuante
            }

            glUniform3fv(glGetUniformLocation(shaderID, "ka"), 1, glm::value_ptr(cena[i].ka));
            glUniform3fv(glGetUniformLocation(shaderID, "kd"), 1, glm::value_ptr(cena[i].kd));
            glUniform3fv(glGetUniformLocation(shaderID, "ks"), 1, glm::value_ptr(cena[i].ks));
            glUniform1f(glGetUniformLocation(shaderID, "q"), cena[i].q);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cena[i].posicao);
            model = glm::rotate(model, glm::radians(cena[i].rotacao.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(cena[i].rotacao.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(cena[i].rotacao.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, cena[i].escala);

            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cena[i].texID);

            glBindVertexArray(cena[i].VAO);
            glDrawArrays(GL_TRIANGLES, 0, cena[i].nVertices);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

bool carregarCenaConfig(string filePATH, glm::vec3 &outCamPos, glm::vec3 &outLightPos, glm::vec3 &outLightColor) {
    ifstream arq(filePATH);
    if (!arq.is_open()) {
        cout << "Erro ao abrir arquivo de configuracao: " << filePATH << endl;
        return false;
    }

    string line;
    while (getline(arq, line)) {
        if (line.empty() || line[0] == '#') continue; 

        istringstream ss(line);
        string tipo;
        ss >> tipo;

        if (tipo == "camera") { 
            ss >> outCamPos.x >> outCamPos.y >> outCamPos.z;
        }
        else if (tipo == "luz") { 
            ss >> outLightPos.x >> outLightPos.y >> outLightPos.z;
            ss >> outLightColor.r >> outLightColor.g >> outLightColor.b;
        }
        else if (tipo == "objeto") { 
            string path;
            int col, row;
            ss >> path >> col >> row; // Lê a coluna e linha do bloco no Atlas

            Modelo3D obj;
            obj.VAO = loadSimpleOBJ(path, obj.nVertices, glm::vec3(1.0f), obj.texID, obj.ka, obj.kd, obj.ks, obj.q, col, row); 
            
            if (obj.VAO != -1) {
                ss >> obj.posicao.x >> obj.posicao.y >> obj.posicao.z;
                ss >> obj.rotacao.x >> obj.rotacao.y >> obj.rotacao.z;
                float esc;
                ss >> esc;
                obj.escala = glm::vec3(esc);
                
                float animado;
                ss >> animado;
                obj.cor = glm::vec3(animado, 0.0f, 0.0f); 

                cena.push_back(obj); 
            }
        }
    }
    arq.close();
    return true;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
    if (cena.empty()) return; 

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) objetoSelecionado = (objetoSelecionado + 1) % cena.size();

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) keyLightOn = !keyLightOn;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) fillLightOn = !fillLightOn;
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) backLightOn = !backLightOn;

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_A) cena[objetoSelecionado].posicao.x -= moveSpeed;
        if (key == GLFW_KEY_D) cena[objetoSelecionado].posicao.x += moveSpeed;
        if (key == GLFW_KEY_W) cena[objetoSelecionado].posicao.y += moveSpeed;
        if (key == GLFW_KEY_S) cena[objetoSelecionado].posicao.y -= moveSpeed;
        if (key == GLFW_KEY_Q) cena[objetoSelecionado].posicao.z += moveSpeed;
        if (key == GLFW_KEY_E) cena[objetoSelecionado].posicao.z -= moveSpeed;

        if (key == GLFW_KEY_UP)    cena[objetoSelecionado].rotacao.x -= rotateSpeed;
        if (key == GLFW_KEY_DOWN)  cena[objetoSelecionado].rotacao.x += rotateSpeed;
        if (key == GLFW_KEY_LEFT)  cena[objetoSelecionado].rotacao.y -= rotateSpeed;
        if (key == GLFW_KEY_RIGHT) cena[objetoSelecionado].rotacao.y += rotateSpeed;
        if (key == GLFW_KEY_Z)     cena[objetoSelecionado].rotacao.z -= rotateSpeed;
        if (key == GLFW_KEY_C)     cena[objetoSelecionado].rotacao.z += rotateSpeed;

        if (key == GLFW_KEY_LEFT_BRACKET) {
            cena[objetoSelecionado].escala -= glm::vec3(scaleSpeed);
            if (cena[objetoSelecionado].escala.x < 0.05f) cena[objetoSelecionado].escala = glm::vec3(0.05f);
        }
        if (key == GLFW_KEY_RIGHT_BRACKET) {
            cena[objetoSelecionado].escala += glm::vec3(scaleSpeed);
        }
    }
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos; lastY = ypos; firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos; lastY = ypos;

    float sensitivity = 0.05f; 
    xoffset *= sensitivity; yoffset *= sensitivity;

    yaw   += xoffset;
    pitch += yoffset;

    if (pitch > 89.0f)  pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    glm::vec3 front;
    front.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    front.y = sin(glm::radians(pitch));
    front.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    cameraFront = glm::normalize(front);
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

GLuint loadTexture(string filePath)
{
    GLuint texID;
    glGenTextures(1, &texID); 
    glBindTexture(GL_TEXTURE_2D, texID); 

    // Configurações ideais para Pixel Art do Minecraft (Sem borrar os blocos)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); // Minecraft costuma precisar de true para alinhar as coordenadas verticais

    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0); 

    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data); 
        glGenerateMipmap(GL_TEXTURE_2D); 
    } else {
        std::cout << "Falha ao carregar a textura: " << filePath << std::endl; 
        return 0;
    }

    stbi_image_free(data); 
    glBindTexture(GL_TEXTURE_2D, 0); 

    return texID;
}

// Alteração na assinatura para aceitar coluna e linha do bloco desejado no Atlas
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 baseColor, GLuint &texID, 
                  glm::vec3 &outKa, glm::vec3 &outKd, glm::vec3 &outKs, float &outQ,
                  int col, int row) // <-- Adicionado aqui
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords; 
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    
    texID = 0; 

    outKa = glm::vec3(0.4f, 0.4f, 0.4f); 
    outKd = glm::vec3(0.8f, 0.8f, 0.8f); 
    outKs = glm::vec3(0.1f, 0.1f, 0.1f); 
    outQ  = 10.0f;

    // Definição do tamanho da grade do seu All_blocks.png
    const float TOTAL_COLUNAS = 16.0f;
    const float TOTAL_LINHAS = 64.0f;

    // Largura e altura de cada bloco individual no espaço UV (0.0 a 1.0)
    float deltaU = 1.0f / TOTAL_COLUNAS;
    float deltaV = 1.0f / TOTAL_LINHAS;

    string diretorio = "";
    size_t lastSlash = filePATH.find_last_of('/');
    if (lastSlash != string::npos) diretorio = filePATH.substr(0, lastSlash + 1);

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open()) return -1;

    std::string line;
    while (std::getline(arqEntrada, line)) 
    {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "mtllib") {
            // ... (Mantém o seu código original de leitura do MTL intacto) ...
        }
        else if (word == "v") {
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        } else if (word == "vt") {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        } else if (word == "vn") {
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } else if (word == "f") {
            while (ssline >> word) {
                int vi = 0, ti = 0, ni = 0;
                std::istringstream ss(word);
                std::string index;

                if (std::getline(ss, index, '/')) vi = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index, '/')) ti = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index)) ni = !index.empty() ? std::stoi(index) - 1 : 0;

                if (vi < 0 || vi >= vertices.size()) vi = 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                
                vBuffer.push_back(baseColor.r);
                vBuffer.push_back(baseColor.g);
                vBuffer.push_back(baseColor.b);

                if (!normals.empty() && ni >= 0 && ni < normals.size()) {
                    vBuffer.push_back(normals[ni].x);
                    vBuffer.push_back(normals[ni].y);
                    vBuffer.push_back(normals[ni].z);
                } else {
                    vBuffer.push_back(0.0f); vBuffer.push_back(0.0f); vBuffer.push_back(1.0f);
                }
                
                if (!texCoords.empty() && ti >= 0 && ti < texCoords.size()) {
                    float u_original = texCoords[ti].s;
                    float v_original = texCoords[ti].t;
                                
                    if (u_original > 1.0f) u_original = 1.0f;
                    if (u_original < 0.0f) u_original = 0.0f;
                    if (v_original > 1.0f) v_original = 1.0f;
                    if (v_original < 0.0f) v_original = 0.0f;
                                
                    // --- RECUO DE SEGURANÇA PARA EVITAR VAZAMENTO (INSET) ---
                    // Aumentamos levemente para 1% (0.01f) se o sangramento de pixel persistir
                    float margem = 0.01f; 
                    float u_ajustado = margem + (u_original * (1.0f - 2.0f * margem));
                    float v_ajustado = margem + (v_original * (1.0f - 2.0f * margem));
                                
                    // Micro-deslocamento opcional para centralizar perfeitamente no pixel bruto (ajuste se necessário)
                    float epsilon = 0.000f; 
                                
                    // Ajuste horizontal (U) baseado no valor corrigido
                    float s_novo = (col * deltaU) + (u_ajustado * deltaU) + epsilon;
                                
                    // Ajuste vertical (V)
                    float v_invertido = (TOTAL_LINHAS - 1.0f) - row;
                    float t_novo = (v_invertido * deltaV) + (v_ajustado * deltaV) + epsilon;
                                
                    vBuffer.push_back(s_novo);
                    vBuffer.push_back(t_novo);
                } else {
                    vBuffer.push_back(0.0f); vBuffer.push_back(0.0f);
                }
            }
        }
    }
    arqEntrada.close();

    // Força o carregamento da imagem unificada caso o .mtl não a defina
    if (texID == 0) {
        texID = loadTexture(diretorio + "All_blocks.png"); // Ajustado para .png conforme seu arquivo
    }

    // ... (Mantém o restante do envio para o VBO/VAO original igual) ...
    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);
    
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    int stride = 11 * sizeof(GLfloat);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(9 * sizeof(GLfloat)));
    glEnableVertexAttribArray(3);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 11; 
    return VAO;
}