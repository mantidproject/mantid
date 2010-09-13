import os
import subprocess
import sys
import platform

if platform.system() == 'Windows':
    msvc_version = "80"
    arch_string = "x86"
    if platform.architecture()[0] == '64bit':
        msvc_version = "100"
        arch_string = 'amd64'
    setenv = 'CALL "%VS' + msvc_version + 'COMNTOOLS%..\\..\\VC\\vcvarsall.bat" ' + arch_string + ' &&' 
else:
    setenv = ''
retcode = subprocess.call(setenv + "scons "+' '.join(sys.argv[1:]), shell=True)
if retcode != 0:
    p = subprocess.call(setenv + "python ../Third_Party/src/scons-local/scons.py "+' '.join(sys.argv[1:]), shell=True)
