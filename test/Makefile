all:
	gcc -Wall -O2 -fno-strict-aliasing -std=gnu11 $(CFLAGS) -o test.out test.c lsm9ds1.c ncv7708.c tca9458a.c tsl2561.c ads1115.c -I./ -lm -lpthread
	sudo ./test.out
