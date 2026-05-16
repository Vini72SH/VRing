#include "fila.h"

#include <stdlib.h>
#include <string.h>

int (*comp)(void*, void*);
void (*atrib)(void*, void*);

int fila_cheia(fila_t* fila) { return fila->ocupacao == fila->tamanho; }
int fila_vazia(fila_t* fila) { return fila->ocupacao == 0; }

fila_t* cria_fila(int (*comparador)(void*, void*),
                  void (*atribuidor)(void*, void*), int tamanhoElemento) {
    fila_t* fila;

    fila = malloc(sizeof(fila_t));
    if (fila == NULL) return NULL;

    fila->it = 0;
    fila->ocupacao = 0;
    fila->tamanho = TAMANHO_INICIAL;
    fila->tamanhoElemento = tamanhoElemento;

    comp = comparador;
    atrib = atribuidor;

    fila->elementos = malloc(tamanhoElemento * TAMANHO_INICIAL);

    return fila;
};

int destroi_fila(fila_t* fila) {
    if (fila == NULL) return 0;

    free(fila->elementos);
    free(fila);
    fila = NULL;

    return 1;
}

int insere_fila(fila_t* fila, void* item) {
    if (fila == NULL) return 0;

    // Quando a fila está cheia, aumenta o tamanho do buffer e copia os
    // elementos já inseridos
    if (fila_cheia(fila)) {
        char* it;
        int idx;

        fila->tamanho = fila->tamanho * FATOR_MULT;
        void* buffer = malloc(fila->tamanho * fila->tamanhoElemento);

        for (int i = 0; i < fila->ocupacao; i++) {
            memcpy(buffer + (i * fila->tamanhoElemento),
                   fila->elementos + (i * fila->tamanhoElemento),
                   fila->tamanhoElemento);
        }

        free(fila->elementos);
        fila->elementos = buffer;
    }

    char* endereco =
        (char*)((fila->elementos) + (fila->ocupacao * fila->tamanhoElemento));

    memcpy(endereco, item, fila->tamanhoElemento);
    fila->ocupacao++;

    return 1;
}

int fila_del(fila_t* fila, void* item) {
    if ((fila == NULL) || fila_vazia(fila)) return 0;

    int idx;
    char* endereco = fila->elementos;
    for (idx = 0; idx < fila->ocupacao; idx++) {
        endereco = endereco + (idx * fila->tamanhoElemento);
        if (comp(endereco, item)) break;
    }

    if (idx >= fila->ocupacao) return 0;

    char* substituto =
        fila->elementos + ((fila->ocupacao - 1) * fila->tamanhoElemento);

    memcpy(endereco, substituto, fila->tamanhoElemento);
    fila->ocupacao--;

    return 1;
}

int tamanho_fila(fila_t* fila) {
    if (fila == NULL) return 0;

    return fila->tamanho;
}

int atualiza_elemento(fila_t* fila, void* antigo, void* novo) {
    char* elemento;

    int i = 0;
    elemento = fila->elementos;
    while ((i < fila->ocupacao) && (comp(elemento, antigo) == 0)) {
        i++;
        elemento = fila->elementos + (i * fila->tamanhoElemento);
    }

    if (i >= fila->ocupacao) return 0;

    atrib(elemento, novo);

    return 1;
}

int fila_head(fila_t* fila, void* item) {
    if ((fila == NULL) || (fila_vazia(fila))) return 0;

    fila->it = 0;
    memcpy(item, fila->elementos + (fila->it * fila->tamanhoElemento),
           fila->tamanhoElemento);

    return 1;
}

int fila_prox(fila_t* fila, void* item) {
    if (fila == NULL) return 0;

    fila->it++;
    if (fila->it >= fila->ocupacao) return 0;

    memcpy(item, fila->elementos + (fila->it * fila->tamanho),
           fila->tamanhoElemento);

    return 1;
}
