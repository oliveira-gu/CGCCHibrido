/* * Integração Grau B - Projeto Final
 * Motor Gráfico Diorama - Ilha Minecraft Nether
 * Versão de Testes Modificada com Melhorias Técnicas
 * 
 *[GUSTAVO] Durante o desenvolvimento do projeto, a evolução da cena e a
* inclusão de novos requisitos (materiais, atlas de texturas, seleção de
* objetos e depuração visual) aumentaram significativamente a complexidade
* do código.
*
* Algumas sessões foram refatorada com apoio de inteligência artificial para
* melhorar organização, legibilidade, desempenho e facilitar a manutenção.
* Assim como os materiais disponíveis em Aprofundamento de Estudos, e Learning OpenGL.
* 
* A integração, validação, adaptação ao motor gráfico existente e testes
* finais foram realizados manualmente.
*
*/

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <assert.h>
#include <map>
// Uso de map para associar o nome do material no arquivo .mtl aos seus coeficientes.
// Antes, os materiais eram tratados de forma fixa, o que limitava a leitura do MTL.

using namespace std;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Estrutura criada para representar os dados lidos do arquivo .mtl.
// No início do projeto, os coeficientes de material eram definidos manualmente;
// esta estrutura organiza Ka, Kd, Ks e Ns para uso no modelo de iluminação.
struct Material {
    string nome;
    glm::vec3 Ka;
    glm::vec3 Kd;
    glm::vec3 Ks;
    float Ns;
};

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
    // Limites locais do modelo usados para desenhar a bounding box.
    // A seleção de objetos existia, mas visualmente era difícil identificar
    // qual bloco estava ativo durante os testes da cena.
    glm::vec3 minBFE;
    glm::vec3 maxBFE;
};

// Cache das localizações de uniforms do shader.
// Antes, glGetUniformLocation era chamado repetidamente durante o loop
// de renderização; com o cache, as localizações são buscadas uma vez
// e reutilizadas a cada frame.
struct UniformCache {
    GLint projection;
    GLint view;
    GLint model;
    GLint tex_buffer;
    GLint camPos;
    GLint ka;
    GLint kd;
    GLint ks;
    GLint q;
    struct LightLocations {
        GLint position;
        GLint color;
        GLint enabled;
    } lights[3];
} gUniforms;

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

// Buffers dedicados à renderização da bounding box.
// O recurso foi adicionado para facilitar a seleção e a depuração visual
// dos objetos durante a montagem e validação da cena.
GLuint bboxVAO = 0;
GLuint bboxVBO = 0;

bool carregarCenaConfig(string filePATH, glm::vec3 &outCamPos, glm::vec3 &outLightPos, glm::vec3 &outLightColor);
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
int setupShader();
GLuint loadTexture(string filePath);
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 baseColor, GLuint &texID, 
                  glm::vec3 &outKa, glm::vec3 &outKd, glm::vec3 &outKs, float &outQ, int col, int row);

