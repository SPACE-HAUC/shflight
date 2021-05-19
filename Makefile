CC=gcc
RM= /bin/rm -vf
PYTHON=python3
ARCH=UNDEFINED
PWD=pwd
CDR=$(shell pwd)

EDCFLAGS:=$(CFLAGS) -DACS_DATAVIS -DACS_PRINT -DSITL -DACS_DATALOG -DDATAVIS
EDLDFLAGS:=$(LDFLAGS)

ifeq ($(ARCH),UNDEFINED)
	ARCH=$(shell uname -m)
endif

EDCFLAGS:= -Wall -fno-strict-aliasing -std=gnu11 -O2 $(EDCFLAGS)
EDLDFLAGS:= -lm -lpthread $(EDLDFLAGS)

TARGETOBJS=drivers/ncv7708.o drivers/tsl2561.o drivers/tca9458a.o drivers/ads1115.o drivers/lsm9ds1.o \
			src/main.o src/sitl_comm.o src/datavis.o src/acs.o src/bessel.o

TARGET=shflight.out

all: build/$(TARGET)

build:
	mkdir build

build/$(TARGET): $(TARGETOBJS) build
	$(CC) $(TARGETOBJS) $(LINKOPTIONS) -o $@ \
	$(EDLDFLAGS)

%.o: %.c
	$(CC) $(EDCFLAGS) -Iinclude/ -Idrivers/ -o $@ -c $<

# clean: cleanobjs
clean:
	$(RM) build/$(TARGET)
	$(RM) $(TARGETOBJS)

spotless: clean
	$(RM) -R build
	$(RM) -R docs

run: build/$(TARGET)
	sudo build/$(TARGET)

sim_server: build
	echo "This program is supposed to run on a Raspberry Pi only to receive data from the H/SITL simulator"
	$(CC) src/sim_server.c -O2 -o build/sim_server.out -lm -lpthread
	sudo build/sim_server.out

doc:
	doxygen .doxyconfig

pdf: doc
	cd docs/latex && make && mv refman.pdf ../../
# cleanobjs:
# 	find $(CDR) -name "*.o" -type f
