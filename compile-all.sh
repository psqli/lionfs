cwd="`pwd`"
xdir="`dirname $0`"

cd $xdir

./lib/libghttp/compile.sh
./modules/compile.sh
./compile.sh

cd $cwd
