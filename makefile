CC = clang
CFLAGS = -Wall -O3 -g
ObjFile = main.o
Target = mishell

all: $(Target)

$(Target) : $(ObjFile)
	$(CC) -o $(Target) main.o $(CFLAGS)

main.o : main.c
	$(CC) -o main.o -c main.c $(CFLAGS)
