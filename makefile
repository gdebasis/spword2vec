#set to the compiler of your system... use gcc-7 for MAC
CC = gcc-7  
#Using -Ofast instead of -O3 might result in faster code, but is supported only by newer GCC versions
CFLAGS = -lm -pthread -O3 -march=native -Wall -funroll-loops -Wno-unused-result

all: node2vec word2vec word2svec distwordpairs

word2vec : word2vec.c
	$(CC) word2vec.c -o wordsvec $(CFLAGS)
node2vec : node2vec.c
	$(CC) node2vec.c -o node2vec $(CFLAGS)
word2svec : word2svec.c
	$(CC) word2svec.c -o word2svec $(CFLAGS)
distwordpairs : distwordpairs.c
	$(CC) distwordpairs.c -o distwordpairs $(CFLAGS)
	chmod +x *.sh

clean:
	rm -rf node2vec word2svec word2vec distwordpairs
