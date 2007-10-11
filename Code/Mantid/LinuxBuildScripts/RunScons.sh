rm ../logs/scons.log
rm ../logs/svn.log
svn log -v -rBASE > ../logs/svn.log
cd Build/
scons >> ../../logs/scons.log