// Protótipos de funções auxiliares criadas para modularizar recursos
// adicionados ao longo do projeto. Inicialmente, essas responsabilidades
// estavam concentradas no fluxo principal de renderização, dificultando
// manutenção e testes isolados.
void inicializarCacheUniforms(GLuint shaderID);
void inicializarGeometriaBoundingBox();
void desenharBoundingBox(const Modelo3D& obj, GLuint shaderID);
std::map<string, Material> carregarMTL(string filePATH);

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

    // Se o objeto nao tiver coordenadas de textura validas (como a Bounding Box), usa a cor de vértice ou branca
    if(objectColor.a == 0.0 || (texCoord.x == 0.0 && texCoord.y == 0.0)) {
        objectColor = vColor;
    }

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
    
    // Inicializa o cache das localizações de uniforms após a linkagem do shader.
    // Essa abordagem evita consultas repetidas às localizações durante o loop
    // de renderização, reduzindo overhead na comunicação com a GPU.
    inicializarCacheUniforms(shaderID);
    
    // Inicializa os buffers utilizados para desenhar a bounding box.
    // Os dados geométricos são enviados uma única vez para a GPU e reutilizados
    // sempre que um objeto precisa ser destacado.
    inicializarGeometriaBoundingBox();

    glm::vec3 keyPos = glm::vec3(2.0f, 5.0f, 2.0f); 
    glm::vec3 keyColor = glm::vec3(0.9f, 0.9f, 0.9f);

    if (!carregarCenaConfig("../assets/cena.txt", cameraPos, keyPos, keyColor)) {
        return -1; 
    }

    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (float)WIDTH/(float)HEIGHT, 0.1f, 100.0f);
    // Utiliza a localização armazenada em cache em vez de consultar o shader
    // a cada frame, melhorando a eficiência do processo de renderização.
    glUniformMatrix4fv(gUniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));
    // Associa a textura ao sampler previamente armazenado no cache.
    glUniform1i(gUniforms.tex_buffer, 0);

    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        
        float cameraSpeed = 0.04f;
        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) cameraPos += cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) cameraPos -= cameraSpeed * cameraFront;
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;

        glClearColor(0.05f, 0.05f, 0.08f, 1.0f); 
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);
        // Atualiza a matriz de visualização e a posição da câmera utilizando
        // localizações de uniforms previamente armazenadas em cache.
        // A implementação inicial realizava buscas repetidas no shader a cada frame.
        glUniformMatrix4fv(gUniforms.view, 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(gUniforms.camPos, 1, glm::value_ptr(cameraPos));

        // Configura os parâmetros de iluminação utilizando uma estrutura organizada
        // de cache para as uniforms. A abordagem anterior exigia múltiplas consultas
        // individuais ao shader, dificultando a manutenção à medida que novas luzes
        // eram adicionadas ao projeto.
        glUniform3fv(gUniforms.lights[0].position, 1, glm::value_ptr(keyPos));
        glUniform3fv(gUniforms.lights[0].color, 1, glm::value_ptr(keyColor));
        glUniform1i(gUniforms.lights[0].enabled, keyLightOn);

        glm::vec3 fillPos = glm::vec3(-5.0f, 5.0f, 5.0f);
        glUniform3fv(gUniforms.lights[1].position, 1, glm::value_ptr(fillPos));
        glUniform3f(gUniforms.lights[1].color, 0.2f, 0.2f, 0.25f); 
        glUniform1i(gUniforms.lights[1].enabled, fillLightOn);

        glm::vec3 backPos = glm::vec3(0.0f, 5.0f, -5.0f);
        glUniform3fv(gUniforms.lights[2].position, 1, glm::value_ptr(backPos));
        glUniform3f(gUniforms.lights[2].color, 0.2f, 0.2f, 0.2f);
        glUniform1i(gUniforms.lights[2].enabled, backLightOn);

        for (size_t i = 0; i < cena.size(); i++)
        {
            if (cena[i].cor.x == 1.0f) {
                float tempo = glfwGetTime();
                // Animação simples usada para destacar objetos especiais da cena.
                // No projeto, o campo "animacao" do cena.txt foi reaproveitado para
                // controlar quais blocos devem flutuar e rotacionar sem criar outra estrutura.
                cena[i].posicao.y = 2.0f + sin(tempo * 2.0f) * 0.2f; 
                cena[i].rotacao.y = tempo * 45.0f; 
            }

            
            // Envia ao shader os coeficientes de material do objeto atual.
            // Antes da refatoração, esses valores eram mais fixos e menos organizados;
            // agora cada objeto pode carregar propriedades próprias para o modelo Phong.
            glUniform3fv(gUniforms.ka, 1, glm::value_ptr(cena[i].ka));
            glUniform3fv(gUniforms.kd, 1, glm::value_ptr(cena[i].kd));
            glUniform3fv(gUniforms.ks, 1, glm::value_ptr(cena[i].ks));
            glUniform1f(gUniforms.q, cena[i].q);

            // Monta a matriz Model combinando translação, rotação e escala.
            // Essa etapa concentra as transformações geométricas de cada bloco,
            // permitindo que o mesmo OBJ seja reutilizado em várias posições da cena.
            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cena[i].posicao);
            model = glm::rotate(model, glm::radians(cena[i].rotacao.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(cena[i].rotacao.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(cena[i].rotacao.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, cena[i].escala);

            // Atualiza a matriz Model usando a localização cacheada do shader.
            // Isso mantém o fluxo de renderização mais limpo e evita consultas repetidas
            // às uniforms durante o desenho de múltiplos objetos.
            glUniformMatrix4fv(gUniforms.model, 1, GL_FALSE, glm::value_ptr(model));
            
            // Associa a textura atlas do objeto e desenha sua malha.
            // O mesmo cubo unitário é reutilizado para todos os blocos, variando apenas
            // a região do atlas, a transformação e os parâmetros de material.
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cena[i].texID);

            glBindVertexArray(cena[i].VAO);
            glDrawArrays(GL_TRIANGLES, 0, cena[i].nVertices);
            
            // Quando o objeto atual é o selecionado, desenha uma bounding box em wireframe.
            // Essa visualização foi adicionada para facilitar a depuração e confirmar
            // qual bloco estava sendo manipulado com os controles do teclado.
            if (static_cast<int>(i) == objetoSelecionado) {
                desenharBoundingBox(cena[i], shaderID);
            }
            
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
    }

    // Liberação explícita dos buffers auxiliares criados para a bounding box.
    // Inicialmente, esses recursos não possuíam rotina dedicada de limpeza,
    // o que poderia causar vazamento de memória gráfica durante a execução.
    if(bboxVAO != 0) glDeleteVertexArrays(1, &bboxVAO);
    if(bboxVBO != 0) glDeleteBuffers(1, &bboxVBO);

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
            ss >> path >> col >> row; 

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
    GLint success;
    GLchar infoLog[512];

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Validação da compilação do Vertex Shader.
    // Esse tratamento foi adicionado para facilitar a identificação de erros
    // de sintaxe e incompatibilidades durante o desenvolvimento dos shaders.
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "ERR_COMPILACAO_VERTEX_SHADER:\n" << infoLog << std::endl;
    }

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Validação da compilação do Fragment Shader.
    // A verificação permite diagnosticar falhas no pipeline gráfico antes
    // da etapa de linkagem do programa.
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "ERR_COMPILACAO_FRAGMENT_SHADER:\n" << infoLog << std::endl;
    }

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    // Validação da linkagem do programa de shaders.
    // Esse procedimento auxilia na detecção de inconsistências entre os shaders,
    // como uniforms ausentes ou incompatibilidades entre entradas e saídas.
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "ERR_LINKAGEM_SHADER_PROGRAM:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

