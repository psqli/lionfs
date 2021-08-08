CFLAGS = -rdynamic -fPIC

LDFLAGS = -shared

all: curl.so

curl.so: curl
	ln -f -s curl curl.so
curl: curl.o
curl.o: curl.c
