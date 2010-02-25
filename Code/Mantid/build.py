import os
import subprocess
import sys
import platform

execfile("release_version.py")

if platform.system() == 'Windows':
	setenv = 'CALL "%VCINSTALLDIR%\\vcvarsall.bat"'
	if platform.architecture()[0] == '64bit':
		setenv += ' amd64'
	setenv += ' && '
else:
	setenv = ''
retcode = subprocess.call(setenv + "scons "+' '.join(sys.argv[1:]), shell=True)
if retcode != 0:
    p = subprocess.call(setenv + "python ../Third_Party/src/scons-local/scons.py "+' '.join(sys.argv[1:]), shell=True)
