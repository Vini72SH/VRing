CC = gcc
CFLAGS = -g -Ilib
DEBUG_FLAGS = -DDEBUG

.PHONY: all debug clean purge lib

BIN = task0 task1 task2 task3 task4 eleicaoDeLider eleicaoDeLiderAleatorizado

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

task%: lib tasks/task%.c
	@echo "=== Compilando $@ ==="
	$(CC) $(CFLAGS) -c tasks/$@.c -o tasks/$@.o

	@echo "=== Ligando $@ ==="
	$(CC) $(CFLAGS) lib/*.o tasks/$@.o -o $@ -lm

clean:
	-rm -f *.o *~ */*.o */*~

purge: clean
	$(RM) $(BIN)
