all:
	gcc -o ../bin/tsock_v0 tsock.c

source:
	gcc -o ../bin/source tsock.c

puits:
	gcc -o ../bin/puits tsock.c