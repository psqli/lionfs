cwd="`pwd`"
xdir="`dirname $0`"

cd $xdir

gcc -D_FILE_OFFSET_BITS=64 -lfuse -ldl -lpthread -o lionfs lionfs.c network.c

cd $cwd
