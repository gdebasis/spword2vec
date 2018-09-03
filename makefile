#set to the compiler of your system... use gcc-7 for MAC
CC = gcc-7  
#Using -Ofast instead of -O3 might result in faster code, but is supported only by newer GCC versions
CFLAGS = -lm -pthread -O3 -march=native -Wall -funroll-loops -Wno-unused-result

all: word2svec distwordpairs

word2svec : word2svec.c
	$(CC) word2svec.c -o word2svec $(CFLAGS)
distwordpairs : distwordpairs.c
	$(CC) distwordpairs.c -o distwordpairs $(CFLAGS)
	chmod +x *.sh

clean:
	rm -rf word2svec distwordpairs
