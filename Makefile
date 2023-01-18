
all: build

debug:
	gcc -Wall -ggdb3 relogio.c

build:
	gcc -Wall relogio.c
