rm ../logs/scons.log
rm ../logs/sconsErr.log
rm ../logs/svn.log
rm ../logs/doxy.log
svn log -v -rBASE > ../logs/svn.log
cd ..
svn up Third_Party/
cd checkout/Build/
scons >> ../../logs/scons.log 2> ../../logs/sconsErr.log
doxygen Mantid.doxyfile 2> ../../logs/doxy.log

