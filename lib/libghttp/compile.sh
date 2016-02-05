cwd="`pwd`"
xdir="`dirname $0`"

cd $xdir

cc -c *.c
ar rcs libghttp *.o
rm *.o

cd $cwd
