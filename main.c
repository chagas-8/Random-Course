#include "raylib.h"
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

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
#define ESPINHO 3
#define TIPO_PILAR 0
#define TIPO_FLUTUANTE 1
#define TIPO_TETO 2
#define LARG_JOGADOR 20
#define ALT_JOGADOR 40
#define DT_SIMULACAO (1.0f / 60.0f)
#define MAX_RANKING 5
#define ARQUIVO_RANKING "ranking.txt"
#define COR_DESTAQUE GREEN
#define COR_DESTAQUE_INATIVA DARKGREEN
#define TIPO_MOVEL 3
#define NIVEL_MOVEIS 2
#define CHANCE_MOVEIS 40
#define NIVEL_ESPINHOS 4
#define MAX_ESPINHOS 5
#define CHANCE_RELOGIO_INICIAL 20
#define CHANCE_RELOGIO_POR_NIVEL 5
#define CHANCE_RELOGIO_MAX 80

typedef struct {
    Rectangle corpo;
    Vector2 vel;
    bool noChao;
    bool olhandoEsquerda;
} Jogador;

typedef struct Plataforma {
    int tipo;
    int col, lin, larg, alt;
    float x, y;
    float vel;
    float xMin, xMax;
    struct Plataforma *prox;
} Plataforma;

typedef struct {
    char nome[4];
    int niveis;
} Recorde;

typedef enum {
    ESTADO_MENU,
    ESTADO_JOGO,
    ESTADO_GAMEOVER,
    ESTADO_NOME,
    ESTADO_RANKING
} Estado;

void resetarJogador(Jogador *j) {
    j->corpo = (Rectangle){ 40, 344, LARG_JOGADOR, ALT_JOGADOR };
    j->vel = (Vector2){ 0, 0 };
    j->noChao = false;
    j->olhandoEsquerda = false;
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

bool tocaEspinho(int **mapa, Rectangle r) {
    int c1 = (int)floorf(r.x / TAM_BLOCO);
    int c2 = (int)floorf((r.x + r.width - 0.01f) / TAM_BLOCO);
    int l1 = (int)floorf(r.y / TAM_BLOCO);
    int l2 = (int)floorf((r.y + r.height - 0.01f) / TAM_BLOCO);
    for (int l = l1; l <= l2; l++) {
        for (int c = c1; c <= c2; c++) {
            if (lerCelula(mapa, l, c) != ESPINHO) continue;
            Rectangle ponta = { c * TAM_BLOCO + 4, l * TAM_BLOCO + 22, 24, 10 };
            if (CheckCollisionRecs(r, ponta)) return true;
        }
    }
    return false;
}

void moverJogador(int **mapa, Jogador *j, float dt) {
    j->vel.y += GRAVIDADE * dt;

    j->corpo.x += j->vel.x * dt;
    if (tocaTipo(mapa, j->corpo, BLOCO)) {
        if (j->vel.x > 0) j->corpo.x = floorf((j->corpo.x + j->corpo.width) / TAM_BLOCO) * TAM_BLOCO - j->corpo.width;
        else if (j->vel.x < 0) j->corpo.x = (floorf(j->corpo.x / TAM_BLOCO) + 1) * TAM_BLOCO;
    }
    if (j->corpo.x < 0) j->corpo.x = 0;
    if (j->corpo.x + j->corpo.width > LARGURA) j->corpo.x = LARGURA - j->corpo.width;

    j->corpo.y += j->vel.y * dt;
    j->noChao = false;
    if (tocaTipo(mapa, j->corpo, BLOCO)) {
        if (j->vel.y > 0) {
            j->corpo.y = floorf((j->corpo.y + j->corpo.height) / TAM_BLOCO) * TAM_BLOCO - j->corpo.height;
            j->noChao = true;
        } else if (j->vel.y < 0) {
            j->corpo.y = (floorf(j->corpo.y / TAM_BLOCO) + 1) * TAM_BLOCO;
        }
        j->vel.y = 0;
    }
}

Plataforma *adicionarPlataforma(Plataforma *lista, int tipo, int col, int lin, int larg, int alt) {
    Plataforma *nova = malloc(sizeof(Plataforma));
    nova->tipo = tipo;
    nova->col = col;
    nova->lin = lin;
    nova->larg = larg;
    nova->alt = alt;
    nova->x = col * TAM_BLOCO;
    nova->y = lin * TAM_BLOCO;
    nova->vel = 0;
    nova->xMin = nova->xMax = nova->x;
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
        if (p->tipo == TIPO_MOVEL) continue;
        preencherArea(mapa, p->col, p->lin, p->larg, p->alt, BLOCO);
    }
}

