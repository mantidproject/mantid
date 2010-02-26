import os
import platform

print 'Current path == ',os.getcwd()
build_cmd = 'build.bat'
if platform.architecture()[0] == "32bit":
    build_cmd += " x86"
elif platform.architecture()[0] == "64bit":
    build_cmd += " amd64"
else:
    pass

build_cmd += ' 1> ../../../../logs/Installer/build.log 2> ../../../../logs/Installer/error.log'
retcode = os.system(build_cmd)
if retcode == 0:
    exit(0)
else:
    print "Error building MSI"
    exit(1)

