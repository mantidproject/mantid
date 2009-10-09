import os
import subprocess as sp
import sys

# Set the make command for this system
if os.name == 'posix':
    make = "make"
else:
    make = "nmake"

# Update the local copy
sp.call("svn up", shell=True)

# Update the header containing the revision number
sp.call("python release_date.py",shell=True)

# If the dependent Mantid framework headers have changed, clean first
frameworkLog = sp.Popen("svn log -qv -rCOMMITTED",stdout=sp.PIPE,shell=True,cwd="../Mantid").communicate()[0]
mantidQtLog = sp.Popen("svn log -qv -rCOMMITTED",stdout=sp.PIPE,shell=True).communicate()[0]
if ("MantidKernel" in frameworkLog) or ("MantidGeometry" in frameworkLog) or ("MantidAPI" in frameworkLog) or ("MantidQt" in mantidQtLog):
    sp.call(make+" clean",shell=True,cwd="MantidQt")
    sp.call(make+" clean",shell=True,cwd="qtiplot")


buildlog = open("../../../../logs/qtiplot/build.log","w") 
errorlog = open("../../../../logs/qtiplot/error.log","w")

# Build MantidQt
ret = sp.call(make,stdout=buildlog,stderr=errorlog,shell=True,cwd="MantidQt")
if ret != 0:
    outcome = "MantidQt build failed"
    buildlog.write(outcome)
    sys.exit(0)

# Now build MantidPlot
sp.call("qmake",stdout=buildlog,stderr=errorlog,shell=True,cwd="qtiplot")
ret = sp.call(make,stdout=buildlog,stderr=errorlog,shell=True,cwd="qtiplot")
if ret != 0:
    outcome = "MantidPlot build failed"
    buildlog.write(outcome)

buildlog.close()
errorlog.close()