bool livre(int **mapa, int l, int c) {
    int v = lerCelula(mapa, l, c);
    return v != BLOCO && v != ESPINHO;
}

bool podeFicarEmPe(int **mapa, int l, int c) {
    if (c < 0 || c >= COLUNAS || l < 1 || l >= LINHAS - 1) return false;
    return livre(mapa, l, c) && livre(mapa, l - 1, c) && lerCelula(mapa, l + 1, c) == BLOCO;
}

bool faixaLivre(int **mapa, int lin, int c) {
    return livre(mapa, lin, c) && livre(mapa, lin - 1, c) && livre(mapa, lin - 2, c);
}

void colocarEspinhos(int **mapa, int nivel) {
    if (nivel < NIVEL_ESPINHOS) return;

    int quantidade = 1 + (nivel - NIVEL_ESPINHOS) / 2;
    if (quantidade > MAX_ESPINHOS) quantidade = MAX_ESPINHOS;

    for (int i = 0; i < quantidade; i++) {
        for (int tentativa = 0; tentativa < 20; tentativa++) {
            int l = GetRandomValue(1, LINHAS - 2);
            int c = GetRandomValue(5, 19);
            if (podeFicarEmPe(mapa, l, c)) {
                mapa[l][c] = ESPINHO;
                break;
            }
        }
    }
}

Plataforma *sortearMoveis(Plataforma *lista, int **mapa, int nivel) {
    if (nivel < NIVEL_MOVEIS || GetRandomValue(1, 100) > CHANCE_MOVEIS) return lista;

    int quantidade = GetRandomValue(1, 2);
    for (int i = 0; i < quantidade; i++) {
        for (int tentativa = 0; tentativa < 20; tentativa++) {
            int lin = GetRandomValue(4, 10);
            int larg = GetRandomValue(2, 3);
            int ini = GetRandomValue(5, 19);
            if (!faixaLivre(mapa, lin, ini)) continue;

            int fim = ini;
            while (ini > 5 && faixaLivre(mapa, lin, ini - 1)) ini--;
            while (fim < 19 && faixaLivre(mapa, lin, fim + 1)) fim++;
            if (fim - ini + 1 < larg + 2) continue;

            lista = adicionarPlataforma(lista, TIPO_MOVEL, ini, lin, larg, 1);
            lista->xMin = ini * TAM_BLOCO;
            lista->xMax = (fim + 1 - larg) * TAM_BLOCO;
            lista->vel = GetRandomValue(60, 110) * (GetRandomValue(0, 1) ? 1 : -1);
            break;
        }
    }
    return lista;
}

void atualizarMoveis(Plataforma *lista, float dt) {
    for (Plataforma *p = lista; p != NULL; p = p->prox) {
        if (p->tipo != TIPO_MOVEL) continue;
        p->x += p->vel * dt;
        if (p->x < p->xMin) { p->x = p->xMin; p->vel = -p->vel; }
        if (p->x > p->xMax) { p->x = p->xMax; p->vel = -p->vel; }
    }
}

