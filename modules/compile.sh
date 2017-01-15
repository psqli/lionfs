cwd="`pwd`"
xdir="`dirname $0`"

cd $xdir

gcc -shared -fPIC -o http    http.c ../lib/libghttp/libghttp

cd $cwd
