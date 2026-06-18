# Motor Gráfico Diorama - Universo Minecraft (OpenGL + C++)

**Desenvolvedor:** Gustavo de Oliveira  
**Contexto:** Projeto Final Integrado - Grau B  

Este projeto consiste em um motor gráfico interativo desenvolvido em C++ e OpenGL (Core Profile 4.1). O objetivo é renderizar um diorama tridimensional inspirado no universo de Minecraft, demonstrando os conceitos fundamentais do pipeline gráfico programável, mapeamento de texturas via Texture Atlas e o modelo de iluminação de Phong.

---

## 1. Funcionalidades e Ativação em Tempo Real

O visualizador interativo possui mapeamento completo de teclado e mouse para validação técnica durante a defesa:

* **Navegação de Câmera:** Teclas `I`, `K`, `J`, `L` para movimentação espacial e rastreamento do mouse para controle dos ângulos de *Yaw* e *Pitch* (olhar ao redor).
* **Seleção de Objetos:** Tecla `TAB` alterna ciclicamente entre os blocos carregados na cena.
* **Transformações Geométricas:** * **Translação:** Teclas `W`/`S` (Eixo Y), `A`/`D` (Eixo X) e `Q`/`E` (Eixo Z).
  * **Rotação:** `Seta UP`/`Seta DOWN` (Eixo X), `Seta LEFT`/`Seta RIGHT` (Eixo Y) e `Z`/`C` (Eixo Z).
  * **Escala Uniforme:** Teclas `[` (Diminuir) e `]` (Aumentar).
* **Controle de Iluminação (Lógica de 3 Pontos):** Teclas `1`, `2` e `3` ligam/desligam individualmente a Luz Principal (Key), Luz de Preenchimento (Fill) e a Contra-Luz (Back).
* **Animação Paramétrica:** Blocos configurados com flag de animação realizam trajetórias senoidais contínuas baseadas no tempo de execução (`glfwGetTime`).

---

## 2. Arquitetura Técnica e Pipeline

### Geometria e Parser (.obj e .mtl)
O sistema conta com um parser customizado (`loadSimpleOBJ`) projetado para ler arquivos de geometria pura em triângulos (`v/vt/vn`). O parser extrai as coordenadas espaciais, normais e coordenadas de textura, compactando-as em um único Vertex Buffer Object (VBO) com layout de stride de 11 floats por vértice.

### Otimização: Mapeamento via Texture Atlas
Para evitar o gargalo de performance gerado pela troca constante de texturas na GPU (*texture binding*), o projeto utiliza uma abordagem de **Texture Atlas unificado** (`All_blocks.png`). O código intercepta as coordenadas UV padrão (0.0 a 1.0) vindas do arquivo `.obj` e recalcula dinamicamente os novos limites baseados na coluna e na linha passadas pelo arquivo de configuração da cena (`cena.txt`). Um algoritmo de *UV Inset* (margem de segurança) foi implementado para mitigar o sangramento de pixels (*texture bleeding*) gerado pelo filtro `GL_NEAREST`.

### Modelo de Iluminação
A renderização utiliza sombreamento por fragmento (*Per-Pixel Lighting*) implementando o **Modelo de Iluminação de Phong** completo no Fragment Shader. Para cada uma das 3 fontes de luz ativas, calcula-se:
1. **Componente Ambiente:** Baseada no coeficiente `ka`.
2. **Componente Difusa:** Baseada no produto escalar do vetor normal da superfície com o vetor de direção da luz ($\max(\vec{N} \cdot \vec{L}, 0.0)$), ponderada pelo coeficiente `kd`.
3. **Componente Especular:** Baseada no reflexo da luz em direção ao observador ($\max(\vec{R} \cdot \vec{V}, 0.0)^q$), controlada pelo coeficiente `ks` e pelo expoente de brilho `q`.