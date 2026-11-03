# Random-Course

Um jogo simples de plataforma com um local fixo que o personagem começa e outro local fixo que você precisa chegar, tem um tempo específico para você chegar e, quando você termina, os obstáculos da fase mudam de forma aleatória e o tempo diminui. Ou seja, são fases de plataforma aleatórias infinitas para ver quantas você consegue completar.

## Como rodar (Windows)

1. Instale a raylib pelo instalador do site [raylib.com](https://www.raylib.com), na pasta padrão `C:\raylib`.
2. Instale o [Git](https://git-scm.com) e o [VS Code](https://code.visualstudio.com) com a extensão C/C++.
3. Clone o repositório:
   ```
   git clone https://github.com/chagas-8/Random-Course.git
   ```
4. Abra a pasta no VS Code e, no terminal, compile:
   ```
   C:\raylib\w64devkit\bin\gcc.exe main.c -o jogo.exe -I C:/raylib/raylib/src -L C:/raylib/raylib/src -lraylib -lgdi32 -lwinmm
   ```
5. Rode o jogo:
   ```
   ./jogo.exe
   ```

## Como rodar (Mac)

1. Instale as ferramentas da Apple: `xcode-select --install`
2. Instale o [Homebrew](https://brew.sh) e depois a raylib:
   ```
   brew install raylib pkg-config
   ```
3. Clone o repositório e, dentro da pasta, compile:
   ```
   cc main.c -o jogo $(pkg-config --cflags --libs raylib)
   ```
4. Rode o jogo:
   ```
   ./jogo
   ```
   
## Controles

- **Setas:** andar
- **Espaço:** pular
- **Enter:** recomeçar após o game over