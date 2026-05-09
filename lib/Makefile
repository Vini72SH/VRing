CC = gcc
CFLAGS = -g
DEBUG_FLAGS = -DDEBUG

all: smpl.o rand.o

smpl.o: smpl.c smpl.h
	$(COMPILE.c) $(CFLAGS) -c smpl.c

rand.o: rand.c
	$(COMPILE.c) $(CFLAGS) -c rand.c