# 2021-07-12

CFLAGS= -D_FILE_OFFSET_BITS=64 -ggdb

LDFLAGS = -fPIC
LDLIBS = -lfuse -ldl -lpthread

all: build_modules lionfs

build_modules:
	cd modules && $(MAKE) all

lionfs: lionfs.o network.o

lionfs.o: lionfs.c lionfs.h
network.o: network.c network.h
