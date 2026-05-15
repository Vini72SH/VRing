#include <stdlib.h>
#include <string.h>

typedef struct nodo_t nodo_t;

struct nodo_t {
    void* elemento;
    nodo_t* prox;
};

typedef struct {
    int tamanho;
    nodo_t* it;
    nodo_t* inicio;
    nodo_t* fim;
    int tamanhoElemento;
} fila_t;

// Cria uma nova fila
fila_t* cria_fila(int (*comparador)(void*, void*), int tamanhoElemento);

// Destroi uma fila, liberando a memória alocada por ela.
int destroi_fila(fila_t* fila);

// Insere um item na fila
int insere_fila(fila_t* fila, void* item);

// Remove uma item da fila
int fila_del(fila_t* fila, void* item);

// Retorna o tamanho da fila
int tamanho_fila(fila_t* fila);

// Atualiza um elemento da fila, substituindo o antigo pelo novo
int atualiza_elemento(fila_t* fila, void* antigo, void* novo);

// Põe o iterador no início da fila e devolve o item apontado por ele
// Retorno: item se ele existe na posição do iterador
//          NULL se a fila estiver vazia ou não existir
void* fila_head(fila_t* fila);

// Avança o iterador ao próximo item na fila e devolve o item apontado por ele
// Retorno: item se ele existe na posição do iterador
//          NULL se o iterador passou do último item da fila
//          NULL se a fila estiver vazia ou não existir
void* fila_prox(fila_t* fila);
