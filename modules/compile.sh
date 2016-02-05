cwd="`pwd`"
xdir="`dirname $0`"

cd $xdir

cc -shared -o http http.c ../lib/libghttp/libghttp

cd $cwd
