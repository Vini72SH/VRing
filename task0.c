#include <stdio.h>
#include <stdlib.h>
#include "smpl.h"

#define TEST 1
#define FAULT 2
#define RECOVERY 3

typedef struct {
  int id;
}TipoProcesso;

TipoProcesso *processo;

int main (int argc, char *argv[]) {
  static int N, token, event, r, i, MaxTempoSimulac = 150;
  static char fa_name[5];

  if (argc != 2) {
    puts("Uso correto: %s tempo <número de processos>");
    exit(1);
  }

  N = atoi(argv[1]);

  smpl(0, "Um Exemplo de Simulação");
  reset();
  stream(1);

  processo = (TipoProcesso *) malloc(sizeof(TipoProcesso) * N);

  for (i = 0; i < N; i++){
    memset(fa_name, '\0', 5);
    sprintf(fa_name, "%d", i);
    processo[i].id = facility(fa_name, 1);
  }

  // Vamos fazer o escalonamento inicial de eventos

  // Nossos processos vão executar testes em intervalos de testes
  // O intervalo de testes vai ser de 30 unidades de tempo
  // A simulação começa no tempo 0 (zero) e vamos escalonar o primeiro teste de todos os
  // processos para o tempo 30.0

  for (i = 0; i < N; i++) {
    schedule(TEST, 30.0, i);
  }

  schedule(FAULT, 31.0, 1);
  schedule(RECOVERY, 61.0, 1);

  puts("===============================================================");
  puts("           Sistemas DistribuÃ­dos Prof. Elias");
  puts("          LOG do Trabalho Prático 0, Tarefa 0");
  puts("      Digitar, compilar e executar o programa tempo.c");
  printf("   Este programa foi executado para: N=%d processos.\n", N);
  printf("           Tempo Total de Simulação = %d\n", MaxTempoSimulac);
  puts("===============================================================");

  while(time() < 150.0) {
    cause(&event, &token);
    switch (event) {
    case TEST:
      if (status(processo[token].id) != 0) break; // Se o processo está falho, não testa!
      printf("Processo %d: estou testando no tempo %4.1f\n", token, time());
      schedule(TEST, 30.0, token);
      break;
    case FAULT:
      r = request(processo[token].id, token, 0);
      printf("Socorro!!! Sou o processo %d e estou falhando no tempo %4.1f\n", token, time());
      break;
    case RECOVERY:
      release(processo[token].id, token);
      printf("Viva!!! Sou o processo %d e acabo de recuperar no tempo %4.1f\n", token, time());
      schedule(TEST, 1.0, token);
      break;
    }

  }

}
