cwd="`pwd`"
xdir="`dirname $0`"

cd $xdir

cc -D_FILE_OFFSET_BITS=64 -lfuse -ldl -lpthread -o lionfs lionfs.c network.c array.c vmutex.c

cd $cwd
