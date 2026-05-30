CC = gcc
CFLAGS = -g -Ilib
DEBUG_FLAGS = -DDEBUG

.PHONY: all debug clean purge lib

BIN = vinicius_tarefa0 vinicius_tarefa1 vinicius_tarefa2 vinicius_tarefa3 vinicius_tarefa4 \
	  agathe_tarefa0 agathe_tarefa1 agathe_tarefa2 agathe_tarefa3 agathe_tarefa4 \
	  eleicaoDeLider eleicaoDeLiderAleatorizado

all: $(BIN)

debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

# compila bibliotecas
lib:
	@echo "=== Compilando $@ ==="
	-test -s $@/Makefile && $(MAKE) -C $@

eleicaoDeLider: lib src/eleicaoDeLider.c
	@echo "=== Compilando $@ ==="
	$(CC) $(CFLAGS) -c src/eleicaoDeLider.c -o src/eleicaoDeLider.o

	@echo "=== Ligando $@ ==="
	$(CC) $(CFLAGS) lib/*.o src/eleicaoDeLider.o -o $@ -lm

eleicaoDeLiderAleatorizado: lib src/eleicaoDeLiderAleatorizado.c
	@echo "=== Compilando $@ ==="
	$(CC) $(CFLAGS) -c src/eleicaoDeLiderAleatorizado.c -o src/eleicaoDeLiderAleatorizado.o

	@echo "=== Ligando $@ ==="
	$(CC) $(CFLAGS) lib/*.o src/eleicaoDeLiderAleatorizado.o -o $@ -lm

vinicius_tarefa%: lib tasks_vinicius/vinicius_tarefa%.c
	@echo "=== Compilando $@ ==="
	$(CC) $(CFLAGS) -c tasks_vinicius/$@.c -o tasks_vinicius/$@.o

	@echo "=== Ligando $@ ==="
	$(CC) $(CFLAGS) lib/*.o tasks_vinicius/$@.o -o $@ -lm

agathe_tarefa%: lib tasks_agathe/agathe_tarefa%.c
	@echo "=== Compilando $@ ==="
	$(CC) $(CFLAGS) -c tasks_agathe/$@.c -o tasks_agathe/$@.o

	@echo "=== Ligando $@ ==="
	$(CC) $(CFLAGS) lib/*.o tasks_agathe/$@.o -o $@ -lm

clean:
	-rm -f *.o *~ */*.o */*~

purge: clean
	$(RM) $(BIN)
