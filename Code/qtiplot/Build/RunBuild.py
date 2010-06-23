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
    setenv = 'CALL "%VCINSTALLDIR%\\vcvarsall.bat"'
    qmake_conf = ''
    if platform.architecture()[0] == '64bit':
        setenv += ' amd64'
        qmake_conf = 'CONFIG+=build64'
    setenv += ' && '
    make = setenv + make
    qmake = setenv + 'qmake' + ' ' + qmake_conf
else:
    pass

# Update the local copy
retcode = sp.call("svn up --accept theirs-full --non-interactive --trust-server-cert", shell=True)
if retcode != 0:
    sys.exit(1)

# Update the header containing the revision number
sp.call("python release_date.py",shell=True)

# If the dependent Mantid framework headers have changed, clean first
#frameworkLog = sp.Popen("svn log -qv -rCOMMITTED",stdout=sp.PIPE,shell=True,cwd="../Mantid").communicate()[0]
#mantidQtLog = sp.Popen("svn log -qv -rCOMMITTED",stdout=sp.PIPE,shell=True).communicate()[0]
#if ("MantidKernel" in frameworkLog) or ("MantidGeometry" in frameworkLog) or ("MantidAPI" in frameworkLog) or ("MantidQt" in mantidQtLog):
#    sp.call(make+" clean",shell=True,cwd="MantidQt")
#    sp.call(make+" clean",shell=True,cwd="qtiplot")


buildlog = open("../../../../logs/qtiplot/build.log","w") 
errorlog = open("../../../../logs/qtiplot/error.log","w")

buildStart = time.time()
# Build MantidQt
# Updates to UI files alone don't always seem to get picked up without a clean first, so do that until we can figure out why
sp.call(qmake,stdout=buildlog,stderr=errorlog,shell=True,cwd="MantidQt")
sp.call(make + ' clean',stdout=buildlog,stderr=errorlog,shell=True,cwd="MantidQt")
ret = sp.call(make,stdout=buildlog,stderr=errorlog,shell=True,cwd="MantidQt")
if ret != 0:
    outcome = "MantidQt build failed"
    buildlog.write(outcome)
    sys.exit(0)

# Build QtPropertyBrowser
sp.call(qmake,stdout=buildlog,stderr=errorlog,shell=True,cwd="QtPropertyBrowser")
ret = sp.call(make,stdout=buildlog,stderr=errorlog,shell=True,cwd="QtPropertyBrowser")
if ret != 0:
    outcome = "QtPropertyBrowser build failed"
    buildlog.write(outcome)
    sys.exit(0)

# Now build MantidPlot
sp.call(qmake,stdout=buildlog,stderr=errorlog,shell=True,cwd="qtiplot")
ret = sp.call(make + ' ' + ''.join(buildargs),stdout=buildlog,stderr=errorlog,shell=True,cwd="qtiplot")
if ret != 0:
    outcome = "MantidPlot build failed"
    buildlog.write(outcome)

buildFinish = time.time()
buildTime = buildFinish - buildStart

BuildTimeLog = open("../../../../logs/qtiplot/timebuild.log", "w")
BuildTimeLog.write(str(buildTime))
BuildTimeLog.close()

buildlog.close()
errorlog.close()
