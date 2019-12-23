CC=gcc
CFLAGS=-Wall -O2 -std=gnu99
LDFLAGS=
SRC=src/main.c src/main_helper.c
INCLUDE=include/

all:
	$(CC) $(CFLAGS) -o flight.out $(SRC) -I$(INCLUDE) $(LDFLAGS)