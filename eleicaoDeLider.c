#include <stdio.h>
#include <stdlib.h>

#include "smpl.h"

#define INICIO_RODADA 0
#define FIM_RODADA 1
#define TEST 2
#define FAULT 3
#define RECOVERY 4
#define ELEICAO_DE_LIDER 5
#define RECIEVE 6
#define END 7

#ifdef DEBUG
#define vring_debug(...)     \
    do {                     \
        printf("DEBUG: ");   \
        printf(__VA_ARGS__); \
    } while (0)
#else
#define vring_debug(...)  // não faz nada
#endif

typedef struct {
    int origem;
    int candidato;
    int epoch;
    int valida;
    float tempo;
} Mensagem;

typedef struct {
    int id;            // Id do processo
    int souCandidato;  // Indica se o processo é candidato a ser líder
    int* state;        // Tabela de estados dos demais processos
    int proxProc;      // Id do próximo processo no anel
    int lider;         // Id do processo líder
    int epoch;         // Época em que ele foi elegido
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

// Função que retorna 0 ou 1 aleatóriamente
int lideresAleatorios() { return (random() % 2); };

// Função que sempre retorna 1
int todosLideres() { return 1; }

// Função que atualiza o vetor State de um processo
void atualizaState(int proc, int prox, int N) {
    int it = (prox + 1) % N;

    while (it != proc) {
        vring_debug(
            "O processo %d obteve o estado do processo %d pelo processo %d\n",
            proc, it, prox);
        processo[proc].state[it] = processo[prox].state[it];
        it = (it + 1) % N;
    }
}

// Primitiva para enviar uma mensagem
void send(int proc, int prox, Mensagem msg) {
    msg.tempo = time() + 1;
    processo[prox].msg = msg;
    schedule(RECIEVE, 1.0, prox);
}

// Primitiva para receber uma mensagem
int recv(int proc, Mensagem* msg) {
    if (processo[proc].msg.valida == 0) return 0;

    if (processo[proc].msg.tempo > time()) return 0;

    memcpy(msg, &processo[proc].msg, sizeof(Mensagem));

    return 1;
}

// Primitiva para invalidar uma mensagem
void commit(int proc) { processo[proc].msg.valida = 0; }

int main(int argc, char* argv[]) {
#ifdef DEBUG
    printf("DEBUG mode is ON\n");
#else
    printf("DEBUG mode is OFF\n");
#endif

    static int ret, token, event, r, i, j, MaxTempoSimulac = 150;
    static char fa_name[5];

    int (*candidatura)();

    int op, prox = 0;
    int procStatus = 0;
    char* infoProc = "";
    Mensagem msg;

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
        processo[i].proxProc = (i + 1) % N;
        processo[i].msg.valida = 0;
        processo[i].lider = -1;
        processo[i].epoch = 0;
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
                vring_debug("Iniciando rodada de testes no tempo %4.1f\n\n",
                            time());
                for (i = 0; i < N; i++) {
                    schedule(TEST, 0.0, i);
                }
                schedule(FIM_RODADA, 0.0, -1);

                break;

            case FIM_RODADA:
                vring_debug("Final da rodada de testes\n\n");
                schedule(INICIO_RODADA, 30.0, -1);
                break;

            case TEST:
                if (status(processo[token].id) != 0)
                    break;  // Se o processo está falho, não testa!

                prox = token;

                do {
                    prox = (prox + 1) % N;
                    procStatus = status(processo[prox].id);
                    infoProc = (procStatus == 0) ? "correto" : "suspeito";

                    vring_debug(
                        "O processo %d testou o processo %d %s no tempo "
                        "%4.1f\n",
                        token, prox, infoProc, time());

                    processo[token].state[prox] = procStatus;

                } while ((procStatus != 0) && (prox != token));

                processo[token].proxProc = prox;
                if (prox != token) {
                    vring_debug(
                        "O processo %d está atualizando sua State Table com "
                        "informações do processo %d\n",
                        token, prox);
                    atualizaState(token, prox, N);
                } else
                    printf("O processo %d é o único processo correto\n", token);

                vring_debug("State Table do processo %d\n", token);
                for (i = 0; i < N; i++) {
                    vring_debug("[%d] %d\n", i, processo[token].state[i]);
                }
                vring_debug("\n");
                break;

            case FAULT:
                r = request(processo[token].id, token, 0);
                vring_debug(
                    "Socorro!!! Sou o processo %d e estou falhando no tempo "
                    "%4.1f\n",
                    token, time());
                break;

            case RECOVERY:
                release(processo[token].id, token);
                vring_debug(
                    "Viva!!! Sou o processo %d e acabo de recuperar no tempo "
                    "%4.1f\n",
                    token, time());
                schedule(TEST, 1.0, token);
                break;

            case ELEICAO_DE_LIDER:
                printf(
                    "O processo %d está se preparando para eleger um líder no "
                    "tempo %4.1f\n",
                    token, time());

                ret = recv(token, &msg);

                // Processo está iniciando uma eleição de líder
                if (ret == 0) {
                    processo[token].epoch++;
                    processo[token].souCandidato = candidatura();
                    if (processo[token].souCandidato) {
                        printf("O processo %d é candidato a líder\n", token);
                        msg.origem = token;
                        msg.valida = 1;
                        msg.epoch = processo[token].epoch;
                        msg.candidato = token;

                        send(token, processo[token].proxProc, msg);
                        processo[token].lider = token;
                    } else {
                        processo[i].lider = -1;
                    }
                }

                // Recebeu uma nova mensagem de eleição de líder, ou seja, o
                // líder anterior falhou
                if ((ret == 1) && (msg.epoch > processo[token].epoch)) {
                }

                break;

            case RECIEVE:
                ret = recv(token, &msg);

                // Recebeu uma mensagem
                if (ret == 1) {
                    printf(
                        "\nSou o processo %d e recebi uma mensagem do processo "
                        "%d "
                        "no "
                        "tempo %4.1f\n",
                        token, msg.origem, time());
                    // Está participando da eleição de líder atual
                    if (msg.epoch <= processo[token].epoch) {
                        // Encontrou um líder com ID maior
                        if (msg.candidato > processo[token].lider) {
                            printf(
                                "O processo %d mudou do candidato %d para "
                                "o "
                                "candidato %d e enviara a mensagem para o "
                                "processo %d\n",
                                token, processo[token].lider, msg.candidato,
                                processo[token].proxProc);
                            processo[token].lider = msg.candidato;
                            processo[token].epoch = msg.epoch;
                            msg.origem = token;

                            send(token, processo[token].proxProc, msg);

                        } else if (msg.candidato < processo[token].lider) {
                            // O líder para esse processo tem ID maior do
                            // que o sugerido
                            printf(
                                "O candidato sugerido %d não possui ID "
                                "maior "
                                "que o do candidato %d\n",
                                msg.candidato, processo[token].lider);

                        } else {
                            // Recebeu a própria mensagem, logo, todos os
                            // processos sabem que é o processo líder
                            printf("O processo %d é o líder\n", token);
                        }
                        commit(token);

                    } else {
                        // Esta desatualizado e precisa eleger um novo líder
                    }
                }

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
