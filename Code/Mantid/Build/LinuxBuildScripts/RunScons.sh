#!/bin/sh
rm ../../../../logs/scons.log
rm ../../../../logs/sconsErr.log
rm ../../../../logs/svn.log
rm ../../../../logs/doxy.log
svn log -v -rBASE > ../../../../logs/svn.log
svn update ../../Test
sh build.sh "skiptest=1" >> ../../../../logs/scons.log 2> ../../../../logs/sconsErr.log
cd Build
doxygen Mantid.doxyfile 2> ../../../../../logs/doxy.log
