/* * Código Integrado - Atividade Vivencial 2 (Módulo 4)
 * Iluminação de 3 Pontos (Phong) com Atenuação.
 */

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <assert.h>

using namespace std;

// Instancia a biblioteca de imagens
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// GLAD
#include <glad/glad.h>
// GLFW
#include <GLFW/glfw3.h>
// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// --- ESTRUTURAS ---
struct Modelo3D {
    GLuint VAO;
    int nVertices;
    GLuint texID;
    glm::vec3 posicao;
    glm::vec3 rotacao; 
    glm::vec3 escala;
    glm::vec3 cor;
    // Propriedades de material vindas do .mtl [cite: 13, 15]
    glm::vec3 ka;
    glm::vec3 kd;
    glm::vec3 ks;
    float q;
};

// --- VARIÁVEIS GLOBAIS ---
const GLuint WIDTH = 1000, HEIGHT = 1000;
std::vector<Modelo3D> cena;
int objetoSelecionado = 0; 

// Velocidades de transformação
float moveSpeed = 0.05f;
float rotateSpeed = 2.0f;
float scaleSpeed = 0.05f;

// Controle das Luzes (Vivencial 2)
bool keyLightOn = true;   // Tecla 1
bool fillLightOn = true;  // Tecla 2
bool backLightOn = true;  // Tecla 3

// Câmera Sintética Global [cite: 17, 31]
glm::vec3 cameraPos   = glm::vec3(0.0f, 0.0f, 3.0f);
glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
glm::vec3 cameraUp    = glm::vec3(0.0f, 1.0f, 0.0f);

// Variáveis para o mouse [cite: 17]
float yaw   = -90.0f;
float pitch =  0.0f;
float lastX =  500.0f;
float lastY =  500.0f;
bool firstMouse = true;

// --- DECLARAÇÕES DE FUNÇÕES (CORRIGIDAS) ---
bool carregarCenaConfig(string filePATH, glm::vec3 &outCamPos, glm::vec3 &outLightPos, glm::vec3 &outLightColor);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
int setupShader();
GLuint loadTexture(string filePath);
// Adicionadas as referências de materiais na assinatura da função:
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 baseColor, GLuint &texID, 
                  glm::vec3 &outKa, glm::vec3 &outKd, glm::vec3 &outKs, float &outQ);

// --- SHADERS DE ILUMINAÇÃO (PHONG + 3 PONTOS + ATENUAÇÃO) ---
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

// Propriedades do Material
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
    if(objectColor.a < 0.1) objectColor = vColor; 

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

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Gustavo de Oliveira - Projeto Final Grau B", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);
    
    // Configura o callback do mouse para a câmera navegar [cite: 17]
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED); // Oculta e trava o mouse na tela

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);
    
    // --- CARREGANDO CONFIGURAÇÃO DA CENA ---
    glm::vec3 keyPos = glm::vec3(2.0f, 2.0f, 2.0f); 
    glm::vec3 keyColor = glm::vec3(0.8f, 0.8f, 0.8f);

    // Carrega a cena do .txt (Preenche a cameraPos, luz e o vetor de objetos) [cite: 25]
    if (!carregarCenaConfig("../assets/cena.txt", cameraPos, keyPos, keyColor)) {
        return -1; 
    }

    // Matriz de Projeção Dinâmica [cite: 31]
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH/(float)HEIGHT, 0.1f, 100.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
    glUniform1i(glGetUniformLocation(shaderID, "tex_buffer"), 0);

    // --- LOOP DE RENDERIZAÇÃO ---
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        // Movimentação fluida da câmera via Teclado (I, K, J, L) para não dar conflito [cite: 17]
        float cameraSpeed = 0.03f;
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) cameraPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        std::cout << "Quantidade de objetos na cena: " << cena.size() << std::endl;

        // Atualiza matriz de visualização (View) baseada na câmera sintética móvel [cite: 17, 31]
        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shaderID, "camPos"), 1, glm::value_ptr(cameraPos));

        // --- LUZES DE 3 PONTOS ---
        // 1. Key Light (Vinda do arquivo de configuração) [cite: 30]
        glUniform3fv(glGetUniformLocation(shaderID, "lights[0].position"), 1, glm::value_ptr(keyPos));
        glUniform3fv(glGetUniformLocation(shaderID, "lights[0].color"), 1, glm::value_ptr(keyColor));
        glUniform1i(glGetUniformLocation(shaderID, "lights[0].enabled"), keyLightOn);

        // 2. Fill Light
        glm::vec3 fillPos = glm::vec3(-2.0f, 1.0f, 2.0f);
        glUniform3fv(glGetUniformLocation(shaderID, "lights[1].position"), 1, glm::value_ptr(fillPos));
        glUniform3f(glGetUniformLocation(shaderID, "lights[1].color"), 0.3f, 0.3f, 0.4f); 
        glUniform1i(glGetUniformLocation(shaderID, "lights[1].enabled"), fillLightOn);

        // 3. Back Light
        glm::vec3 backPos = glm::vec3(0.0f, 2.0f, -3.0f);
        glUniform3fv(glGetUniformLocation(shaderID, "lights[2].position"), 1, glm::value_ptr(backPos));
        glUniform3f(glGetUniformLocation(shaderID, "lights[2].color"), 0.6f, 0.6f, 0.6f);
        glUniform1i(glGetUniformLocation(shaderID, "lights[2].enabled"), backLightOn);

        // Renderiza cada objeto da cena dinamicamente [cite: 8]
        for (size_t i = 0; i < cena.size(); i++)
        {
            // Curvas paramétricas / Animação em tempo real se flag ativada [cite: 29]
            if (cena[i].cor.x == 1.0f) {
                float tempo = glfwGetTime();
                cena[i].posicao.y = sin(tempo) * 0.5f; // Oscilação paramétrica senoidal
            }

            // Envia as propriedades específicas de iluminação do material deste objeto (.mtl) [cite: 13, 15]
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

// --- LER ARQUIVO DE CONFIGURAÇÃO DA CENA ---
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

        if (tipo == "camera") { // [cite: 31]
            ss >> outCamPos.x >> outCamPos.y >> outCamPos.z;
        }
        else if (tipo == "luz") { // [cite: 30]
            ss >> outLightPos.x >> outLightPos.y >> outLightPos.z;
            ss >> outLightColor.r >> outLightColor.g >> outLightColor.b;
        }
        else if (tipo == "objeto") { // [cite: 26]
            string path;
            ss >> path; // [cite: 27]

            Modelo3D obj;
            // Carrega injetando as variáveis internas de material para serem salvas
            obj.VAO = loadSimpleOBJ(path, obj.nVertices, glm::vec3(1.0f), obj.texID, obj.ka, obj.kd, obj.ks, obj.q); 
            
            if (obj.VAO != -1) {
                // Lê as transformações iniciais do arquivo txt [cite: 28]
                ss >> obj.posicao.x >> obj.posicao.y >> obj.posicao.z;
                ss >> obj.rotacao.x >> obj.rotacao.y >> obj.rotacao.z;
                float esc;
                ss >> esc;
                obj.escala = glm::vec3(esc);
                
                float animado;
                ss >> animado;
                obj.cor = glm::vec3(animado, 0.0f, 0.0f); // 1.0 se animado, 0.0 se estático [cite: 29]

                cena.push_back(obj); 
            }
        }
    }
    arq.close();
    return true;
}

