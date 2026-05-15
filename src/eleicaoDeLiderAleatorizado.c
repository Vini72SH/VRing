#include <stdio.h>
#include <stdlib.h>

#include "fila.h"
#include "smpl.h"

#define TEST 0
#define FAULT 1
#define RECOVERY 2
#define ELEICAO_DE_LIDER 3
#define RECEIVE 4
#define END 5

// Definição de um print para debug
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
    int ack;      // Indica se a mensagem recebeu ack
    int criador;  // ID do processo criador da mensagem
    int origem;   // ID do processo de origem
    int destino;  // ID do processo destino
    int bit;      // Bit de candidatura do processo criador
    int epoch;    // Época da eleição
    int seq;      // Número de sequencia da mensagem
    float tempo;  // Quando a mensagem será recebida
} Mensagem;

typedef struct {
    int id;              // Id do processo
    int bit;             // Indica se o processo é candidato a ser líder
    int* state;          // Tabela de estados dos demais processos
    int* candidato;      // Vetor de candidatos
    int* receivedRound;  // Vetor de mensagens recebidas naquela rodada
    int proxProc;        // Id do próximo processo no anel
    int lider;           // Id do processo líder
    int epoch;           // Época da eleição atual

    int contador;      // Contador do número de sequência
    fila_t* msgs;      // Mensagens que pode receber dos outros processos
    fila_t* sentMsgs;  // Mensagens que enviou para outros processos
} TipoProcesso;

TipoProcesso* processo;

static int N;

int comparaMensagens(void* a, void* b) {
    Mensagem* msga = (Mensagem*)a;
    Mensagem* msgb = (Mensagem*)b;

    return ((msga->criador == msgb->criador) && (msga->seq == msgb->seq));
}

// Função que atualiza o vetor State de um processo
void atualizaState(int proc, int prox) {
    int it = (prox + 1) % N;

    while (it != proc) {
        vring_debug(
            "O processo %d obteve o estado do processo %d pelo processo %d\n",
            proc, it, prox);
        processo[proc].state[it] = processo[prox].state[it];
        it = (it + 1) % N;
    }
}

void imprimeState(int proc) {
    vring_debug("State Table do processo %d\n", proc);
    for (int i = 0; i < N; i++) {
        vring_debug("[%02d] %d\n", i, processo[proc].state[i]);
    }
    vring_debug("\n");
}

void geradorDeEventosFault() {
    for (int i = 0; i < N; i++) {
        if ((random() % 10 < 3)) {
            schedule(FAULT, 60.0, i);
        }
    }
}

// Primitiva para enviar uma mensagem
void send(int proc, int prox, Mensagem msg) {
    msg.ack = 0;
    msg.origem = proc;
    msg.destino = prox;
    msg.seq = processo[proc].contador++;
    msg.tempo = time() + 1;

    insere_fila(processo[proc].sentMsgs, &msg);
    insere_fila(processo[prox].msgs, &msg);

    schedule(RECEIVE, 1.0, prox);
}

// Primitiva para receber uma mensagem
int recv(int proc, Mensagem* msg) {
    float tempoAtual = time();

    Mensagem* aux = fila_head(processo[proc].msgs);
    while (aux != NULL) {
        if (aux->tempo <= tempoAtual) {
            memcpy(msg, aux, sizeof(Mensagem));
            return 1;
        }

        aux = fila_prox(processo[proc].msgs);
    }

    return 0;
}

void sendAck(int proc) {}

// Remove uma mensagem da fila de recebidas
void commit(int proc, Mensagem* msg) { fila_del(processo[proc].msgs, msg); }

