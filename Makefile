
all: build

debug:
	gcc -Wall -ggdb3 -o relogio relogio.c

build:
	gcc -Wall -o relogio relogio.c
