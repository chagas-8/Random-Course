#include "raylib.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define LARGURA 800
#define ALTURA 450
#define GRAVIDADE 1500.0f
#define VEL_PULO 600.0f
#define VEL_ANDAR 250.0f
#define TEMPO_INICIAL 20.0f
#define TEMPO_MINIMO 5.0f
#define TAM_BLOCO 32
#define COLUNAS 25
#define LINHAS 14
#define VAZIO 0
#define BLOCO 1
#define PORTA 2
#define TIPO_PILAR 0
#define TIPO_FLUTUANTE 1
#define TIPO_TETO 2
#define PULO_MAX_ALTURA 3

typedef struct {
    Rectangle corpo;
    Vector2 vel;
    bool noChao;
} Jogador;

typedef struct Plataforma {
    int tipo;
    int col, lin, larg, alt;
    struct Plataforma *prox;
} Plataforma;

void resetarJogador(Jogador *j) {
    j->corpo = (Rectangle){ 40, 344, 20, 40 };
    j->vel = (Vector2){ 0, 0 };
    j->noChao = false;
}
int **criarMapa(void) {
    int **mapa = malloc(LINHAS * sizeof(int *));
    for (int l = 0; l < LINHAS; l++) {
        mapa[l] = calloc(COLUNAS, sizeof(int));
    }
    return mapa;
}

void liberarMapa(int **mapa) {
    for (int l = 0; l < LINHAS; l++) {
        free(mapa[l]);
    }
    free(mapa);
}

void preencherArea(int **mapa, int col, int lin, int larg, int alt, int valor) {
    for (int l = lin; l < lin + alt; l++) {
        for (int c = col; c < col + larg; c++) {
            if (l >= 0 && l < LINHAS && c >= 0 && c < COLUNAS) {
                mapa[l][c] = valor;
            }
        }
    }
}

int lerCelula(int **mapa, int lin, int col) {
    if (col < 0 || col >= COLUNAS) return BLOCO;
    if (lin < 0 || lin >= LINHAS) return VAZIO;
    return mapa[lin][col];
}

bool tocaTipo(int **mapa, Rectangle r, int tipo) {
    int c1 = (int)floorf(r.x / TAM_BLOCO);
    int c2 = (int)floorf((r.x + r.width - 0.01f) / TAM_BLOCO);
    int l1 = (int)floorf(r.y / TAM_BLOCO);
    int l2 = (int)floorf((r.y + r.height - 0.01f) / TAM_BLOCO);
    for (int l = l1; l <= l2; l++) {
        for (int c = c1; c <= c2; c++) {
            if (lerCelula(mapa, l, c) == tipo) return true;
        }
    }
    return false;
}

Plataforma *adicionarPlataforma(Plataforma *lista, int tipo, int col, int lin, int larg, int alt) {
    Plataforma *nova = malloc(sizeof(Plataforma));
    nova->tipo = tipo;
    nova->col = col;
    nova->lin = lin;
    nova->larg = larg;
    nova->alt = alt;
    nova->prox = lista;
    return nova;
}

void liberarLista(Plataforma *lista) {
    while (lista != NULL) {
        Plataforma *proxima = lista->prox;
        free(lista);
        lista = proxima;
    }
}

Plataforma *sortearPlataformas(void) {
    Plataforma *lista = NULL;
    int quantidade = GetRandomValue(4, 7);

    for (int i = 0; i < quantidade; i++) {
        int tipo = GetRandomValue(TIPO_PILAR, TIPO_TETO);
        int col, lin, larg, alt;

        if (tipo == TIPO_PILAR) {
            larg = GetRandomValue(1, 2);
            alt = GetRandomValue(3, 9);
            lin = LINHAS - alt;
        } else if (tipo == TIPO_FLUTUANTE) {
            larg = GetRandomValue(2, 4);
            alt = 1;
            lin = GetRandomValue(5, 10);
        } else {
            larg = GetRandomValue(3, 6);
            alt = 1;
            lin = GetRandomValue(1, 4);
        }

        col = GetRandomValue(5, 20 - larg);
        lista = adicionarPlataforma(lista, tipo, col, lin, larg, alt);
    }

    return lista;
}

void montarFase(int **mapa, Plataforma *lista) {
    preencherArea(mapa, 0, 0, COLUNAS, LINHAS, VAZIO);
    preencherArea(mapa, 0, 12, 5, 2, BLOCO);
    preencherArea(mapa, 20, 5, 5, 1, BLOCO);
    preencherArea(mapa, 23, 3, 1, 2, PORTA);

    for (Plataforma *p = lista; p != NULL; p = p->prox) {
        preencherArea(mapa, p->col, p->lin, p->larg, p->alt, BLOCO);
    }
}

bool livre(int **mapa, int l, int c) {
    return lerCelula(mapa, l, c) != BLOCO;
}

bool podeFicarEmPe(int **mapa, int l, int c) {
    if (c < 0 || c >= COLUNAS || l < 1 || l >= LINHAS - 1) return false;
    return livre(mapa, l, c) && livre(mapa, l - 1, c) && lerCelula(mapa, l + 1, c) == BLOCO;
}