int main(int argc, char* argv[]) {
#ifdef DEBUG
    printf("DEBUG mode is ON\n");
#else
    printf("DEBUG mode is OFF\n");
#endif

    static int ret, token, event, r, i, j, MaxTempoSimulac = 150;
    static char fa_name[5];

    int fault = 0;
    int op, prox = 0;
    int procStatus = 0;

    // Controle de estatísticas da eleição
    int estatisticasInicializadas = 0;
    float timestampEleicao = 0.0;
    int numMensagensEleicao = 0;
    int epocaEleicao = 0;  // Época global da eleição

    if (argc != 2) {
        puts("Uso correto: ./eleicaoDeLiderAleatorizado <número de processos>");
        exit(1);
    }

    N = atoi(argv[1]);

    puts("===============================================================");
    puts("           Sistemas Distribuídos Prof. Elias");
    puts("                LOG do Trabalho Prático");
    puts("              Vinícius Jeremias dos Santos");
    puts("             Última Modificação: 15/04/2026");
    puts("        Algoritmo de Eleição de Líder Aleatorizado");
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

        processo[i].bit = 0;

        processo[i].state = malloc(sizeof(int) * N);
        processo[i].candidato = malloc(sizeof(int) * N);
        processo[i].receivedRound = malloc(sizeof(int) * N);

        processo[i].contador = 0;
        processo[i].msgs = cria_fila(comparaMensagens, sizeof(Mensagem));
        processo[i].sentMsgs = cria_fila(comparaMensagens, sizeof(Mensagem));

        for (j = 0; j < N; j++) {
            processo[i].state[j] = -1;
        }

        processo[i].state[i] = 0;
        processo[i].proxProc = (i + 1) % N;
        processo[i].lider = -1;
        processo[i].epoch = 0;
    }

    printf("Nesta execução, processos podem falhar (incluindo o líder)?:\n");
    printf("1) Sim\n");
    printf("2) Não\n");
    printf("Selecione uma opção entre 1 e 2: ");
    scanf("%d", &op);

    if (op == 1) {
        printf("Gerando eventos de falha...\n");
        geradorDeEventosFault();
        fault = 1;  // Habilita a falha do líder
    }

    // Escalonamento dos eventos iniciais
    for (i = 0; i < N; i++) {
        schedule(TEST, 30.0, i);
        schedule(ELEICAO_DE_LIDER, 35.0, i);
    }

    schedule(END, MaxTempoSimulac, -1);

    while (time() < MaxTempoSimulac) {
        cause(&event, &token);
        switch (event) {
            case TEST:
                if (status(processo[token].id) != 0)
                    break;  // Se o processo está falho, não testa!

                // Testa até achar um processo correto ou todos falhos
                prox = (token + 1) % N;
                procStatus = status(processo[prox].id);
                while ((procStatus != 0) && (prox != token)) {
                    vring_debug(
                        "O processo %d testou o processo %d suspeito no tempo "
                        "%4.1f\n",
                        token, prox, time());

                    processo[token].state[prox] = 1;
                    processo[token].candidato[prox] = 0;
                    prox = (prox + 1) % N;
                    procStatus = status(processo[prox].id);
                }
                processo[token].proxProc = prox;

                // Atualiza sua tabela State com as informações dos demais
                // processos não testados nessa rodada
                if (prox != token) {
                    vring_debug(
                        "O processo %d testou o processo %d correto no tempo "
                        "%4.1f\n",
                        token, prox, time());
                    processo[token].state[prox] = 0;
                    atualizaState(token, prox);

                } else
                    vring_debug("O processo %d é o único processo correto\n",
                                token);

                imprimeState(token);

                // Se não recebeu ACK da mensagem enviada, envia novamente.
                /*
                if (processo[token].sent) {
                    if (processo[token].recievedAck == 0) {
                        printf(
                            "O processo %d enviou uma mensagem para o processo "
                            "%d e não recebeu "
                            "um ACK\n",
                            token, processo[token].buff.destino);

                        printf("Enviando a mensagem para %d\n", prox);
                        // send(token, prox, processo[token].buff);
                    }
                }
                */

                schedule(TEST, 30.0, token);
                break;

            case FAULT:
                if (status(processo[token].id) != 0)
                    break;  // O processo já está falho

                r = request(processo[token].id, token, 0);
                printf(
                    "\nSocorro!!! Sou o processo %d e estou falhando no tempo "
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

            case ELEICAO_DE_LIDER:
                if (status(processo[token].id) != 0)
                    break;  // Se o processo está falho, não participa da
                            // eleição!

                processo[token].epoch++;
                for (j = 0; j < N; j++) {
                    processo[token].candidato[j] = 0;
                    processo[token].receivedRound[j] = 0;
                }

                processo[token].bit = random() % 2;
                processo[token].candidato[token] = processo[token].bit;

                Mensagem novaMsg;
                novaMsg.criador = token;
                novaMsg.bit = processo[token].bit;
                novaMsg.epoch = processo[token].epoch;
                send(token, processo[token].proxProc, novaMsg);

                break;

            case RECEIVE:
                if (status(processo[token].id) != 0)
                    break;  // Se o processo está falho, não recebe mensagem!

                Mensagem recMsg;
                while (recv(token, &recMsg)) {
                    printf(
                        "O processo %d recebeu uma mensagem do processo %d\n",
                        token, recMsg.origem);
                    commit(token, &recMsg);

                    if (recMsg.criador != token) {
                        send(token, processo[token].proxProc, recMsg);
                    } else {
                        fila_del(processo[token].sentMsgs, &recMsg);
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
        free(processo[i].candidato);
        free(processo[i].receivedRound);
        destroi_fila(processo[i].sentMsgs);
        destroi_fila(processo[i].msgs);
    }

    free(processo);
}
