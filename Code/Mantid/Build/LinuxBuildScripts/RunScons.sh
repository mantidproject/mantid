#!/bin/sh
rm ../logs/scons.log
rm ../logs/sconsErr.log
rm ../logs/svn.log
rm ../logs/doxy.log
svn log -v -rBASE > ../logs/svn.log
svn up ../Third_Party
svn up ../../Test
cd Build
sh build.sh "skiptest=1" >> ../../logs/scons.log 2> ../../logs/sconsErr.log
doxygen Mantid.doxyfile 2> ../../logs/doxy.log
