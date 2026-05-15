#include "fila.h"

int (*comp)(void*, void*);

fila_t* cria_fila(int (*comparador)(void*, void*), int tamanhoElemento) {
    fila_t* fila;

    fila = malloc(sizeof(fila_t));
    if (fila == NULL) return NULL;

    fila->tamanho = 0;
    fila->it = NULL;
    fila->inicio = NULL;
    fila->fim = NULL;
    fila->tamanhoElemento = tamanhoElemento;

    comp = comparador;

    return fila;
};

int destroi_fila(fila_t* fila) {
    if (fila == NULL) return 0;

    nodo_t* aux = fila->inicio;
    nodo_t* prox = NULL;

    while (aux != NULL) {
        prox = aux->prox;
        free(aux->elemento);
        free(aux);
        aux = prox;
    }

    free(fila);
    fila = NULL;

    return 1;
}

int insere_fila(fila_t* fila, void* item) {
    if (fila == NULL) return 0;

    nodo_t* nodo = malloc(sizeof(nodo_t));
    if (nodo == NULL) return 0;

    nodo->elemento = malloc(fila->tamanhoElemento);
    if (nodo->elemento == NULL) return 0;

    memcpy(nodo->elemento, item, fila->tamanhoElemento);
    nodo->prox = NULL;

    if (fila->inicio == NULL) {
        fila->inicio = nodo;
        fila->fim = nodo;
        fila->it = nodo;
    } else {
        fila->fim->prox = nodo;
        fila->fim = nodo;
    }

    fila->tamanho++;

    return 1;
}

int fila_del(fila_t* fila, void* item) {
    if (fila == NULL) return 0;

    nodo_t* ant = NULL;
    nodo_t* aux = fila->inicio;

    while ((aux != NULL) && (comp(aux->elemento, item) == 0)) {
        ant = aux;
        aux = aux->prox;
    }

    if (aux == NULL) return 0;

    if (fila->it == aux) fila->it = fila->it->prox;

    if (ant == NULL)
        fila->inicio = aux->prox;
    else
        ant->prox = aux->prox;

    if (aux->prox == NULL) fila->fim = ant;

    fila->tamanho--;
    free(aux->elemento);
    free(aux);

    return 1;
}

int tamanho_fila(fila_t* fila) {
    if (fila == NULL) return 0;

    return fila->tamanho;
}

// Atualiza um elemento da fila, substituindo o antigo pelo novo
int atualiza_elemento(fila_t* fila, void* antigo, void* novo) {
    nodo_t* ant = NULL;
    nodo_t* aux = fila->inicio;

    while ((aux != NULL) && (comp(aux->elemento, antigo) == 0)) {
        ant = aux;
        aux = aux->prox;
    }

    if (aux == NULL) return 0;

    memcpy(aux->elemento, novo, fila->tamanhoElemento);
}

void* fila_head(fila_t* fila) {
    if ((fila == NULL) || fila->inicio == NULL) return NULL;

    fila->it = fila->inicio;

    return fila->it->elemento;
}

void* fila_prox(fila_t* fila) {
    if ((fila == NULL) || (fila->it == NULL)) return NULL;

    fila->it = fila->it->prox;
    if (fila->it == NULL) return NULL;

    return fila->it->elemento;
}
