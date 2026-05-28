// Autores:
//      Vinícius Jeremias dos Santos
//      Agathe My Anh Rabasse
// Data de Modificação: 27/05/2026

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fila.h"
#include "smpl.h"

#define TEST 0
#define FAULT 1
#define RECOVERY 2
#define ELEICAO_DE_LIDER 3
#define NOVA_RODADA 4
#define RECEIVE 5
#define END 6

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
    int rodada;   // Rodada atual da eleição
    int epoch;    // Época da eleição
    int seq;      // Número de sequencia da mensagem
    float tempo;  // Quando a mensagem será recebida
} Mensagem;

typedef struct {
    int id;                  // Id do processo
    int bit;                 // Indica se o processo é candidato a ser líder
    int* state;              // Tabela de estados dos demais processos
    int* candidato;          // Vetor de candidatos
    int* receivedRound;      // Vetor de mensagens recebidas naquela rodada
    int proxProc;            // Id do próximo processo no anel
    int lider;               // Id do processo líder
    int epoch;               // Época da eleição atual
    int rodada;              // Rodada da eleição atual
    int novaRodada;          // Define que vai começar uma nova rodada
    int att;                 // Indica se é necessário se atualizar
    int recebendoMensagens;  // Habilita recepção de mensagens
    int epochAgendamento;    // Armazena a época da eleição agendada para evitar
    // múltiplos escalonamentos simultâneos

    int contador;      // Contador do número de sequência
    fila_t* msgs;      // Mensagens que pode receber dos outros processos
    fila_t* sentMsgs;  // Mensagens que enviou para outros processos
    int* acks;         // Contador de acks que enviou para cada processo
    Mensagem buff;     // Mensagem de buffer para armazenar a mensagem recebida
                       // enquanto se atualiza
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

// Gera eventos de falha para os processos, com uma probabilidade de 30%
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
    msg.tempo = time() + 1;

    insere_fila(processo[proc].sentMsgs, &msg);
    insere_fila(processo[prox].msgs, &msg);

    schedule(RECEIVE, 1.0, prox);
}

// Reenvia uma mensagem para o próximo processo no anel, não insere a mensagem
// na fila de enviadas, pois ela já está lá, apenas a reenvia para o próximo
// processo
void resend(int proc, int prox, Mensagem msg) {
    msg.ack = 0;
    msg.origem = proc;
    msg.destino = prox;
    msg.tempo = time() + 1;

    insere_fila(processo[prox].msgs, &msg);
    schedule(RECEIVE, 1.0, prox);
}

// Envia um ACK para a mensagem recebida, indicando que ela foi tratada e pode
// ser removida do anel
void sendAck(Mensagem msg) {
    msg.ack = 1;

    if (processo[msg.destino].acks[msg.criador] <= msg.seq) {
        processo[msg.destino].acks[msg.criador]++;
    }

    atualiza_elemento(processo[msg.origem].sentMsgs, &msg, &msg);
}

// Remove uma mensagem da fila de recebidas
void commit(int proc, Mensagem* msg) { fila_del(processo[proc].msgs, msg); }

// Primitiva para receber uma mensagem
int recv(int proc, Mensagem* msg) {
    int ret;
    float tempoAtual = time();

    Mensagem aux;
    ret = fila_head(processo[proc].msgs, &aux);

    while ((ret) && (aux.tempo <= tempoAtual)) {
        // Se a mensagem for válida, a coloca no ponteiro
        if (processo[proc].acks[aux.criador] == aux.seq) {
            memcpy(msg, &aux, sizeof(Mensagem));
            return 1;
        }

        // A mensagem já foi tratada e é descartada
        if (processo[proc].acks[aux.criador] > aux.seq) {
            sendAck(aux);
            commit(proc, &aux);
        }

        ret = fila_prox(processo[proc].msgs, &aux);
    }

    return 0;
}