bool colunaLivre(int **mapa, int c, int lTopo, int lBase) {
    for (int l = lTopo; l <= lBase; l++) {
        if (!livre(mapa, l, c) || !livre(mapa, l - 1, c)) return false;
    }
    return true;
}

bool linhaLivre(int **mapa, int l, int c1, int c2) {
    int inicio = c1 < c2 ? c1 : c2;
    int fim = c1 < c2 ? c2 : c1;
    for (int c = inicio; c <= fim; c++) {
        if (!livre(mapa, l, c) || !livre(mapa, l - 1, c)) return false;
    }
    return true;
}

bool caminhoLivre(int **mapa, int l1, int c1, int l2, int c2) {
    if (l2 < l1) {
        return colunaLivre(mapa, c1, l2, l1) && linhaLivre(mapa, l2, c1, c2);
    }
    return linhaLivre(mapa, l1, c1, c2) && colunaLivre(mapa, c2, l1, l2);
}

int alcanceHorizontal(int subida) {
    if (subida >= 3) return 3;
    if (subida == 2) return 4;
    return 5;
}

bool explorar(int **mapa, int **visitado, int l, int c) {
    if (visitado[l][c]) return false;
    visitado[l][c] = 1;

    if (lerCelula(mapa, l, c) == PORTA || lerCelula(mapa, l - 1, c) == PORTA) return true;

    for (int l2 = l - PULO_MAX_ALTURA; l2 < LINHAS - 1; l2++) {
        int alcance = alcanceHorizontal(l - l2);
        for (int c2 = c - alcance; c2 <= c + alcance; c2++) {
            if (podeFicarEmPe(mapa, l2, c2) && caminhoLivre(mapa, l, c, l2, c2)) {
                if (explorar(mapa, visitado, l2, c2)) return true;
            }
        }
    }
    return false;
}

bool faseValida(int **mapa) {
    int **visitado = criarMapa();
    bool ok = explorar(mapa, visitado, 11, 1);
    liberarMapa(visitado);
    return ok;
}

void novaFase(int **mapa, Plataforma **lista) {
    do {
        liberarLista(*lista);
        *lista = sortearPlataformas();
        montarFase(mapa, *lista);
    } while (!faseValida(mapa));
}

int main(void) {
    InitWindow(LARGURA, ALTURA, "Jogo de Plataforma");
    SetTargetFPS(60);

    int **mapa = criarMapa();
    SetRandomSeed((unsigned int)time(NULL));
    Plataforma *plataformas = NULL;
    novaFase(mapa, &plataformas);

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
            if (IsKeyPressed(KEY_R)) {
                novaFase(mapa, &plataformas);
                resetarJogador(&j);
                novaFase(mapa, &plataformas);
            }

            j.vel.y += GRAVIDADE * dt;

            j.corpo.x += j.vel.x * dt;
            if (tocaTipo(mapa, j.corpo, BLOCO)) {
                if (j.vel.x > 0) j.corpo.x = floorf((j.corpo.x + j.corpo.width) / TAM_BLOCO) * TAM_BLOCO - j.corpo.width;
                else if (j.vel.x < 0) j.corpo.x = (floorf(j.corpo.x / TAM_BLOCO) + 1) * TAM_BLOCO;
            }
            if (j.corpo.x < 0) j.corpo.x = 0;
            if (j.corpo.x + j.corpo.width > LARGURA) j.corpo.x = LARGURA - j.corpo.width;

            j.corpo.y += j.vel.y * dt;
            j.noChao = false;
            if (tocaTipo(mapa, j.corpo, BLOCO)) {
                if (j.vel.y > 0) {
                    j.corpo.y = floorf((j.corpo.y + j.corpo.height) / TAM_BLOCO) * TAM_BLOCO - j.corpo.height;
                    j.noChao = true;
                } else if (j.vel.y < 0) {
                    j.corpo.y = (floorf(j.corpo.y / TAM_BLOCO) + 1) * TAM_BLOCO;
                }
                j.vel.y = 0;
            }
            tempo -= dt;
            if (j.corpo.y > ALTURA || tempo <= 0) gameOver = true;

            if (tocaTipo(mapa, j.corpo, PORTA)) {
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
            novaFase(mapa, &plataformas);
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        for (int l = 0; l < LINHAS; l++) {
        for (int c = 0; c < COLUNAS; c++) {
            if (mapa[l][c] == BLOCO) DrawRectangle(c * TAM_BLOCO, l * TAM_BLOCO, TAM_BLOCO, TAM_BLOCO, DARKGRAY);
            else if (mapa[l][c] == PORTA) DrawRectangle(c * TAM_BLOCO, l * TAM_BLOCO, TAM_BLOCO, TAM_BLOCO, BROWN);
        }
}
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

    liberarLista(plataformas);
    liberarMapa(mapa);
    CloseWindow();
    return 0;
}
