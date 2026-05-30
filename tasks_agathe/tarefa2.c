/* Programa: tarefa2.c
 * Autor: Agathe Rabasse
 * Data ultima modificação: 30/05/2026

Tarefa 2 : Cada processo correto executa testes até achar outro processo correto.
Lembre-se de tratar o caso em que todos os demais processos estão falhos.
Imprimir os testes e resultados.*/

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
} TipoProcesso;

TipoProcesso *processo;

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
	puts("          LOG do Trabalho Prático 0, Tarefa 2");
	puts("      Cada processo correto executa testes até achar outro processo correto.");
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
						printf("O processo %d testou o processo %d correto no tempo %4.1f\n", token, seguinte, time());
						break;
					} else { // vizinho falho
						printf("O processo %d testou o processo %d falho no tempo %4.1f\n", token, seguinte, time());
						seguinte = (seguinte + 1) % N;
					}
				}

				// se ele deu a volta no anel e chegou nele mesmo
				if (seguinte == token) {
					printf("O processo %d testou todo o anel e esta isolado no tempo %4.1f\n", token, time());
				}
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

	free(processo);
} // tempo.c


