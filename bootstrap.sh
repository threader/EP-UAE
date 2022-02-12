#!/bin/sh

<<<<<<< HEAD
aclocal -I m4 \
&& automake --foreign --add-missing \
&& autoconf

cd src/tools
aclocal
autoconf
=======
echo .
echo .
echo Please Wait..
echo .
aclocal -I m4 \
&& automake --foreign --add-missing \
&& autoconf
echo ..almost over..
echo .
cd src/tools
aclocal
autoconf
echo Done. Thank you.
echo .
>>>>>>> p-uae/v2.1.0
