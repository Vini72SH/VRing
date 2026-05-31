// Trabalho Prático 0, Tarefa 4
// Alunos: Vinícius Jeremias dos Santos
// Data de Modificação: 28/05/2026

#include <stdio.h>
#include <stdlib.h>

#include "smpl.h"

#define TEST 0
#define FAULT 1
#define RECOVERY 2
#define END 3

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

    if ((argc != 2) || atoi(argv[1]) < 5) {
        puts("Uso correto: ./task4 < número de processos >= 5 >");
        exit(1);
    }

    N = atoi(argv[1]);

    puts("===============================================================");
    puts("           Sistemas Distribuídos Prof. Elias");
    puts("          LOG do Trabalho Prático 0, Tarefa 4");
    puts(
        "Quando um processo correto testa outro processo correto obtém as\n"
        "       informações do estado dos demais processos do sistema\n"
        "       exceto aqueles que testou nesta rodada, além do próprio\n"
        "                               testador.\n");
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

    for (i = 0; i < N; i++) {
        schedule(TEST, 30.0, i);
    }

    schedule(END, MaxTempoSimulac, -1);

    schedule(FAULT, 60.0, 1);
    schedule(FAULT, 75.0, 4);
    schedule(FAULT, 90.0, 3);
    schedule(FAULT, 105.0, 2);

    while (time() < MaxTempoSimulac) {
        cause(&event, &token);
        switch (event) {
            case TEST:
                if (status(processo[token].id) != 0)
                    break;  // Se o processo está falho, não testa!

                // Processo testa até encontrar um processo correto ou testar
                // todos falhos. Atualiza sua tabela State com os processos
                // testados
                prox = (token + 1) % N;
                procStatus = status(processo[prox].id);
                while ((procStatus != 0) && (prox != token)) {
                    printf(
                        "O processo %d testou o processo %d suspeito no tempo "
                        "%4.1f\n",
                        token, prox, time());

                    processo[token].state[prox] = 1;
                    prox = (prox + 1) % N;
                    procStatus = status(processo[prox].id);
                }

                // Atualiza sua tabela State com as informações dos demais
                // processos não testados nessa rodada
                if (prox != token) {
                    printf(
                        "O processo %d testou o processo %d correto no tempo "
                        "%4.1f\n",
                        token, prox, time());
                    processo[token].state[prox] = 0;
                    atualizaState(token, prox, N);

                } else
                    printf("O processo %d é o único processo correto\n", token);

                // Impressão da tabela do processo
                printf("State Table do processo %d\n", token);
                for (i = 0; i < N; i++) {
                    printf("[%d] %d\n", i, processo[token].state[i]);
                }
                printf("\n");

                schedule(TEST, 30.0, token);
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
