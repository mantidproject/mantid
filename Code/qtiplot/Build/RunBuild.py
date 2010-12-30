import os
import platform
import subprocess as sp
import sys
import platform
import time

# Set the make command for this system
buildargs = []
if os.name == 'posix':
    make = "make"
else:
    make = "nmake"

if platform.system() == 'Linux':
    qmake = "qmake-qt4 QMAKE_CXX=g++44 QMAKE_CC=gcc44 QMAKE_LINK=g++44"
    buildargs.append("-j2")
elif platform.system() == 'Darwin':
    # Have to set QMAKESPEC for mac to avoid xcode build
    os.putenv('QMAKESPEC','/usr/local/Qt4.5/mkspecs/macx-g++')
    qmake = "qmake CONFIG+=release"
    buildargs.append("-j2")
elif platform.system() == 'Windows':
    msvc_version = "100"
    arch_string = "x86"
    qmake_conf = 'QMAKESPEC=win32-msvc2008'
    if platform.architecture()[0] == '64bit':
        arch_string = 'amd64'
        qmake_conf = ' CONFIG+=build64'
    setenv = 'CALL "%VS' + msvc_version + 'COMNTOOLS%..\\..\\VC\\vcvarsall.bat" ' + arch_string + ' &&' 
    make = setenv + make
    qmake = setenv + 'qmake' + ' ' + qmake_conf
else:
    pass

# Update the local copy
retcode = sp.call("svn up --accept theirs-full --non-interactive --trust-server-cert", shell=True, cwd="../Mantid/QtPropertyBrowser")
if retcode != 0:
    sys.exit(1)
retcode = sp.call("svn up --accept theirs-full --non-interactive --trust-server-cert", shell=True, cwd="../Mantid/MantidQt")
if retcode != 0:
    sys.exit(1)
retcode = sp.call("svn up --accept theirs-full --non-interactive --trust-server-cert", shell=True, cwd="../Mantid/MantidPlot")
if retcode != 0:
    sys.exit(1)

log_dir = "../../../../logs/"
svnlog = open(log_dir + "qtiplot/svn.log","w")
sp.call("svn log -v -rBASE",stdout=svnlog,shell=True)
svnlog.close()

# Update the header containing the revision number
sp.call("python release_date.py",shell=True,cwd="../Mantid/MantidPlot")

buildlog = open(log_dir + "qtiplot/build.log","w") 
errorlog = open(log_dir + "qtiplot/error.log","w")

buildStart = time.time()
#Build QtPropertyBrowser
sp.call(qmake,stdout=buildlog,stderr=errorlog,shell=True,cwd="../Mantid/QtPropertyBrowser")
ret = sp.call(make,stdout=buildlog,stderr=errorlog,shell=True,cwd="../Mantid/QtPropertyBrowser")
if ret != 0:
    outcome = "QtPropertyBrowser build failed"
    buildlog.write(outcome)
    sys.exit(0)

# Build MantidQt
# Updates to UI files alone don't always seem to get picked up without a clean first, so do that until we can figure out why
sp.call(qmake,stdout=buildlog,stderr=errorlog,shell=True,cwd="../Mantid/MantidQt")
sp.call(make + ' clean',stdout=buildlog,stderr=errorlog,shell=True,cwd="../Mantid/MantidQt")
ret = sp.call(make,stdout=buildlog,stderr=errorlog,shell=True,cwd="../Mantid/MantidQt")

if ret != 0:
    outcome = "MantidQt build failed"
    buildlog.write(outcome)
    sys.exit(0)


#Now build MantidPlot
sp.call(qmake,stdout=buildlog,stderr=errorlog,shell=True,cwd="../Mantid/MantidPlot")
ret = sp.call(make + ' ' + ''.join(buildargs),stdout=buildlog,stderr=errorlog,shell=True,cwd="../Mantid/MantidPlot")
if ret != 0:
    outcome = "MantidPlot build failed"
    buildlog.write(outcome)

buildFinish = time.time()
buildTime = buildFinish - buildStart

BuildTimeLog = open(log_dir + "qtiplot/timebuild.log", "w")
BuildTimeLog.write(str(buildTime))
BuildTimeLog.close()

buildlog.close()
errorlog.close()
