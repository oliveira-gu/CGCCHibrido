# Motor Gráfico Diorama – Universo Minecraft (OpenGL + C++)

**Desenvolvedor:** Gustavo de Oliveira
**Disciplina:** Computação Gráfica – Unisinos
**Contexto:** Projeto Final Integrado – Grau B

Este projeto consiste em um motor gráfico interativo desenvolvido em C++ e OpenGL (Core Profile 4.1). O objetivo é renderizar um diorama tridimensional inspirado no universo de Minecraft, demonstrando conceitos fundamentais do pipeline gráfico programável, incluindo transformações geométricas, carregamento de modelos 3D, mapeamento de texturas via Texture Atlas e iluminação baseada no modelo de Phong.

A cena representa uma ilha flutuante inspirada no Nether, composta por blocos configuráveis por meio de um arquivo externo de descrição da cena.

---

## Demonstração

![Demonstração do diorama final](image.png)

---

## Funcionalidades

* Navegação livre por câmera em primeira pessoa.
* Controle de orientação por mouse (*Yaw* e *Pitch*).
* Seleção interativa de objetos.
* Transformações geométricas em tempo real:

  * Translação;
  * Rotação;
  * Escala uniforme.
* Sistema de iluminação de três pontos (*Key Light*, *Fill Light* e *Back Light*).
* Modelo de iluminação de Phong com componentes ambiente, difusa e especular.
* Carregamento de modelos OBJ e materiais MTL.
* Mapeamento de texturas utilizando Texture Atlas.
* Arquivo externo de configuração da cena (`cena.txt`).
* Animações paramétricas baseadas no tempo.
* Visualização de *Bounding Box* para depuração.

---

## Setup

### Requisitos

* Compilador com suporte a C++17 ou superior.
* OpenGL 4.1 ou superior.
* CMake 3.15 ou superior.
* Git.

### Dependências

O projeto utiliza as seguintes bibliotecas:

* GLFW
* GLAD
* GLM
* stb_image
* OpenGL 4.1 Core Profile

### Estrutura esperada do projeto

```text
projeto/
├── assets/
│   ├── Modelos3D/
│   │   ├── Grass_block.obj
│   │   ├── Grass_block.mtl
│   │   └── All_blocks.png
│   └── cena.txt
├── src/
├── CMakeLists.txt
└── README.md
```

### Compilação

Clone o repositório:

```bash
git clone <url-do-repositorio>
cd <nome-do-repositorio>
```

Crie a pasta de compilação:

```bash
mkdir build
cd build
```

Configure o projeto:

```bash
cmake ..
```

Compile:

```bash
make
```

Execute:

```bash
./nome_do_executavel
```

> **Observação:** os caminhos relativos dos assets devem ser preservados para garantir o carregamento correto dos modelos, texturas e do arquivo de configuração da cena.

---

## Controles

### Navegação da câmera

| Tecla | Ação                        |
| ----- | --------------------------- |
| `I`   | Avançar                     |
| `K`   | Recuar                      |
| `J`   | Mover para a esquerda       |
| `L`   | Mover para a direita        |
| Mouse | Controlar direção da câmera |

### Seleção e transformação de objetos

| Tecla     | Ação                        |
| --------- | --------------------------- |
| `TAB`     | Alternar objeto selecionado |
| `W` / `S` | Translação no eixo Y        |
| `A` / `D` | Translação no eixo X        |
| `Q` / `E` | Translação no eixo Z        |
| `↑` / `↓` | Rotação no eixo X           |
| `←` / `→` | Rotação no eixo Y           |
| `Z` / `C` | Rotação no eixo Z           |
| `[` / `]` | Diminuir/Aumentar escala    |

### Iluminação

| Tecla | Ação                                  |
| ----- | ------------------------------------- |
| `1`   | Ativar/desativar luz principal        |
| `2`   | Ativar/desativar luz de preenchimento |
| `3`   | Ativar/desativar contra-luz           |

### Sistema

| Tecla | Ação               |
| ----- | ------------------ |
| `ESC` | Encerrar aplicação |

---

## Arquitetura Técnica

### Geometria e carregamento de modelos

O projeto implementa um parser próprio para arquivos `.obj`, capaz de interpretar vértices (`v`), coordenadas de textura (`vt`), normais (`vn`) e materiais (`.mtl`).

