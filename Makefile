CC=gcc
RM= /bin/rm -vf
PYTHON=python3
ARCH=UNDEFINED
PWD=pwd
CDR=$(shell pwd)

ifeq ($(ARCH),UNDEFINED)
	ARCH=$(shell uname -m)
endif

EDCFLAGS:= -Wall -fno-strict-aliasing -std=gnu11 -O3 -mfp16-format=ieee \
			-I include/ -I drivers/ \
			-DGPIODEV_PINOUT=0 \
			-Wno-unused-value \
			-Wno-unused-function \
			-Wno-unused-variable \
			-Wno-format \
			-Wno-unused-result \
			-Wno-array-bounds \
			$(CFLAGS) \
			-DDLGR_EPS
EDLDFLAGS:= -liio -lm -lpthread $(LDFLAGS)

EDCFLAGS+= -DDATAVIS -DACS_PRINT -DACS_DATALOG

DRVOBJS = drivers/adar1000/adar1000.o \
			drivers/adf4355/adf4355.o \
			drivers/eps_p31u/p31u.o   \
			drivers/gpiodev/gpiodev.o \
			drivers/i2cbus/i2cbus.o   \
			drivers/lsm9ds1/lsm9ds1.o \
			drivers/ncv7718/ncv7718.o \
			drivers/spibus/spibus.o   \
			drivers/tsl2561/tsl2561.o \
			drivers/tca9458a/tca9458a.o \
			drivers/tmp123/tmp123.o \
			drivers/uhf_modem/uhf_modem.o

DRVLIBS = drivers/modem/libmodem.a \
		drivers/finesunsensor/liba60sensor.a

TARGETOBJS=src/main.o \
		src/datavis.o \
		src/acs.o \
		src/bessel.o \
		src/cmd_parser.o \
		src/eps.o \
		src/sw_update_sh.o \
		src/xband.o \
		src/datalogger.o


TARGET=shflight.out

all: build/$(TARGET)

$(DRVLIBS): drivers/modem/libmodem.a drivers/finesunsensor/liba60sensor.a

drivers/modem/libmodem.a: 
	cd drivers/modem && make modemlib && cd $(CDR)

drivers/finesunsensor/liba60sensor.a:
	cd drivers/finesunsensor && make liba60sensor.a && cd $(CDR)

build:
	mkdir build

build/$(TARGET): $(DRVLIBS) $(DRVOBJS) $(TARGETOBJS) build
	$(CC) $(TARGETOBJS) $(DRVLIBS) $(DRVOBJS) $(LINKOPTIONS) -o $@ \
	$(EDLDFLAGS)

%.o: %.c
	$(CC) $(EDCFLAGS) -Iinclude/ -Idrivers/ -o $@ -c $<

# clean: cleanobjs
clean:
	$(RM) build/$(TARGET)
	$(RM) $(TARGETOBJS)
	$(RM) $(DRVOBJS)
	$(RM) $(DRVLIBS)

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