void colidirMoveis(Plataforma *lista, int **mapa, Jogador *j, float pesAntes, float dt) {
    if (j->vel.y < 0) return;

    for (Plataforma *p = lista; p != NULL; p = p->prox) {
        if (p->tipo != TIPO_MOVEL) continue;

        float largura = p->larg * TAM_BLOCO;
        bool emCimaX = j->corpo.x + j->corpo.width > p->x && j->corpo.x < p->x + largura;
        float pes = j->corpo.y + j->corpo.height;

        if (emCimaX && pesAntes <= p->y + 1 && pes >= p->y) {
            j->corpo.y = p->y - j->corpo.height;
            j->vel.y = 0;
            j->noChao = true;

            float antes = j->corpo.x;
            j->corpo.x += p->vel * dt;
            if (tocaTipo(mapa, j->corpo, BLOCO)) j->corpo.x = antes;
            return;
        }
    }
}
int simularMovimento(int **mapa, float x, float y, bool pular, int dir1, int dir2, float troca, int *linDestino, int *colDestino) {
    Jogador s;
    s.corpo = (Rectangle){ x, y, LARG_JOGADOR, ALT_JOGADOR };
    s.vel = (Vector2){ 0, pular ? -VEL_PULO : 0 };
    s.noChao = true;
    bool saiuDoChao = false;

    for (float t = 0; t < 2.5f; t += DT_SIMULACAO) {
        int dir = t < troca ? dir1 : dir2;
        s.vel.x = dir * VEL_ANDAR;
        moverJogador(mapa, &s, DT_SIMULACAO);

        if (tocaTipo(mapa, s.corpo, PORTA)) return 2;
        if (s.corpo.y > ALTURA || tocaEspinho(mapa, s.corpo)) return 0;

        if (!s.noChao) {
            saiuDoChao = true;
        } else if (saiuDoChao) {
            int lin = (int)floorf((s.corpo.y + s.corpo.height) / TAM_BLOCO) - 1;
            int c1 = (int)floorf(s.corpo.x / TAM_BLOCO);
            int c2 = (int)floorf((s.corpo.x + s.corpo.width - 0.01f) / TAM_BLOCO);
            for (int c = c1; c <= c2; c++) {
                if (podeFicarEmPe(mapa, lin, c)) {
                    *linDestino = lin;
                    *colDestino = c;
                    return 1;
                }
            }
            return 0;
        } else if (t > 0.3f) {
            return 0;
        }
    }
    return 0;
}

bool explorar(int **mapa, int **visitado, int l, int c) {
    if (visitado[l][c]) return false;
    visitado[l][c] = 1;

    if (podeFicarEmPe(mapa, l, c - 1) && explorar(mapa, visitado, l, c - 1)) return true;
    if (podeFicarEmPe(mapa, l, c + 1) && explorar(mapa, visitado, l, c + 1)) return true;

    float y = (l + 1) * TAM_BLOCO - ALT_JOGADOR;
    float posicoes[2] = { c * TAM_BLOCO, c * TAM_BLOCO + TAM_BLOCO - LARG_JOGADOR };
    float trocas[4] = { 0.1f, 0.2f, 0.3f, 0.45f };

    for (int p = 0; p < 2; p++)
        for (int pulo = 0; pulo < 2; pulo++)
            for (int d1 = -1; d1 <= 1; d1++)
                for (int d2 = -1; d2 <= 1; d2++)
                    for (int t = 0; t < 4; t++) {
                        int l2, c2;
                        int r = simularMovimento(mapa, posicoes[p], y, pulo, d1, d2, trocas[t], &l2, &c2);
                        if (r == 2) return true;
                        if (r == 1 && explorar(mapa, visitado, l2, c2)) return true;
                    }
    return false;
}

bool faseValida(int **mapa) {
    int **visitado = criarMapa();
    bool ok = explorar(mapa, visitado, 11, 1);
    liberarMapa(visitado);
    return ok;
}

void novaFase(int **mapa, Plataforma **lista, int nivel) {
    do {
        liberarLista(*lista);
        *lista = sortearPlataformas();
        montarFase(mapa, *lista);
        colocarEspinhos(mapa, nivel);
    } while (!faseValida(mapa));
    *lista = sortearMoveis(*lista, mapa, nivel);
}

