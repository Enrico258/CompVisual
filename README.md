# Projeto 1 - Computação Visual

Enrico Cuono Alves Pereira - 10402875

Gabriel Mason Guerino - 10409928

# Resumo do projeto:

O projeto consiste em um software processador de imagens desenvolvido em C as bibliotecas SDL3, SDL3_image e SDL3_TTF. O programa é executado via linha de comando em um formato padrão, exemplo: 

```bash
.\processador dog.png
```

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
gcc processadorIMG.c -o processador flas-> vou por
```

### 3. Execução

```bash
.\processador dog.png
```
obs: pode ser feita a substituição do dog.png por qualquer imagem png, jpg ou bmp de sua escolha