// --- CONTROLES DO TECLADO ---
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);

    if (cena.empty()) return; // Prevenção contra crash se a cena estiver vazia

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) objetoSelecionado = (objetoSelecionado + 1) % cena.size();

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) keyLightOn = !keyLightOn;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) fillLightOn = !fillLightOn;
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) backLightOn = !backLightOn;

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        // Movimentação do objeto selecionado via Teclado [cite: 18]
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

// --- CONTROLE DE MOUSE (CÂMERA SINTÉTICA YAW/PITCH) ---
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    if (firstMouse) {
        lastX = xpos; lastY = ypos; firstMouse = false;
    }
    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; 
    lastX = xpos; lastY = ypos;

    float sensitivity = 0.05f; // Ajustada levemente para suavidade
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

// --- CONFIGURAÇÃO DE SHADERS ---
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

// --- CARREGAMENTO DE TEXTURA ---
GLuint loadTexture(string filePath)
{
    GLuint texID;
    glGenTextures(1, &texID); 
    glBindTexture(GL_TEXTURE_2D, texID); 

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);

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

// --- LEITOR DE OBJETO (CORRIGIDO E ATUALIZADO) ---
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 baseColor, GLuint &texID, 
                  glm::vec3 &outKa, glm::vec3 &outKd, glm::vec3 &outKs, float &outQ)
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords; 
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    
    texID = 0; 

    // Valores padrão caso não existam no .mtl [cite: 13]
    outKa = glm::vec3(0.2f);
    outKd = glm::vec3(0.8f);
    outKs = glm::vec3(0.5f);
    outQ  = 32.0f;

    string diretorio = "";
    size_t lastSlash = filePATH.find_last_of('/');
    if (lastSlash != string::npos) diretorio = filePATH.substr(0, lastSlash + 1);

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open()) return -1;

    std::string line;
    while (std::getline(arqEntrada, line)) 
    {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "mtllib") {
            string mtlFile;
            ssline >> mtlFile;
            std::ifstream mtlArq((diretorio + mtlFile).c_str());
            if (mtlArq.is_open()) {
                string mLine, mWord;
                while (getline(mtlArq, mLine)) {
                    std::istringstream mss(mLine);
                    mss >> mWord;

                    // Mapeamento correto dos identificadores do .mtl para as referências de saída [cite: 13, 15]
                    if (mWord == "Ka" || mWord == "ka") {
                        mss >> outKa.x >> outKa.y >> outKa.z;
                    } 
                    else if (mWord == "Kd" || mWord == "kd") {
                        mss >> outKd.x >> outKd.y >> outKd.z;
                    } 
                    else if (mWord == "Ks" || mWord == "ks") {
                        mss >> outKs.x >> outKs.y >> outKs.z;
                    } 
                    else if (mWord == "Ns" || mWord == "ns") {
                        mss >> outQ; 
                    }
                    else if (mWord == "map_Kd") { 
                        string texName;
                        mss >> texName;
                        texID = loadTexture(diretorio + texName); 
                    }
                }
                mtlArq.close();
            }
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

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                
                vBuffer.push_back(baseColor.r);
                vBuffer.push_back(baseColor.g);
                vBuffer.push_back(baseColor.b);

                if (!normals.empty()) {
                    vBuffer.push_back(normals[ni].x);
                    vBuffer.push_back(normals[ni].y);
                    vBuffer.push_back(normals[ni].z);
                } else {
                    vBuffer.push_back(0.0f); vBuffer.push_back(0.0f); vBuffer.push_back(1.0f);
                }
                
                if (!texCoords.empty() && ti >= 0) {
                    vBuffer.push_back(texCoords[ti].s);
                    vBuffer.push_back(texCoords[ti].t);
                } else {
                    vBuffer.push_back(0.0f); vBuffer.push_back(0.0f);
                }
            }
        }
    }
    arqEntrada.close();

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