void iniciarPartida(Jogador *j, int **mapa, Plataforma **lista, int *nivel, float *tempoMax, float *tempo) {
    *nivel = 0;
    *tempoMax = TEMPO_INICIAL;
    *tempo = *tempoMax;
    resetarJogador(j);
    novaFase(mapa, lista, *nivel);
}
int lerRanking(Recorde ranking[]) {
    FILE *arq = fopen(ARQUIVO_RANKING, "r");
    if (arq == NULL) return 0;

    int total = 0;
    while (total < MAX_RANKING && fscanf(arq, "%3s %d", ranking[total].nome, &ranking[total].niveis) == 2) {
        total++;
    }
    fclose(arq);
    return total;
}

void salvarRanking(Recorde ranking[], int total) {
    FILE *arq = fopen(ARQUIVO_RANKING, "w");
    if (arq == NULL) return;

    for (int i = 0; i < total; i++) {
        fprintf(arq, "%s %d\n", ranking[i].nome, ranking[i].niveis);
    }
    fclose(arq);
}

bool entraNoRanking(Recorde ranking[], int total, int niveis) {
    if (total < MAX_RANKING) return true;
    return niveis > ranking[total - 1].niveis;
}

int inserirRecorde(Recorde ranking[], int total, Recorde novo) {
    int pos = total;
    while (pos > 0 && ranking[pos - 1].niveis < novo.niveis) pos--;
    if (pos >= MAX_RANKING) return total;

    if (total < MAX_RANKING) total++;
    for (int i = total - 1; i > pos; i--) {
        ranking[i] = ranking[i - 1];
    }
    ranking[pos] = novo;
    return total;
}

void desenharCentralizado(Texture2D tex, float y, float larguraMax) {
    float escala = 1.0f;
    if (tex.width > larguraMax) escala = larguraMax / tex.width;

    float x = (LARGURA - tex.width * escala) / 2;
    DrawTextureEx(tex, (Vector2){ x, y }, 0, escala, WHITE);
}

bool sortearRelogio(int **mapa, Rectangle *r, int nivel) {
    int chance = CHANCE_RELOGIO_INICIAL + nivel * CHANCE_RELOGIO_POR_NIVEL;
    if (chance > CHANCE_RELOGIO_MAX) chance = CHANCE_RELOGIO_MAX;
    if (GetRandomValue(1, 100) > chance) return false;

    for (int tentativa = 0; tentativa < 50; tentativa++) {
        int l = GetRandomValue(1, LINHAS - 2);
        int c = GetRandomValue(5, 19);

        if (podeFicarEmPe(mapa, l, c)) {
            *r = (Rectangle){ c * TAM_BLOCO + 4, l * TAM_BLOCO + 4, 24, 24 };
            return true;
        }
    }
    return false;
}

void textoCentralizado(const char *texto, int y, int tamanho, Color cor) {
    int x = (LARGURA - MeasureText(texto, tamanho)) / 2;
    DrawText(texto, x, y, tamanho, cor);
}

char proximoCaractere(char c) {
    if (c == 'Z') return '0';
    if (c == '9') return 'A';
    return c + 1;
}

