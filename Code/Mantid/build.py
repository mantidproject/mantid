import os
import subprocess
import sys
import platform

if platform.system() == 'Windows':
    msvc_version = "100"
    arch_string = "x86"
    if platform.architecture()[0] == '64bit':
        arch_string = 'amd64'
    setenv = 'CALL "%VS' + msvc_version + 'COMNTOOLS%..\\..\\VC\\vcvarsall.bat" ' + arch_string + ' &&' 
    use_system_scons = False
else:
    setenv = ''
    retcode = subprocess.call("which scons", shell=True)
    if retcode == 0:
        use_system_scons = True
    else:
        use_system_scons = False

if use_system_scons:
    print 'Using system installed scons'
    build_cmd = setenv + "scons "
else:
    print 'Using scons from third party directory'
    build_cmd = setenv + "python ../Third_Party/src/scons-local/scons.py "

# Add command line arguments
build_cmd += ' '.join(sys.argv[1:])
subprocess.call(build_cmd, shell=True)
