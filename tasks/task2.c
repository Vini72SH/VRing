#include <stdio.h>
#include <stdlib.h>

#include "smpl.h"

#define TEST 1
#define FAULT 2
#define RECOVERY 3
#define END 4

typedef struct {
    int id;
} TipoProcesso;

TipoProcesso* processo;

int main(int argc, char* argv[]) {
    static int N, token, event, r, i, MaxTempoSimulac = 150;
    static char fa_name[5];

    int prox = 0;
    int procStatus = 0;
    char* infoProc = "";

    if ((argc != 2) || atoi(argv[1]) < 5) {
        puts("Uso correto: ./task2 < número de processos >= 5 >");
        exit(1);
    }

    N = atoi(argv[1]);

    puts("===============================================================");
    puts("           Sistemas Distribuídos Prof. Elias");
    puts("          LOG do Trabalho Prático 0, Tarefa 2");
    puts(
        "Cada processo correto executa testes até achar outro processo\n"
        "correto. Lembre-se de tratar o caso em que todos os demais processos\n"
        "       estão falhos. Imprimir os testes e resultados.\n");
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
    }

    // Vamos fazer o escalonamento inicial de eventos

    // Nossos processos vão executar testes em intervalos de testes
    // O intervalo de testes vai ser de 30 unidades de tempo
    // A simulação começa no tempo 0 (zero) e vamos escalonar o primeiro teste
    // de todos os processos para o tempo 30.0

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

                prox = token;

                // Processo testa até encontrar um processo correto ou testar
                // todos falhos
                do {
                    prox = (prox + 1) % N;
                    procStatus = status(processo[prox].id);
                    infoProc = (procStatus == 0) ? "correto" : "suspeito";

                    printf(
                        "O processo %d testou o processo %d %s no tempo "
                        "%4.1f\n",
                        token, prox, infoProc, time());

                } while ((procStatus != 0) && (prox != token));

                if (prox == token)
                    printf("O processo %d é o único processo correto\n", token);

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

    free(processo);
}
