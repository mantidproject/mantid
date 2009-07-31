rem del ..\..\..\..\logs\Mantid\scons.log
rem del ..\..\..\..\logs\Mantid\sconsErr.log
rem del ..\..\..\..\logs\Mantid\svn.log
rem del ..\..\..\..\logs\Mantid\doxy.log
svn up
svn log -v -rBASE > ..\..\..\..\logs\Mantid\svn.log
svn up ../Third_Party
svn up ../../Test
svn up ../../Images
cd Build
doxygen Mantid.doxyfile 2> ..\..\..\..\..\logs\Mantid\doxy.log
cd ..
build.bat -j2 "win64=1" "skiptest=1" 1> ..\..\..\..\logs\Mantid\scons.log 2> ..\..\..\..\logs\Mantid\sconsErr.log