char caractereAnterior(char c) {
    if (c == 'A') return '9';
    if (c == '0') return 'Z';
    return c - 1;
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(LARGURA, ALTURA, "Random Course");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    Texture2D texFundo   = LoadTexture("assets/fundo.png");
    Texture2D texTopo    = LoadTexture("assets/bloco_topo.png");
    Texture2D texMeio    = LoadTexture("assets/bloco_meio.png");
    Texture2D texPorta   = LoadTexture("assets/porta.png");
    Texture2D texJogador = LoadTexture("assets/jogador.png");
    Texture2D texLogo    = LoadTexture("assets/logo.png");
    Texture2D texGameOver = LoadTexture("assets/gameover.png");
    Texture2D texRelogio  = LoadTexture("assets/relogio.png");
    Texture2D texEspinho  = LoadTexture("assets/espinho.png");

    RenderTexture2D tela = LoadRenderTexture(LARGURA, ALTURA);
    SetTextureFilter(tela.texture, TEXTURE_FILTER_POINT);

    int **mapa = criarMapa();
    SetRandomSeed((unsigned int)time(NULL));
    Plataforma *plataformas = NULL;
    novaFase(mapa, &plataformas, 0);

    Jogador j;
    resetarJogador(&j);

    float tempoMax = TEMPO_INICIAL;
    float tempo = tempoMax;
    int nivel = 0;
    Estado estado = ESTADO_MENU;
    int opcaoMenu = 0;
    int opcaoGameOver = 0;
    bool recordeSalvo = false;
    char nome[4] = "AAA";
    int letraAtual = 0;
    Recorde ranking[MAX_RANKING];
    int totalRanking = lerRanking(ranking);
    bool sair = false;
    Rectangle relogio;
    bool temRelogio = false;
    while (!WindowShouldClose() && !sair) {
        float dt = GetFrameTime();
        bool podeSalvar = !recordeSalvo && nivel > 0 && entraNoRanking(ranking, totalRanking, nivel);
        if (dt > 0.05f) dt = 0.05f;

        if (IsKeyPressed(KEY_F)) ToggleBorderlessWindowed();

        if (estado == ESTADO_MENU) {
            if (IsKeyPressed(KEY_ESCAPE)) sair = true;
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_DOWN)) opcaoMenu = !opcaoMenu;
            if (IsKeyPressed(KEY_ENTER)) {
                if (opcaoMenu == 0) {
                    iniciarPartida(&j, mapa, &plataformas, &nivel, &tempoMax, &tempo);
                    temRelogio = sortearRelogio(mapa, &relogio, nivel);
                    estado = ESTADO_JOGO;
                } else {
                    estado = ESTADO_RANKING;
                }
            }
        }
        else if (estado == ESTADO_JOGO) {
            j.vel.x = 0;
            if (IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A))  j.vel.x = -VEL_ANDAR;
            if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D)) j.vel.x = VEL_ANDAR;

            if (j.vel.x < 0) j.olhandoEsquerda = true;
            else if (j.vel.x > 0) j.olhandoEsquerda = false;

            bool pulo = IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
            if (pulo && j.noChao) j.vel.y = -VEL_PULO;
            float pesAntes = j.corpo.y + j.corpo.height;
            atualizarMoveis(plataformas, dt);
            moverJogador(mapa, &j, dt);
            colidirMoveis(plataformas, mapa, &j, pesAntes, dt);
            tempo -= dt;
            if (j.corpo.y > ALTURA || tempo <= 0 || tocaEspinho(mapa, j.corpo)) {
                estado = ESTADO_GAMEOVER;
                opcaoGameOver = 0;
                recordeSalvo = false;
            }

            if (temRelogio && CheckCollisionRecs(j.corpo, relogio)) {
                tempo = fminf(tempo + 1.5f, tempoMax);
                temRelogio = false;
            }

            if (tocaTipo(mapa, j.corpo, PORTA)) {
                nivel++;
                tempoMax *= 0.9f;
                if (tempoMax < TEMPO_MINIMO) tempoMax = TEMPO_MINIMO;
                tempo = tempoMax;
                resetarJogador(&j);
                novaFase(mapa, &plataformas, nivel);
                temRelogio = sortearRelogio(mapa, &relogio, nivel);
            }
        }
        else if (estado == ESTADO_GAMEOVER) {
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W))   opcaoGameOver = (opcaoGameOver + 2) % 3;
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) opcaoGameOver = (opcaoGameOver + 1) % 3;

            if (IsKeyPressed(KEY_ENTER)) {
                if (opcaoGameOver == 0) {
                    iniciarPartida(&j, mapa, &plataformas, &nivel, &tempoMax, &tempo);
                    temRelogio = sortearRelogio(mapa, &relogio, nivel);
                    estado = ESTADO_JOGO;
                } else if (opcaoGameOver == 1 && podeSalvar) {
                    strcpy(nome, "AAA");
                    letraAtual = 0;
                    estado = ESTADO_NOME;
                } else if (opcaoGameOver == 2) {
                    estado = ESTADO_MENU;
                }
            }
        }
        else if (estado == ESTADO_NOME) {
            if (IsKeyPressed(KEY_UP))   nome[letraAtual] = proximoCaractere(nome[letraAtual]);
            if (IsKeyPressed(KEY_DOWN)) nome[letraAtual] = caractereAnterior(nome[letraAtual]);
            if (IsKeyPressed(KEY_LEFT)  && letraAtual > 0) letraAtual--;
            if (IsKeyPressed(KEY_RIGHT) && letraAtual < 2) letraAtual++;

            int tecla = GetCharPressed();
            if (tecla >= 'a' && tecla <= 'z') tecla -= 32;
            if ((tecla >= 'A' && tecla <= 'Z') || (tecla >= '0' && tecla <= '9')) {
                nome[letraAtual] = (char)tecla;
                if (letraAtual < 2) letraAtual++;
            }

            if (IsKeyPressed(KEY_ENTER)) {
                Recorde novo;
                strcpy(novo.nome, nome);
                novo.niveis = nivel;
                totalRanking = inserirRecorde(ranking, totalRanking, novo);
                salvarRanking(ranking, totalRanking);
                recordeSalvo = true;
                estado = ESTADO_GAMEOVER;
            }
            if (IsKeyPressed(KEY_BACKSPACE)) estado = ESTADO_GAMEOVER;
        }
        else if (estado == ESTADO_RANKING) {
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_BACKSPACE) || IsKeyPressed(KEY_ESCAPE)) {
                estado = ESTADO_MENU;
            }
        }
        
        BeginTextureMode(tela);
        ClearBackground(BLACK);
        DrawTexture(texFundo, 0, 0, WHITE);

        if (estado == ESTADO_MENU) {
            desenharCentralizado(texLogo, 50, 600);
            textoCentralizado("JOGAR", 300, 28, opcaoMenu == 0 ? COR_DESTAQUE : WHITE);
            textoCentralizado("RANKING", 340, 28, opcaoMenu == 1 ? COR_DESTAQUE : WHITE);
            textoCentralizado("F para tela cheia, Setas/WASD para escolher, Enter para confirmar, Esc para sair", 410, 16, GRAY);
        }
        else if (estado == ESTADO_RANKING) {
            textoCentralizado("RANKING", 50, 40, RED);
            if (totalRanking == 0) {
                textoCentralizado("Nenhum recorde ainda", 200, 22, WHITE);
            }
            for (int i = 0; i < totalRanking; i++) {
                textoCentralizado(TextFormat("%d. %s  -  %d niveis", i + 1, ranking[i].nome, ranking[i].niveis),
                                130 + i * 45, 28, i == 0 ? GOLD : WHITE);
            }
            textoCentralizado("Enter/Esc para voltar", 400, 18, GRAY);
        }
        else {
            for (int l = 0; l < LINHAS; l++) {
                for (int c = 0; c < COLUNAS; c++) {
                    int x = c * TAM_BLOCO, y = l * TAM_BLOCO;

                    if (mapa[l][c] == BLOCO) {
                        if (l == 0 || mapa[l - 1][c] != BLOCO)
                            DrawTexture(texTopo, x, y, WHITE);
                        else
                            DrawTexture(texMeio, x, y, WHITE);
                    }
                    else if (mapa[l][c] == PORTA && (l == 0 || mapa[l - 1][c] != PORTA)) {
                        DrawTexture(texPorta, x, y, WHITE);
                    }
                    else if (mapa[l][c] == ESPINHO) {
                        DrawTexture(texEspinho, x, y, WHITE);
                    }
                }
            }
            for (Plataforma *p = plataformas; p != NULL; p = p->prox) {
                if (p->tipo != TIPO_MOVEL) continue;
                for (int i = 0; i < p->larg; i++)
                    DrawTexture(texTopo, (int)p->x + i * TAM_BLOCO, (int)p->y, SKYBLUE);
            }

            if (temRelogio) {
                float flutua = sinf(GetTime() * 4) * 2;
                DrawTexture(texRelogio, (int)relogio.x, (int)(relogio.y + flutua), WHITE);
            }

            Rectangle origemJogador = { 0, 0, (float)texJogador.width, (float)texJogador.height };
            if (j.olhandoEsquerda) origemJogador.width = -origemJogador.width;
            DrawTextureRec(texJogador, origemJogador, (Vector2){ (int)j.corpo.x, (int)j.corpo.y }, WHITE);

            DrawText(TextFormat("Nivel: %d", nivel), 10, 10, 20, WHITE);
            DrawText(TextFormat("Tempo: %.1f", tempo), 10, 35, 20, tempo < 5 ? RED : WHITE);

            if (estado == ESTADO_GAMEOVER || estado == ESTADO_NOME) {
                DrawRectangle(0, 0, LARGURA, ALTURA, Fade(BLACK, 0.7f));
                desenharCentralizado(texGameOver, 30, 400);
                textoCentralizado(TextFormat("Niveis passados: %d", nivel), 215, 22, WHITE);

                if (estado == ESTADO_GAMEOVER) {
                    const char *textoSalvar = recordeSalvo ? "RECORDE SALVO" : (podeSalvar ? "SALVAR RECORDE" : "NAO ENTROU NO TOP 5");
                    const char *opcoes[3] = { "RECOMECAR", textoSalvar, "MENU" };

                    for (int i = 0; i < 3; i++) {
                        Color cor = (opcaoGameOver == i) ? COR_DESTAQUE : WHITE;
                        if (i == 1 && !podeSalvar) cor = (opcaoGameOver == i) ? COR_DESTAQUE_INATIVA : DARKGRAY;
                        textoCentralizado(opcoes[i], 260 + i * 35, 24, cor);
                    }
                    textoCentralizado("Setas/WASD para escolher, Enter para confirmar", 400, 16, GRAY);
                } else {
                    textoCentralizado("Digite seu nome", 255, 20, LIGHTGRAY);
                    for (int i = 0; i < 3; i++) {
                        int x = LARGURA / 2 - 60 + i * 40;
                        DrawText(TextFormat("%c", nome[i]), x + 8, 290, 36, i == letraAtual ? COR_DESTAQUE : WHITE);
                        if (i == letraAtual) DrawRectangle(x + 4, 330, 28, 4, COR_DESTAQUE);
                    }
                    textoCentralizado("Teclado escreve, Setas trocam a letra, Enter salva, Backspace cancela", 400, 16, GRAY);
                }
            }
        }
        EndTextureMode();

        BeginDrawing();
        ClearBackground(BLACK);

        float escala = fminf((float)GetScreenWidth() / LARGURA, (float)GetScreenHeight() / ALTURA);
        float w = LARGURA * escala;
        float h = ALTURA * escala;

        DrawTexturePro(tela.texture,
                       (Rectangle){ 0, 0, LARGURA, -ALTURA },
                       (Rectangle){ (GetScreenWidth() - w) / 2, (GetScreenHeight() - h) / 2, w, h },
                       (Vector2){ 0, 0 }, 0, WHITE);
        EndDrawing();
    }

    liberarLista(plataformas);
    liberarMapa(mapa);
    UnloadTexture(texFundo);
    UnloadTexture(texTopo);
    UnloadTexture(texMeio);
    UnloadTexture(texPorta);
    UnloadTexture(texJogador);
    UnloadTexture(texLogo);
    UnloadTexture(texGameOver);
    UnloadTexture(texRelogio);
    UnloadTexture(texEspinho);
    UnloadRenderTexture(tela);
    CloseWindow();
    return 0;
}