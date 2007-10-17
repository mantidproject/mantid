rm ../logs/scons.log
rm ../logs/svn.log
svn log -v -rBASE > ../logs/svn.log
cd ..
cd svn up Third_Party/
cd checkout/Build/
scons >> ../../logs/scons.log

