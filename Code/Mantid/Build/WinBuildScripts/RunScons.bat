del ..\logs\scons.log
del ..\logs\sconsErr.log
del ..\logs\svn.log
del ..\logs\doxy.log
svn log -v -rBASE:HEAD > ..\logs\svn.log
svn up
svn up ../Third_Party
svn up ../../Test
doxygen Build\Mantid.doxyfile 2> ..\logs\doxy.log
build.bat 1> ..\logs\scons.log 2> ..\logs\sconsErr.log