Os dados são compactados em um único *Vertex Buffer Object* (VBO), utilizando um layout de 11 atributos por vértice:

* Posição (3 floats);
* Cor (3 floats);
* Normal (3 floats);
* Coordenadas UV (2 floats).

### Texture Atlas

Todas as texturas são agrupadas em um único atlas (`All_blocks.png`), reduzindo trocas de textura na GPU e melhorando a eficiência da renderização.

As coordenadas UV são recalculadas dinamicamente a partir da coluna e da linha especificadas no arquivo de configuração da cena.

Também foi implementado um algoritmo de *UV Inset* para minimizar artefatos de *texture bleeding*.

### Iluminação

O sistema utiliza o modelo de iluminação de Phong implementado em *Fragment Shader*.

Para cada fonte de luz ativa são calculadas:

* Componente ambiente (`Ka`);
* Componente difusa (`Kd`);
* Componente especular (`Ks`);
* Expoente de brilho (`Ns`).

O projeto utiliza uma configuração de iluminação de três pontos:

* Luz principal (*Key Light*);
* Luz de preenchimento (*Fill Light*);
* Contra-luz (*Back Light*).

### Bounding Box

Cada objeto pode ser destacado por uma *Bounding Box* em modo wireframe, utilizada para depuração e validação de transformações.

---

## Configuração da Cena

A cena é descrita externamente pelo arquivo `cena.txt`, permitindo separar os dados da aplicação da lógica de renderização.

Formato:

```text
objeto caminhoOBJ coluna linha posX posY posZ rotX rotY rotZ escala animacao
```

Exemplo:

```text
objeto ../assets/Modelos3D/Grass_block.obj 5 2 0.0 1.0 0.0 0.0 0.0 0.0 1.0 0
```

Parâmetros:

* `caminhoOBJ`: modelo utilizado;
* `coluna` e `linha`: posição da textura no atlas;
* `posX`, `posY`, `posZ`: posição inicial;
* `rotX`, `rotY`, `rotZ`: rotação inicial;
* `escala`: fator de escala uniforme;
* `animacao`: habilita ou desabilita animação paramétrica.

O arquivo também permite configurar câmera e iluminação.

---

## Assets

Os seguintes assets são utilizados no projeto:

| Asset             | Descrição                                         |
| ----------------- | ------------------------------------------------- |
| `Grass_block.obj` | Modelo base cúbico utilizado para todos os blocos |
| `Grass_block.mtl` | Materiais associados ao modelo                    |
| `All_blocks.png`  | Texture Atlas contendo as texturas                |
| `cena.txt`        | Arquivo de configuração da cena                   |

### Procedência

* Texturas inspiradas no universo Minecraft, utilizadas exclusivamente para fins educacionais e acadêmicos.
* Modelo cúbico desenvolvido e adaptado para o projeto.
* Organização e ajuste das coordenadas UV realizados no software Blender.

### Ferramentas auxiliares

* Blender 4.0.2 — modelagem e ajustes dos assets.
* Visual Studio Code — desenvolvimento.
* Git e GitHub — controle de versão.
* Gimp

---

## Tecnologias Utilizadas

* C++
* OpenGL 4.1
* GLFW
* GLAD
* GLM
* stb_image
* GLSL
* CMake
* Git

---

## Referências

UNIVERSIDADE DO VALE DO RIO DOS SINOS (UNISINOS). *Computação Gráfica: aprofundamento de estudos*. São Leopoldo: Unisinos, 2026. Material disponibilizado no Ambiente Virtual de Aprendizagem Moodle.

DE VRIES, Joey. *LearnOpenGL*. [S. l.], 2014–. Disponível em: https://learnopengl.com. Acesso em: 18 jun. 2026.

GET INTO GAME DEV. *Canal no YouTube*. Disponível em: https://www.youtube.com/@GetIntoGameDev. Acesso em: 18 jun. 2026.

GORDAN, Victor. *Canal no YouTube*. Disponível em: https://www.youtube.com/@VictorGordan. Acesso em: 18 jun. 2026.

---

## Licença

Este projeto foi desenvolvido exclusivamente para fins acadêmicos na disciplina de Computação Gráfica da Unisinos.

Os assets e texturas utilizados pertencem aos seus respectivos autores e detentores de direitos.
