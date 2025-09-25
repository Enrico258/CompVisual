# Projeto 1 - Computação Visual

**Integrantes:**

[Enrico Cuono Alves Pereira - 10402875](https://github.com/Enrico258)

[Gabriel Mason Guerino - 10409928](https://github.com/GabrielMasonGuerino)

[Eduardo Honorio Friaça - 10408959](https://github.com/EduardoFriaca)

# Resumo do projeto:

O projeto consiste em um software processador de imagens desenvolvido em C, fazendo uso das bibliotecas SDL3, SDL3_image e SDL3_TTF. 

**Suas principais funcionalidades incluem:**
- Carregamento e tratamento de erros no acesso da imagem e exibição de interface gráfica com duas janelas (a principal para mostrar a imagem e a secundária para exibir o histograma), além da possibilidade de salvar a imagem processada em arquivo. **Responsável: Enrico**
- Análise e conversão da imagem para escala de cinza. **Responsável: Eduardo**
- Cálculo e análise estatística do histograma (média de intensidade e contraste), equalização do histograma com alternância entre imagem original e equalizada. **Responsável: Gabriel**

# Compilação e execução:

### 1. Clone o repositório

```bash
git clone https://github.com/Enrico258/CompVisual.git
```

### 2. Compilar

```bash
gcc processadorV2.c -o processador -IC:/libs/SDL3/include -LC:/libs/SDL3/lib -lSDL3 -IC:/libs/SDL3_image/include -LC:/libs/SDL3_image/lib -lSDL3_image -IC:/libs/SDL3_ttf/include -LC:/libs/SDL3_ttf/lib -lSDL3_ttf
```
OU (caso as libs estejam instaladas no disco local D):

```bash
gcc processadorV2.c -o processador -ID:/libs/SDL3/include -LD:/libs/SDL3/lib -lSDL3 -ID:/libs/SDL3_image/include -LD:/libs/SDL3_image/lib -lSDL3_image -ID:/libs/SDL3_ttf/include -LD:/libs/SDL3_ttf/lib -lSDL3_ttf
```

### 3. Execução

```bash
.\processador dog.png
```

OU (caso receba o Aviso: "nennhuma fonte TTF encontrada.")

```bash
.\processador dog.png "C:\Windows\Fonts\arial.ttf"
```

obs: pode ser feita a substituição do dog.png por qualquer imagem png, jpg ou bmp de sua escolha

# Exemplo de execução:

<img width="1483" height="805" alt="image" src="https://github.com/user-attachments/assets/c28e78ab-cf55-4744-a277-9f85cae3dcbb" />

