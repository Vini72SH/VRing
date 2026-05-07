#include <stdio.h>
#include <stdlib.h>

#include "smpl.h"

#define INICIO_RODADA 0
#define FIM_RODADA 1
#define TEST 2
#define FAULT 3
#define RECOVERY 4
#define END 5

typedef struct {
    int id;
    int* state;
} TipoProcesso;

TipoProcesso* processo;

void atualizaState(int proc, int prox, int N) {
    int it = (prox + 1) % N;

    while (it != proc) {
        printf(
            "O processo %d obteve o estado do processo %d pelo processo %d\n",
            proc, it, prox);
        processo[proc].state[it] = processo[prox].state[it];
        it = (it + 1) % N;
    }
}

int main(int argc, char* argv[]) {
    static int N, token, event, r, i, j, MaxTempoSimulac = 150;
    static char fa_name[5];

    int prox = 0;
    int procStatus = 0;
    char* infoProc = "";

    if ((argc != 2) || atoi(argv[1]) < 5) {
        puts("Uso correto: %s tempo < número de processos >= 5 >");
        exit(1);
    }

    N = atoi(argv[1]);

    puts("===============================================================");
    puts("           Sistemas Distribuídos Prof. Elias");
    puts("          LOG do Trabalho Prático 0, Tarefa 4");
    puts(
        "Quando um processo correto testa outro processo correto obtém as\n"
        "informações do estado dos demais processos do sistema, processos do\n"
        "sistema exceto aqueles que testou nesta rodada, além do próprio\n"
        "testador.\n");
    printf("   Este programa foi executado para: N=%d processos.\n", N);
    printf("           Tempo Total de Simulação = %d\n", MaxTempoSimulac);
    puts("===============================================================");

    smpl(0, "Um Exemplo de Simulação");
    reset();
    stream(1);

    processo = (TipoProcesso*)malloc(sizeof(TipoProcesso) * N);

    for (i = 0; i < N; i++) {
        memset(fa_name, '\0', 5);
        sprintf(fa_name, "%d", i);
        processo[i].id = facility(fa_name, 1);
        processo[i].state = malloc(sizeof(int) * N);

        for (j = 0; j < N; j++) {
            processo[i].state[j] = -1;
        }
        processo[i].state[i] = 0;
    }

    schedule(INICIO_RODADA, 30.0, -1);

    schedule(END, MaxTempoSimulac, -1);

    schedule(FAULT, 60.0, 1);
    schedule(FAULT, 75.0, 4);
    schedule(FAULT, 90.0, 3);
    schedule(FAULT, 105.0, 2);

    while (time() < MaxTempoSimulac) {
        cause(&event, &token);
        switch (event) {
            case INICIO_RODADA:
                printf("Iniciando rodada de testes no tempo %4.1f\n\n", time());
                for (i = 0; i < N; i++) {
                    schedule(TEST, 0.0, i);
                }
                schedule(FIM_RODADA, 0.0, -1);

                break;

            case TEST:
                if (status(processo[token].id) != 0)
                    break;  // Se o processo está falho, não testa!

                prox = token;

                do {
                    prox = (prox + 1) % N;
                    procStatus = status(processo[prox].id);
                    infoProc = (procStatus == 0) ? "correto" : "suspeito";

                    printf(
                        "O processo %d testou o processo %d %s no tempo "
                        "%4.1f\n",
                        token, prox, infoProc, time());

                    processo[token].state[prox] = procStatus;

                } while ((procStatus != 0) && (prox != token));

                if (prox != token) {
                    printf(
                        "O processo %d está atualizando sua State Table com "
                        "informações do processo %d\n",
                        token, prox);
                    atualizaState(token, prox, N);
                } else
                    printf("O processo %d é o único processo correto\n", token);

                printf("State Table do processo %d\n", token);
                for (i = 0; i < N; i++) {
                    printf("[%d] %d\n", i, processo[token].state[i]);
                }
                printf("\n");
                break;

            case FAULT:
                r = request(processo[token].id, token, 0);
                printf(
                    "Socorro!!! Sou o processo %d e estou falhando no tempo "
                    "%4.1f\n",
                    token, time());
                break;

            case RECOVERY:
                release(processo[token].id, token);
                printf(
                    "Viva!!! Sou o processo %d e acabo de recuperar no tempo "
                    "%4.1f\n",
                    token, time());
                schedule(TEST, 1.0, token);
                break;

            case FIM_RODADA:
                printf("Final da rodada de testes\n\n");
                schedule(INICIO_RODADA, 30.0, -1);
                break;

            case END:
                printf("Fim de Simulação no tempo %4.1f...\n", time());
                break;
        }
    }

    for (i = 0; i < N; i++) {
        free(processo[i].state);
    }

    free(processo);
}
