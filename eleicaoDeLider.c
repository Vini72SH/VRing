#include <stdio.h>
#include <stdlib.h>

#include "smpl.h"

#define INICIO_RODADA 0
#define FIM_RODADA 1
#define RECIEVE 2
#define TEST 3
#define FAULT 4
#define RECOVERY 5
#define END 6
#define ELEICAO_DE_LIDER 7

typedef struct {
    int candidato;
    int valida;
} Mensagem;

typedef struct {
    int id;            // Id do processo
    int souCandidato;  // Indica se o processo é candidato a ser líder
    int* state;        // Tabela de estados dos demais processos
    int lider;
    Mensagem
        msg;  // Estrutura de uma mensagem que pode receber de outros processos
} TipoProcesso;

TipoProcesso* processo;

static int N;

// Função para sortear 1 apenas uma vez a cada N tentativas
int umLider() {
    static int cont = 0;
    static int sorteado = -1;
    static int contador_ciclos = 0;

    if (cont >= N || sorteado == -1) {
        cont = 0;
        sorteado = rand() % N;
        contador_ciclos++;
    }

    int resultado = (cont == sorteado) ? 1 : 0;
    cont++;

    return resultado;
}

int lideresAleatorios() { return (random() % 2); };

int todosLideres() { return 1; }

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
    static int token, event, r, i, j, MaxTempoSimulac = 150;
    static char fa_name[5];

    int (*candidatura)();

    int op, prox = 0;
    int procStatus = 0;
    char* infoProc = "";

    if (argc != 2) {
        puts("Uso correto: ./eleicaoDeLider <número de processos>");
        exit(1);
    }

    N = atoi(argv[1]);

    puts("===============================================================");
    puts("           Sistemas Distribuídos Prof. Elias");
    puts("                LOG do Trabalho Prático");
    puts("              Algoritmos de Eleição de Líder");
    printf("   Este programa foi executado para: N=%d processos.\n", N);
    printf("           Tempo Total de Simulação = %d\n", MaxTempoSimulac);
    puts("===============================================================");

    smpl(0, "Eleição de Líder");
    reset();
    stream(1);

    // Inicialização dos processos
    processo = (TipoProcesso*)malloc(sizeof(TipoProcesso) * N);

    for (i = 0; i < N; i++) {
        memset(fa_name, '\0', 5);
        sprintf(fa_name, "%d", i);
        processo[i].id = facility(fa_name, 1);
        processo[i].souCandidato = 0;
        processo[i].state = malloc(sizeof(int) * N);

        for (j = 0; j < N; j++) {
            processo[i].state[j] = -1;
        }
        processo[i].state[i] = 0;
        processo[i].msg.valida = 0;
        processo[i].lider = -1;
    }

    printf("Nesta execução, a eleição de líder deve ser com:\n");
    printf("1) 1 candidato aleatório\n");
    printf("2) Um número aleatório de candidatos\n");
    printf("3) Todos os processos são candidatos\n");
    printf("Selecione uma opção entre 1 e 3: ");
    scanf("%d", &op);

    // Definição de candidatos
    switch (op) {
        case 1:
            candidatura = umLider;
            break;

        case 2:
            candidatura = lideresAleatorios;
            break;

        case 3:
            candidatura = todosLideres;
            break;
    }

    // Escalonamento dos eventos iniciais

    for (i = 0; i < N; i++) schedule(ELEICAO_DE_LIDER, 35.0, i);

    schedule(INICIO_RODADA, 30.0, -1);
    schedule(END, MaxTempoSimulac, -1);

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

            case RECIEVE:
                printf(
                    "Sou o processo %d e recebi uma mensagem no tempo %4.1f\n",
                    token, time());
                break;

            case FIM_RODADA:
                printf("Final da rodada de testes\n\n");
                schedule(INICIO_RODADA, 30.0, -1);
                break;

            case END:
                printf("Fim de Simulação no tempo %4.1f...\n", time());
                break;

            case ELEICAO_DE_LIDER:
                printf(
                    "O processo %d está se preparando para eleger um líder no "
                    "tempo %4.1f\n",
                    token, time());
                processo[token].souCandidato = candidatura();
                if (processo[token].souCandidato) {
                    Mensagem msg;
                    msg.valida = 1;
                    msg.candidato = token;
                    // Mandar mensagem para o próximo no anel
                    processo[token].lider = token;
                } else {
                    processo[i].lider = -1;
                }

                break;
        }
    }

    for (i = 0; i < N; i++) {
        free(processo[i].state);
    }

    free(processo);
}