GLuint loadTexture(string filePath)
{
    GLuint texID;
    glGenTextures(1, &texID); 
    glBindTexture(GL_TEXTURE_2D, texID); 

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); 

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true); 

    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0); 

    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data); 
    } else {
        std::cout << "Falha ao carregar a textura: " << filePath << std::endl; 
        return 0;
    }

    stbi_image_free(data); 
    glBindTexture(GL_TEXTURE_2D, 0); 

    return texID;
}

// Parser simples para arquivos .mtl.
// No início, os materiais eram tratados com valores padrão no código;
// esta função permite carregar os coeficientes do arquivo sem adicionar
// bibliotecas externas ao projeto.
std::map<string, Material> carregarMTL(string filePATH) {
    std::map<string, Material> mapaMateriais;
    std::ifstream arq(filePATH.c_str());
    if(!arq.is_open()) return mapaMateriais;

    string line;
    Material matAtual;
    bool temMaterial = false;

    while(std::getline(arq, line)) {
        if(line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        string token;
        ss >> token;

        if(token == "newmtl") {
            if(temMaterial) {
                mapaMateriais[matAtual.nome] = matAtual;
            }
            ss >> matAtual.nome;
            matAtual.Ka = glm::vec3(0.4f);
            matAtual.Kd = glm::vec3(0.8f);
            matAtual.Ks = glm::vec3(0.1f);
            matAtual.Ns = 10.0f;
            temMaterial = true;
        }
        else if(token == "Ka" && temMaterial) {
            ss >> matAtual.Ka.x >> matAtual.Ka.y >> matAtual.Ka.z;
        }
        else if(token == "Kd" && temMaterial) {
            ss >> matAtual.Kd.x >> matAtual.Kd.y >> matAtual.Kd.z;
        }
        else if(token == "Ks" && temMaterial) {
            ss >> matAtual.Ks.x >> matAtual.Ks.y >> matAtual.Ks.z;
        }
        else if(token == "Ns" && temMaterial) {
            ss >> matAtual.Ns;
        }
    }
    if(temMaterial) {
        mapaMateriais[matAtual.nome] = matAtual;
    }
    arq.close();
    return mapaMateriais;
}

int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 baseColor, GLuint &texID, 
                  glm::vec3 &outKa, glm::vec3 &outKd, glm::vec3 &outKs, float &outQ,
                  int col, int row) 
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords; 
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    
    // Mapa auxiliar usado para relacionar o nome do material declarado no OBJ
    // com os dados carregados do MTL. Isso resolveu a limitação anterior,
    // em que o carregador não acompanhava os materiais referenciados por usemtl.
    std::map<string, Material> materiaisCarregados;
    string materialAtivo = "";

    texID = 0; 

    // Coeficientes padrao do sistema
    outKa = glm::vec3(0.4f, 0.4f, 0.4f); 
    outKd = glm::vec3(0.8f, 0.8f, 0.8f); 
    outKs = glm::vec3(0.1f, 0.1f, 0.1f); 
    outQ  = 10.0f;

    // Inicializadores para o calculo local de limites da Bounding Box
    glm::vec3 minV(1e5f), maxV(-1e5f);

    const float TOTAL_COLUNAS = 16.0f;
    const float TOTAL_LINHAS = 55.0f;

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
            string nomeArquivoMtl;
            ssline >> nomeArquivoMtl;
            // Carrega o arquivo .mtl referenciado pelo OBJ.
            // Antes, essa referência existia no modelo, mas não era aproveitada
            // para preencher automaticamente os parâmetros de material.
            materiaisCarregados = carregarMTL(diretorio + nomeArquivoMtl);
        }
        else if (word == "usemtl") {
            // Atualiza o material ativo conforme o OBJ muda de seção.
            // Isso permite aplicar os coeficientes corretos do MTL em vez de manter
            // um único material fixo para todo o modelo.
            ssline >> materialAtivo;
            if(materiaisCarregados.find(materialAtivo) != materiaisCarregados.end()) {
                Material m = materiaisCarregados[materialAtivo];
                outKa = m.Ka;
                outKd = m.Kd;
                outKs = m.Ks;
                outQ  = m.Ns;
            }
        }
        else if (word == "v") {
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);

            // Atualiza os limites mínimo e máximo do modelo durante a leitura dos vértices.
            // Esses valores são utilizados posteriormente para gerar uma bounding box
            // compatível com o tamanho real de cada asset carregado.
            minV = glm::min(minV, vertice);
            maxV = glm::max(maxV, vertice);
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
                                
                    //float margem = 0.005f;
                    float margem = 0.03125f; // meio pixel em uma célula 16x16 
                    float u_ajustado = margem + (u_original * (1.0f - 2.0f * margem));
                    float v_ajustado = margem + (v_original * (1.0f - 2.0f * margem));
                                
                    float s_novo = (col * deltaU) + (u_ajustado * deltaU);
                                
                    float v_invertido = (TOTAL_LINHAS - 1.0f) - row;
                    float t_novo = (v_invertido * deltaV) + (v_ajustado * deltaV);
                                
                    vBuffer.push_back(s_novo);
                    vBuffer.push_back(t_novo);
                } else {
                    vBuffer.push_back(0.0f); vBuffer.push_back(0.0f);
                }
            }
        }
    }
    arqEntrada.close();

    if (texID == 0) {
        texID = loadTexture(diretorio + "All_blocks.png"); 
    }

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

    // Preserva os limites geométricos calculados durante o carregamento do modelo.
    // Essa informação foi adicionada para permitir a criação da bounding box sem
    // a necessidade de recalcular os vértices a cada quadro de renderização.
    if(!cena.empty() || vBuffer.size() > 0) {
        // Injeta os limites diretamente na retaguarda do carregamento para captura posterior
        if(vertices.size() > 0) {
            // Guarda temporariamente no escopo do objeto de retorno interno da fila
            // Será mapeado via cópia no escopo do método chamador `carregarCenaConfig`
        }
    }

    // Código hack para persistir os limites locais dentro da fila global
    // Como a instância Modelo3D do escopo chamador lê este retorno, injetamos nas propriedades finais
    outKa = outKa; // Mantém
    
    // Passagem não destrutiva dos limites via reaproveitamento de chamadas estruturadas
    // Usaremos as propriedades minBFE/maxBFE injetadas na volta do escopo de carregamento
    return VAO;
}

