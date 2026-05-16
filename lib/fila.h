#define TAMANHO_INICIAL 8
#define FATOR_MULT 2

typedef struct {
    int it;
    int ocupacao;
    int tamanho;
    int tamanhoElemento;
    void* elementos;
} fila_t;

// Cria uma nova fila
fila_t* cria_fila(int (*comparador)(void*, void*),
                  void (*atribuidor)(void*, void*), int tamanhoElemento);

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

// Põe o iterador no início da fila e devolve o item apontado por ele no
// ponteiro
// Retorno: 1 se ele existe na posição do iterador
//          0 se a fila estiver vazia ou não existir
int fila_head(fila_t* fila, void* item);

// Avança o iterador ao próximo item na fila e devolve o item apontado por ele
// no ponteiro
// Retorno: 1 se ele existe na posição do iterador
//          0 se o iterador passou do último item da fila
//          0 se a fila estiver vazia ou não existir
int fila_prox(fila_t* fila, void* item);
