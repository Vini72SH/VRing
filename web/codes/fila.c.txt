#include "fila.h"

#include <stdlib.h>
#include <string.h>

int fila_cheia(fila_t* fila) { return fila->ocupacao == fila->capacidade; }
int fila_vazia(fila_t* fila) { return fila->ocupacao == 0; }

fila_t* cria_fila(int (*comparador)(void*, void*), int tamanhoElemento) {
    fila_t* fila = malloc(sizeof(fila_t));
    if (fila == NULL) return NULL;

    fila->elementos = malloc(tamanhoElemento * TAMANHO_INICIAL);
    if (fila->elementos == NULL) {
        free(fila);
        return NULL;
    }

    fila->it = 0;
    fila->ocupacao = 0;
    fila->capacidade = TAMANHO_INICIAL;
    fila->tamanhoElemento = tamanhoElemento;

    fila->comp = comparador;

    return fila;
}

int destroi_fila(fila_t** fila) {
    if (fila == NULL || *fila == NULL) return 0;

    free((*fila)->elementos);
    free(*fila);
    *fila = NULL;

    return 1;
}

int insere_fila(fila_t* fila, void* item) {
    if (fila == NULL) return 0;

    if (fila_cheia(fila)) {
        int novaCapacidade = fila->capacidade * FATOR_MULT;
        void* buffer = malloc(novaCapacidade * fila->tamanhoElemento);
        if (buffer == NULL) return 0;

        memcpy(buffer, fila->elementos, fila->ocupacao * fila->tamanhoElemento);

        free(fila->elementos);
        fila->elementos = buffer;
        fila->capacidade = novaCapacidade;
    }

    char* endereco =
        (char*)fila->elementos + (fila->ocupacao * fila->tamanhoElemento);

    memcpy(endereco, item, fila->tamanhoElemento);
    fila->ocupacao++;

    return 1;
}

int fila_del(fila_t* fila, void* item) {
    if (fila == NULL || fila_vazia(fila)) return 0;

    int idx;
    char* endereco;

    for (idx = 0; idx < fila->ocupacao; idx++) {
        endereco = (char*)fila->elementos + (idx * fila->tamanhoElemento);
        if (fila->comp(endereco, item)) break;
    }

    if (idx >= fila->ocupacao) return 0;
    if (idx == fila->it) fila->it = (fila->it > 0) ? (fila->it - 1) : 0;

    int offset = fila->ocupacao - idx - 1;
    if (offset > 0) {
        memmove((char*)fila->elementos + (idx * fila->tamanhoElemento),
                (char*)fila->elementos + ((idx + 1) * fila->tamanhoElemento),
                offset * fila->tamanhoElemento);
    }

    fila->ocupacao--;

    return 1;
}

int tamanho_fila(fila_t* fila) {
    if (fila == NULL) return 0;
    return fila->ocupacao;
}

int atualiza_elemento(fila_t* fila, void* antigo, void* novo) {
    if (fila == NULL) return 0;

    int i;
    char* elemento;

    for (i = 0; i < fila->ocupacao; i++) {
        elemento = (char*)fila->elementos + (i * fila->tamanhoElemento);
        if (fila->comp(elemento, antigo)) {
            memcpy(elemento, novo, sizeof(fila->tamanhoElemento));
            return 1;
        }
    }

    return 0;
}

int fila_head(fila_t* fila, void* item) {
    if (fila == NULL || fila_vazia(fila)) return 0;

    fila->it = 0;
    memcpy(item, (char*)fila->elementos + (fila->it * fila->tamanhoElemento),
           fila->tamanhoElemento);

    return 1;
}

int fila_prox(fila_t* fila, void* item) {
    if (fila == NULL) return 0;

    fila->it++;
    if (fila->it >= fila->ocupacao) return 0;

    memcpy(item, (char*)fila->elementos + (fila->it * fila->tamanhoElemento),
           fila->tamanhoElemento);

    return 1;
}