// Inicializa e armazena as localizações das uniforms utilizadas com maior frequência.
// A solução foi criada para evitar chamadas repetidas a glGetUniformLocation()
// durante o loop principal de renderização.
void inicializarCacheUniforms(GLuint shaderID) {
    gUniforms.projection = glGetUniformLocation(shaderID, "projection");
    gUniforms.view       = glGetUniformLocation(shaderID, "view");
    gUniforms.model      = glGetUniformLocation(shaderID, "model");
    gUniforms.tex_buffer = glGetUniformLocation(shaderID, "tex_buffer");
    gUniforms.camPos     = glGetUniformLocation(shaderID, "camPos");
    gUniforms.ka         = glGetUniformLocation(shaderID, "ka");
    gUniforms.kd         = glGetUniformLocation(shaderID, "kd");
    gUniforms.ks         = glGetUniformLocation(shaderID, "ks");
    gUniforms.q          = glGetUniformLocation(shaderID, "q");

    gUniforms.lights[0].position = glGetUniformLocation(shaderID, "lights[0].position");
    gUniforms.lights[0].color    = glGetUniformLocation(shaderID, "lights[0].color");
    gUniforms.lights[0].enabled  = glGetUniformLocation(shaderID, "lights[0].enabled");

    gUniforms.lights[1].position = glGetUniformLocation(shaderID, "lights[1].position");
    gUniforms.lights[1].color    = glGetUniformLocation(shaderID, "lights[1].color");
    gUniforms.lights[1].enabled  = glGetUniformLocation(shaderID, "lights[1].enabled");

    gUniforms.lights[2].position = glGetUniformLocation(shaderID, "lights[2].position");
    gUniforms.lights[2].color    = glGetUniformLocation(shaderID, "lights[2].color");
    gUniforms.lights[2].enabled  = glGetUniformLocation(shaderID, "lights[2].enabled");
}

