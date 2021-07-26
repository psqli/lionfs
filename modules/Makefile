CFLAGS = -rdynamic -fPIC

LDFLAGS = -shared

all: curl.so

curl.so: curl
	ln -s curl curl.so
curl: curl.o
curl.o: curl.c
