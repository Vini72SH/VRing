CC = gcc
CFLAGS = -g
DEBUG_FLAGS = -DDEBUG

all: eleicaoDeLider eleicaoDeLiderAleatorizado

debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

eleicaoDeLider: eleicaoDeLider.o smpl.o rand.o
	$(LINK.c) -o $@ -Bstatic eleicaoDeLider.o smpl.o rand.o -lm

eleicaoDeLiderAleatorizado: eleicaoDeLiderAleatorizado.o smpl.o rand.o
	$(LINK.c) -o $@ -Bstatic eleicaoDeLiderAleatorizado.o smpl.o rand.o -lm

eleicaoDeLider.o: eleicaoDeLider.c smpl.h
	$(COMPILE.c) $(CFLAGS) -c eleicaoDeLider.c

eleicaoDeLiderAleatorizado.o: eleicaoDeLiderAleatorizado.c smpl.h
	$(COMPILE.c) $(CFLAGS) -c eleicaoDeLiderAleatorizado.c

smpl.o: smpl.c smpl.h
	$(COMPILE.c) $(CFLAGS) -c smpl.c

rand.o: rand.c
	$(COMPILE.c) $(CFLAGS) -c rand.c

clean:
	$(RM) *.o

purge: clean
	$(RM) eleicaoDeLider eleicaoDeLiderAleatorizado relat saida