// Verifica se há mensagens na fila de enviadas que não receberam ACK e as
// reenvia
void reenviaMensagens(int proc) {
    int ret;
    Mensagem msg;

    ret = fila_head(processo[proc].sentMsgs, &msg);
    while (ret) {
        if (msg.ack == 0) {
            printf(
                "O processo %d enviou uma mensagem para o processo "
                "%d e não recebeu "
                "um ACK\n",
                proc, msg.destino);
            resend(proc, processo[proc].proxProc, msg);
            printf("Enviando a mensagem para %d\n", processo[proc].proxProc);
        } else {
            fila_del(processo[proc].sentMsgs, &msg);
        }

        ret = fila_prox(processo[proc].sentMsgs, &msg);
    }
}

// Remove uma mensagem da fila de enviadas
void freeMsg(int proc, Mensagem msg) {
    fila_del(processo[proc].sentMsgs, &msg);
}

int main(int argc, char* argv[]) {
#ifdef DEBUG
    printf("DEBUG MODE is ON\n");
#else
    printf("DEBUG MODE is OFF\n");
#endif

    static int ret, token, event, i, j, MaxTempoSimulac = 1000;
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
        fprintf(stderr,
                "Uso correto: ./eleicaoDeLiderAleatorizado <número de "
                "processos>\n");
        exit(EXIT_FAILURE);
    }

    N = atoi(argv[1]);
    if (N <= 0) {
        fprintf(stderr, "Número de processos deve ser um inteiro positivo.\n");
        return EXIT_FAILURE;
    }

    puts("===============================================================");
    puts("           Sistemas Distribuídos Prof. Elias");
    puts("                LOG do Trabalho Prático");
    puts("   Vinícius Jeremias dos Santos e Agathe My Anh Rabasse");
    puts("             Última Modificação: 27/05/2026");
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

        processo[i].acks = malloc(sizeof(int) * N);

        for (j = 0; j < N; j++) {
            processo[i].state[j] = -1;
            processo[i].acks[j] = 0;
            processo[i].candidato[j] = 0;
            processo[i].receivedRound[j] = 0;
        }

        processo[i].state[i] = 0;
        processo[i].proxProc = (i + 1) % N;
        processo[i].lider = -1;
        processo[i].epoch = 0;
        processo[i].rodada = 0;
        processo[i].novaRodada = 0;
        processo[i].att = 0;
        processo[i].recebendoMensagens = 1;
        processo[i].epochAgendamento = 0;
    }

    printf("Nesta execução, processos podem falhar (incluindo o líder)?:\n");
    printf("1) Sim\n");
    printf("2) Não\n");
    printf("Selecione uma opção entre 1 e 2: ");

    if (scanf("%d", &op) != 1 || (op != 1 && op != 2)) {
        fprintf(stderr,
                "Entrada inválida. Digite 1 para habilitar falhas ou 2 para "
                "desabilitar.\n");
        return EXIT_FAILURE;
    }

    printf("Opção de falha selecionada: %d\n", op);

    if (op == 1) {
        printf("Gerando eventos de falha...\n");
        geradorDeEventosFault();
        fault = 1;  // Habilita a falha do líder
    }

    // Escalonamento dos eventos iniciais
    for (i = 0; i < N; i++) {
        schedule(TEST, 30.0, i);
        processo[i].epochAgendamento = processo[i].epoch + 1;
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
                        "O processo %02d testou o processo %02d suspeito no "
                        "tempo "
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
                        "O processo %02d testou o processo %02d correto no "
                        "tempo "
                        "%4.1f\n",
                        token, prox, time());
                    processo[token].state[prox] = 0;
                    atualizaState(token, prox);

                } else
                    vring_debug("O processo %02d é o único processo correto\n",
                                token);

                imprimeState(token);

                // Se não recebeu ACK das mensagens enviadas, envia novamente.
                reenviaMensagens(token);

                if ((processo[token].lider >= 0) &&
                    (processo[token].state[processo[token].lider] > 0)) {
                    printf(
                        "\nO processo %02d detectou que o líder %02d falhou no "
                        "tempo %4.1f e irá iniciar uma nova eleição\n",
                        token, processo[token].lider, time());

                    processo[token].epochAgendamento =
                        processo[token].epoch + 1;
                    schedule(ELEICAO_DE_LIDER, 1.0, token);
                }

                // Verificar se todos os processos corretos responderam nessa
                // rodada, escalonando uma nova rodada caso ainda não tenha sido
                // escalonada
                int todosResponderam = 1;
                for (i = 0; i < N; i++) {
                    if ((processo[token].state[i] != 1) &&
                        (processo[token].receivedRound[i] == 0)) {
                        todosResponderam = 0;
                        break;
                    }
                }

                if ((todosResponderam) && (processo[token].novaRodada == 0) &&
                    (processo[token].lider == -1)) {
                    processo[token].novaRodada = 1;
                    schedule(NOVA_RODADA, 1.0, token);
                }

                // Se há mensagens na fila de recebidas, escalona um evento de
                // tratamento de mensagens
                Mensagem msg;
                if (recv(token, &msg)) schedule(RECEIVE, 1.0, token);

                schedule(TEST, 30.0, token);
                break;

            case FAULT:
                if (status(processo[token].id) != 0)
                    break;  // O processo já está falho

                request(processo[token].id, token, 0);
                printf(
                    "\nSocorro!!! Sou o processo %02d e estou falhando no "
                    "tempo "
                    "%4.1f\n",
                    token, time());
                break;

            case RECOVERY:
                release(processo[token].id, token);
                printf(
                    "Viva!!! Sou o processo %02d e acabo de recuperar no tempo "
                    "%4.1f\n",
                    token, time());
                schedule(TEST, 1.0, token);
                break;

            case ELEICAO_DE_LIDER:
                if (status(processo[token].id) != 0)
                    break;  // Se o processo está falho, não participa da
                            // eleição!
                if (processo[token].epoch >= processo[token].epochAgendamento)
                    break;

                // Caso seja necessário se atualizar utilizando os dados da
                // mensagem recebida
                ret = 0;
                if (processo[token].att) {
                    printf(
                        "O processo %02d irá se atualizar para a eleição de "
                        "líder\n",
                        token);
                    processo[token].att = 0;
                    processo[token].recebendoMensagens = 1;
                    if (processo[token].lider >= 0)
                        processo[token].state[processo[token].lider] = -1;
                    ret = 1;
                }

                printf(
                    "O processo %02d está se preparando para eleger um líder "
                    "no tempo %4.1f\n",
                    token, time());

                // Inicialização das variáveis
                processo[token].lider = -1;
                processo[token].epoch++;
                processo[token].rodada = 1;
                for (j = 0; j < N; j++) {
                    processo[token].candidato[j] = 0;
                    processo[token].receivedRound[j] = 0;
                }
                processo[token].receivedRound[token] = 1;

                processo[token].bit = random() % 2;
                processo[token].candidato[token] = processo[token].bit;

                if (processo[token].bit)
                    printf("O processo %02d é candidato a líder\n", token);
                else
                    printf("O processo %02d não é candidato a líder\n", token);

                // O processo recebeu uma mensagem de uma época posterior,
                // é necessário manter a consistência atualizando o vetor de
                // candidatos com as informações sobre o processo que criou
                // essa mensagem
                if (ret) {
                    processo[token].epoch = processo[token].buff.epoch;
                    processo[token].rodada = processo[token].buff.rodada;
                    processo[token].candidato[processo[token].buff.criador] =
                        processo[token].buff.bit;
                    processo[token]
                        .receivedRound[processo[token].buff.criador] = 1;
                }

                // Envia uma mensagem no anel avisando a todos sobre seu status
                // nessa eleição
                Mensagem novaMsg;
                novaMsg.criador = token;
                novaMsg.bit = processo[token].bit;
                novaMsg.rodada = 1;
                novaMsg.epoch = processo[token].epoch;
                novaMsg.seq = processo[token].contador++;

                printf("Enviando a mensagem dessa eleição para %02d\n",
                       processo[token].proxProc);
                send(token, processo[token].proxProc, novaMsg);

                break;

            case NOVA_RODADA:
                if (status(processo[token].id) != 0)
                    break;  // Se o processo está falho, não participa

                printf(
                    "\nFIM DA RODADA %02d PARA O PROCESSO %02d NO TEMPO "
                    "%4.1f\n",
                    processo[token].rodada, token, time());
                printf("Vetor de Bits do Processo %02d\n", token);
                printf("[ ");
                for (int i = 0; i < N; i++) {
                    printf("%d ", processo[token].candidato[i]);
                }
                printf("]\n");

                // Caso seja necessário se atualizar utilizando os dados da
                // mensagem recebida
                ret = 0;
                if (processo[token].att) {
                    processo[token].att = 0;
                    processo[token].recebendoMensagens = 1;
                    ret = 1;
                }

                processo[token].rodada++;
                processo[token].novaRodada = 0;

                // Verificação do número de candidatos e de quem é o líder
                int lider = -1;
                int candidatos = 0;
                for (i = 0; i < N; i++) {
                    if (processo[token].candidato[i]) {
                        if (candidatos == 0) lider = i;
                        candidatos++;
                    }
                }

                if (candidatos == 0) {
                    printf(
                        "O processo %02d detectou que não há nenhum "
                        "candidato "
                        "a "
                        "líder\n",
                        token);
                    processo[token].epochAgendamento =
                        processo[token].epoch + 1;
                    schedule(ELEICAO_DE_LIDER, 1.0, token);

                } else if (candidatos == 1) {
                    // Um líder foi eleito

                    if (processo[token].lider == -1) {
                        Mensagem msgFinal;

                        msgFinal.criador = token;
                        msgFinal.bit = (lider == token) ? 1 : 0;
                        msgFinal.rodada = processo[token].rodada;
                        msgFinal.epoch = processo[token].epoch;
                        msgFinal.seq = processo[token].contador++;

                        processo[token].lider = lider;
                        if (lider != token)
                            printf(
                                "O processo %02d considera o processo %02d o "
                                "líder\n",
                                token, lider);
                        else {
                            printf("Eu, o processo %02d, sou o líder!\n",
                                   token);
                            if (fault) {
                                schedule(FAULT, 30.0, token);
                                fault = 0;
                            }
                        }

                        printf("Enviando a mensagem dessa rodada para %d\n",
                               processo[token].proxProc);
                        send(token, processo[token].proxProc, msgFinal);
                    }

                } else {
                    // Há mais de 1 processo concorrendo, é necessário uma nova
                    // rodada de mensagens
                    printf(
                        "O processo %02d está iniciando uma nova rodada no "
                        "tempo "
                        "%4.1f\n",
                        token, time());

                    // Reinicialização das variáveis utilizadas
                    for (i = 0; i < N; i++) {
                        processo[token].candidato[i] = 0;
                        processo[token].receivedRound[i] = 0;
                    }
                    processo[token].receivedRound[token] = 1;

                    if (processo[token].bit) processo[token].bit = random() % 2;

                    if (processo[token].bit) {
                        printf("O processo %02d ainda é candidato a líder\n",
                               token);
                        processo[token].candidato[token] = 1;
                    } else {
                        printf("O processo %02d não é candidato a líder\n",
                               token);
                    }

                    // O processo recebeu uma mensagem de uma rodada posterior,
                    // é necessário manter a consistência atualizando o vetor de
                    // candidatos com as informações sobre o processo que criou
                    // essa mensagem
                    if (ret) {
                        processo[token].rodada = processo[token].buff.rodada;
                        processo[token]
                            .candidato[processo[token].buff.criador] =
                            processo[token].buff.bit;
                        processo[token]
                            .receivedRound[processo[token].buff.criador] = 1;
                    }

                    // Gera uma nova mensagem sobre seu status dessa rodada e a
                    // envia no anel
                    Mensagem novaMsgRodada;
                    novaMsgRodada.criador = token;
                    novaMsgRodada.bit = processo[token].bit;
                    novaMsgRodada.rodada = processo[token].rodada;
                    novaMsgRodada.epoch = processo[token].epoch;
                    novaMsgRodada.seq = processo[token].contador++;

                    printf(
                        "Enviando a mensagem dessa rodada para o processo "
                        "%02d\n",
                        processo[token].proxProc);
                    send(token, processo[token].proxProc, novaMsgRodada);
                }

                break;

            case RECEIVE:
                if (status(processo[token].id) != 0)
                    break;  // Se o processo está falho, não recebe mensagem!

                Mensagem recMsg;
                while (recv(token, &recMsg) &&
                       (processo[token].recebendoMensagens == 1)) {
                    // Recebeu a própria mensagem, portanto, a retira do anel
                    if (recMsg.criador == token) {
                        printf(
                            "\nO processo %02d recebeu a própria mensagem, a "
                            "retirando do anel no tempo %4.1f\n",
                            token, time());
                        sendAck(recMsg);
                        commit(token, &recMsg);
                        freeMsg(token, recMsg);
                        continue;
                    }

                    printf(
                        "\nSou o processo %02d e recebi uma mensagem do "
                        "processo %02d\n"
                        "Sobre a candidatura do processo %02d na "
                        "rodada %02d da eleição %02d no tempo %4.1f\n",
                        token, recMsg.origem, recMsg.criador, recMsg.rodada,
                        recMsg.epoch, time());

                    // Essa mensagem pertence a eleição e rodada atuais
                    if (recMsg.epoch == processo[token].epoch) {
                        if (recMsg.rodada == processo[token].rodada) {
                            processo[token].candidato[recMsg.criador] =
                                recMsg.bit;
                            processo[token].receivedRound[recMsg.criador] = 1;

                            send(token, processo[token].proxProc, recMsg);
                            printf("Enviando mensagem para o processo %02d\n",
                                   processo[token].proxProc);
                        } else if (recMsg.rodada < processo[token].rodada) {
                            // Pertence a uma rodada anterior
                            printf(
                                "A mensagem pertence a uma rodada anterior, "
                                "enviando para %02d\n",
                                processo[token].proxProc);
                            send(token, processo[token].proxProc, recMsg);
                        } else {
                            // Pertence a uma rodada posterior
                            printf(
                                "A mensagem pertence a uma rodada futura, "
                                "atualizando e enviando para %02d\n",
                                processo[token].proxProc);
                            processo[token].novaRodada = 1;
                            processo[token].recebendoMensagens = 0;
                            processo[token].buff = recMsg;
                            processo[token].att = 1;
                            schedule(NOVA_RODADA, 5.0, token);
                            send(token, processo[token].proxProc, recMsg);
                        }

                    } else if (recMsg.epoch < processo[token].epoch) {
                        // Pertence a uma eleição anterior
                        printf(
                            "A mensagem pertence a uma eleição anterior, "
                            "enviando para %02d\n",
                            processo[token].proxProc);
                        send(token, processo[token].proxProc, recMsg);

                    } else {
                        // Pertence a uma eleição posterior
                        printf(
                            "A mensagem pertence a uma eleição futura, "
                            "atualizando e enviando para %02d\n",
                            processo[token].proxProc);
                        processo[token].recebendoMensagens = 0;
                        processo[token].buff = recMsg;
                        processo[token].att = 1;
                        processo[token].epochAgendamento = recMsg.epoch;
                        send(token, processo[token].proxProc, recMsg);
                        schedule(ELEICAO_DE_LIDER, 5.0, token);
                    }

                    commit(token, &recMsg);
                    sendAck(recMsg);
                }

                int recebeuTodasAsMensagens = 1;
                for (i = 0; i < N; i++) {
                    if ((processo[token].state[i] != 1) &&
                        (processo[token].receivedRound[i] == 0)) {
                        recebeuTodasAsMensagens = 0;
                        break;
                    }
                }

                if ((recebeuTodasAsMensagens) &&
                    (processo[token].novaRodada == 0) &&
                    (processo[token].att == 0) &&
                    (processo[token].lider == -1)) {
                    printf(
                        "O processo %02d detectou que todos os corretos "
                        "responderam no tempo %4.1f\n",
                        token, time());
                    processo[token].novaRodada = 1;
                    schedule(NOVA_RODADA, 5.0, token);
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
        destroi_fila(&processo[i].sentMsgs);
        destroi_fila(&processo[i].msgs);
        free(processo[i].acks);
    }

    free(processo);
}
