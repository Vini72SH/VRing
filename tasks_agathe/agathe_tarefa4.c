/* Programa: tarefa4.c
 * Autor: Agathe Rabasse
 * Data ultima modificação: 31/05/2026

Tarefa 4: Quando um processo correto testa outro processo correto obtém as informações
do estado dos demais processos do sistema, processos do sistema exceto aqueles que
testou nesta rodada, além do próprio testador.*/

#include <stdio.h>
#include <stdlib.h>
#include "smpl.h" // página da disciplina, com o makefile


// Vamos definer eventos: um processo em algum instante de tempo pode sofrer um evento
#define test 1
#define fault 2
#define recovery 3

// Vamos definer o descritor do processo

typedef struct{
	int id; // identificador de facility do SMPL. facility : objeto que está simulando (vento, etc.). aqui o objeto é o processo
	int *State; // vetor para armazenar o estado conhecido dos outros processos
} TipoProcesso;

TipoProcesso *processo;

// Estados dos processos
#define unknown -1
#define correto 0
#define falho 1

int main(int argc, char *argv[]) {
	static int N, // número de processos do sistema distribuído
		token, // indica o processo que está executando
		event, r, i,
		MaxTempoSimulac=120;

	static char fa_name[5]; // nome da facility

	if (argc!=2){
		puts("Uso correto: tempo <numero de processos>"); // printf que já salta línea etc
		exit(1);
	}

	N = atoi(argv[1]);

	smpl(0, "Meu primeiro programa de simulacao de sistemas distribuidos");
	reset();
	stream(1);

	// inicializar os N processos

	processo = (TipoProcesso *)malloc(sizeof(TipoProcesso)*N);

	for(i=0; i<N; i++) {
		memset(fa_name, '\0',5);
		sprintf(fa_name, "%d", i);
		processo[i].id= facility(fa_name,1);

		// inicializando o vetor state[N]
		processo[i].State = (int *)malloc(sizeof(int)*N);
		for(int j = 0; j < N; j++) {
			if (i == j) {
				processo[i].State[j] = 0;  // O próprio processo se considera correto
			} else {
				processo[i].State[j] = -1; // O estado dos outros é inicialmente desconhecido
			}
		}
	}

	// Vamos agora fazer o escalonamento dos eventos iniciais
	// No primeiro intervalo de testes os processos vão testar

	for (i=0; i<N; i++)
		schedule(test, 30.0, i); // todos os processos de 0 até N-1 vão testar na unidade de tempo 30. // funcao do smpl. para o tempo 30.0

	schedule(fault, 31.0, 1);
	schedule(recovery, 61.0, 1);

	// agora vem o loop principal do simulador

	puts("===============================================================");
	puts("           Sistemas Distribuídos Prof. Elias - Agathe Rabasse");
	puts("          LOG do Trabalho Prático 0, Tarefa 4");
	puts("      Quando um processo correto testa outro processo correto obtém \n as informações do estado dos demais processos do sistema exceto aqueles que \n testou nesta rodada, além do próprio testador.");
	printf("   Este programa foi executado para: N=%d processos.\n", N);
	printf("           Tempo Total de Simulação = %d\n", MaxTempoSimulac);
	puts("===============================================================");

	while (time() < MaxTempoSimulac){

		cause(&event, &token);
		switch(event) { // " eventos : testes, falha, recovery

			case test:
				if (status(processo[token].id) != 0) break; // se o processo que vai testar estiver falho não testa

			{
				int seguinte = (token + 1) % N; // o seguinte no anel

				// continua testando enquanto não der a volta completa
				while (seguinte != token) {
					if (status(processo[seguinte].id) == 0) { // Testar o status do vizinho com a função status() do SMPL
						processo[token].State[seguinte] = 0;

						printf("O processo %d testou o processo %d correto no tempo %4.1f\n", token, seguinte, time());

						// Fase de fofoca : copia apenas o que não testamos
						// Começamos a copiar a partir do processo logo após o seguinte
						int nao_testado = (seguinte + 1) % N;

						// Copia as informações até chegar de volta no token
						while (nao_testado != token) {
							processo[token].State[nao_testado] = processo[seguinte].State[nao_testado];
							nao_testado = (nao_testado + 1) % N;
						}

						break;

					} else { // vizinho falho
						processo[token].State[seguinte] = 1;
						printf("O processo %d testou o processo %d falho no tempo %4.1f\n", token, seguinte, time());
						seguinte = (seguinte + 1) % N;
					}
				}

				// se ele deu a volta no anel e chegou nele mesmo
				if (seguinte == token) {
					printf("O processo %d testou todo o anel e esta isolado no tempo %4.1f\n", token, time());
				}

				// mostre o vetor State[N]
				printf("Vetor State do processo %d: [ ", token);
				for(int k = 0; k < N; k++) {
					printf("%d ", processo[token].State[k]);
				}
				printf("]\n");
			}

				schedule(test, 30.0, token); // Agendar o próximo teste deste processo para daqui a 30 unidades de tempo
				break;

			case fault: r = request(processo[token].id, token, 0);
				printf("Socooorro!!! Sou o processo %d  e estou falhando no tempo %4.1f\n", token, time());
				break;

			case recovery: release(processo[token].id, token);
				printf("Viva!!! Sou o processo %d e acabo de recuperar no tempo %4.1f\n", token, time());
				schedule(test, 1.0, token);
				break;

		} // switch
	} // while

	// Liberação de memória
	for (i = 0; i < N; i++) {
		free(processo[i].State);
	}
	free(processo);

} // tempo.c


