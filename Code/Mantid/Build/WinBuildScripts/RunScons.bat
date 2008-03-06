del ..\logs\scons.log
del ..\logs\sconsErr.log
del ..\logs\svn.log
del ..\logs\doxy.log
svn up
svn log -v -rBASE > ..\logs\svn.log
svn up ../Third_Party
svn up ../../Test
doxygen Build\Mantid.doxyfile 2> ..\logs\doxy.log
build.bat -Q "skiptest=1" 1> ..\logs\scons.log 2> ..\logs\sconsErr.log