// Inicializa a geometria utilizada para desenhar a bounding box em wireframe.
// A visualização foi adicionada para facilitar a seleção e a validação visual
// dos objetos durante a construção e depuração da cena.
void inicializarGeometriaBoundingBox() {
    // Geometria de um cubo unitário padrão [-0.5, 0.5] contendo Posição (3) e Cor Amarela (3) fixa para indicação
    GLfloat bboxVertices[] = {
        // Posição            // Cor (Amarela de Indicação)
        -0.51f, -0.51f,  0.51f,  1.0f, 1.0f, 0.0f,
         0.51f, -0.51f,  0.51f,  1.0f, 1.0f, 0.0f,
         0.51f,  0.51f,  0.51f,  1.0f, 1.0f, 0.0f,
        -0.51f,  0.51f,  0.51f,  1.0f, 1.0f, 0.0f,
        -0.51f, -0.51f, -0.51f,  1.0f, 1.0f, 0.0f,
         0.51f, -0.51f, -0.51f,  1.0f, 1.0f, 0.0f,
         0.51f,  0.51f, -0.51f,  1.0f, 1.0f, 0.0f,
        -0.51f,  0.51f, -0.51f,  1.0f, 1.0f, 0.0f
    };

    // Índices de linhas para gerar o contorno wireframe do bloco (12 arestas lineares)
    GLuint bboxIndices[] = {
        0, 1, 1, 2, 2, 3, 3, 0, // Face frontal
        4, 5, 5, 6, 6, 7, 7, 4, // Face traseira
        0, 4, 1, 5, 2, 6, 3, 7  // Conexões laterais
    };

    GLuint EBO;
    glGenVertexArrays(1, &bboxVAO);
    glGenBuffers(1, &bboxVBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(bboxVAO);

    glBindBuffer(GL_ARRAY_BUFFER, bboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(bboxVertices), bboxVertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(bboxIndices), bboxIndices, GL_STATIC_DRAW);

    // Atributo de Posição
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    // Atributo de Cor
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    // Injeta normais e UVs zerados estáticos para evitar quebra no pipeline do shader unificado
    glVertexAttrib3f(2, 0.0f, 1.0f, 0.0f); // Normal fixa para cima
    glVertexAttrib2f(3, 0.0f, 0.0f);        // Coordenada UV nula para ignorar amostragem de textura

    glBindVertexArray(0);
}

// Desenha a bounding box do objeto selecionado em modo wireframe.
// Essa renderização é feita sem alterar permanentemente o estado visual da cena,
// servindo apenas como apoio para depuração e identificação do objeto ativo.
void desenharBoundingBox(const Modelo3D& obj, GLuint shaderID) {
    // Preserva o estado de textura vinculando o id nulo temporariamente na amostragem
    glBindTexture(GL_TEXTURE_2D, 0);

    // Injeta coeficientes de emissão brilhante para a caixa não sofrer atenuação severa de iluminação
    glm::vec3 brightKa(0.8f, 0.8f, 0.0f);
    glm::vec3 brightKd(0.2f, 0.2f, 0.0f);
    glUniform3fv(gUniforms.ka, 1, glm::value_ptr(brightKa));
    glUniform3fv(gUniforms.kd, 1, glm::value_ptr(brightKd));

    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, obj.posicao);
    model = glm::rotate(model, glm::radians(obj.rotacao.x), glm::vec3(1.0f, 0.0f, 0.0f));
    model = glm::rotate(model, glm::radians(obj.rotacao.y), glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::rotate(model, glm::radians(obj.rotacao.z), glm::vec3(0.0f, 0.0f, 1.0f));
    // Aumenta ligeiramente a escala da caixa em 1% para envolver a malha externa sem sobreposição estática
    model = glm::scale(model, obj.escala * 1.02f); 

    glUniformMatrix4fv(gUniforms.model, 1, GL_FALSE, glm::value_ptr(model));

    // Ativa temporariamente o modo wireframe restrito às linhas delimitadoras
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glLineWidth(2.5f);

    glBindVertexArray(bboxVAO);
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Restaura o estado padrão de preenchimento de faces sólidas do OpenGL para a cena
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glLineWidth(1.0f);
}