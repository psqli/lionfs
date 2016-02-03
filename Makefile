lionfs : lionfs.c lionfs.h network.c network.h array.c array.h
	cc -D_FILE_OFFSET_BITS=64 -lfuse -ldl -o lionfs lionfs.c network.c \
		array.c
