CC = gcc
CFLAGS = -Wall -Wextra -g

all: simulador

simulador: simulador.o PageTable.o Memory.o utils.o
	$(CC) $(CFLAGS) $^ -o $@ -lm

simulador.o: simulador.c PageTable.h Memory.h utils.h
	$(CC) $(CFLAGS) -c $< -o $@

PageTable.o: PageTable.c PageTable.h
	$(CC) $(CFLAGS) -c $< -o $@ -g

Memory.o: Memory.c Memory.h
	$(CC) $(CFLAGS) -c $< -o $@

utils.o: utils.c utils.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f *.o simulador
