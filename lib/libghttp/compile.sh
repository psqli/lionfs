cwd="`pwd`"
xdir="`dirname $0`"

cd $xdir

gcc -fPIC -c *.c
ar rcs libghttp *.o
rm *.o

cd $cwd
