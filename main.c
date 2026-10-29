#include "raylib.h"

#define LARGURA 800
#define ALTURA 450
#define NUM_PLAT 5
#define GRAVIDADE 1500.0f
#define VEL_PULO 600.0f
#define VEL_ANDAR 250.0f
#define TEMPO_INICIAL 20.0f
#define TEMPO_MINIMO 5.0f

typedef struct {
    Rectangle corpo;
    Vector2 vel;
    bool noChao;
} Jogador;

void resetarJogador(Jogador *j) {
    j->corpo = (Rectangle){ 40, 330, 20, 40 };
    j->vel = (Vector2){ 0, 0 };
    j->noChao = false;
}

int main(void) {
    InitWindow(LARGURA, ALTURA, "Jogo de Plataforma");
    SetTargetFPS(60);

    Rectangle plataformas[NUM_PLAT] = {
        { 0,   380, 150, 70  },
        { 220, 320, 60,  130 },
        { 350, 260, 90,  50  },
        { 500, 220, 60,  230 },
        { 620, 160, 180, 40  }
    };
    Rectangle porta = { 740, 110, 30, 50 };

    Jogador j;
    resetarJogador(&j);

    float tempoMax = TEMPO_INICIAL;
    float tempo = tempoMax;
    int nivel = 0;
    bool gameOver = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (!gameOver) {
            j.vel.x = 0;
            if (IsKeyDown(KEY_LEFT))  j.vel.x = -VEL_ANDAR;
            if (IsKeyDown(KEY_RIGHT)) j.vel.x = VEL_ANDAR;
            if (IsKeyPressed(KEY_SPACE) && j.noChao) j.vel.y = -VEL_PULO;

            j.vel.y += GRAVIDADE * dt;

            j.corpo.x += j.vel.x * dt;
            for (int i = 0; i < NUM_PLAT; i++) {
                Rectangle p = plataformas[i];
                if (CheckCollisionRecs(j.corpo, p)) {
                    if (j.vel.x > 0) j.corpo.x = p.x - j.corpo.width;
                    else if (j.vel.x < 0) j.corpo.x = p.x + p.width;
                }
            }
            if (j.corpo.x < 0) j.corpo.x = 0;
            if (j.corpo.x + j.corpo.width > LARGURA) j.corpo.x = LARGURA - j.corpo.width;

            j.corpo.y += j.vel.y * dt;
            j.noChao = false;
            for (int i = 0; i < NUM_PLAT; i++) {
                Rectangle p = plataformas[i];
                if (CheckCollisionRecs(j.corpo, p)) {
                    if (j.vel.y > 0) {
                        j.corpo.y = p.y - j.corpo.height;
                        j.noChao = true;
                    } else if (j.vel.y < 0) {
                        j.corpo.y = p.y + p.height;
                    }
                    j.vel.y = 0;
                }
            }

            tempo -= dt;
            if (j.corpo.y > ALTURA || tempo <= 0) gameOver = true;

            if (CheckCollisionRecs(j.corpo, porta)) {
                nivel++;
                tempoMax *= 0.9f;
                if (tempoMax < TEMPO_MINIMO) tempoMax = TEMPO_MINIMO;
                tempo = tempoMax;
                resetarJogador(&j);
            }
        } else if (IsKeyPressed(KEY_ENTER)) {
            nivel = 0;
            tempoMax = TEMPO_INICIAL;
            tempo = tempoMax;
            gameOver = false;
            resetarJogador(&j);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (int i = 0; i < NUM_PLAT; i++) DrawRectangleRec(plataformas[i], DARKGRAY);
        DrawRectangleRec(porta, BROWN);
        DrawRectangleRec(j.corpo, BLUE);

        DrawText(TextFormat("Nivel: %d", nivel), 10, 10, 20, BLACK);
        DrawText(TextFormat("Tempo: %.1f", tempo), 10, 35, 20, tempo < 5 ? RED : BLACK);

        if (gameOver) {
            DrawRectangle(0, 0, LARGURA, ALTURA, Fade(BLACK, 0.6f));
            DrawText("GAME OVER", 290, 160, 40, RED);
            DrawText(TextFormat("Niveis passados: %d", nivel), 300, 215, 22, WHITE);
            DrawText("Enter para recomecar", 295, 255, 20, LIGHTGRAY);
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
