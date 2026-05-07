all: eleicaoDeLider eleicaoDeLiderAleatorizado

eleicaoDeLider: eleicaoDeLider.o smpl.o rand.o
	$(LINK.c) -o $@ -Bstatic eleicaoDeLider.o smpl.o rand.o -lm

eleicaoDeLiderAleatorizado: eleicaoDeLiderAleatorizado.o smpl.o rand.o
	$(LINK.c) -o $@ -Bstatic eleicaoDeLiderAleatorizado.o smpl.o rand.o -lm

eleicaoDeLider.o: eleicaoDeLider.c smpl.h
	$(COMPILE.c) -g eleicaoDeLider.c

eleicaoDeLiderAleatorizado.o: eleicaoDeLiderAleatorizado.c smpl.h
	$(COMPILE.c) -g eleicaoDeLiderAleatorizado.c

smpl.o: smpl.c smpl.h
	$(COMPILE.c) -g smpl.c

rand.o: rand.c
	$(COMPILE.c) -g rand.c

clean:
	$(RM) *.o 

purge: clean
	$(RM) eleicaoDeLider eleicaoDeLiderAleatorizado relat saida
