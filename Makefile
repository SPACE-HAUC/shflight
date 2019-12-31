CC=gcc
CFLAGS=-Wall -O2 -std=gnu11
LDFLAGS=
SRC=src/main.c src/main_helper.c src/data_ack.c src/acs.c
driver_objects=drivers/ncv7708/ncv7708.o
INCLUDE=include/
DRIVERS=drivers/

all:
	$(CC) $(CFLAGS) -o flight.out $(SRC) $(driver_objects) -I$(INCLUDE) -I$(DRIVERS) $(LDFLAGS)