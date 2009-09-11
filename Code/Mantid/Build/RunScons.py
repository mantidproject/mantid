import os
import subprocess as sp
import platform
import buildNotification

buildNotification.sendBuildStarted("Mantid")

sp.call("svn up", shell=True)
svnlog = open("../../../../logs/Mantid/svn.log","w")
sp.call("svn log -v -rBASE",stdout=svnlog,shell=True)
svnlog.close()

sp.call("svn up ../Third_Party", shell=True)
sp.call("svn up ../../Test", shell=True)
sp.call("svn up ../../Images", shell=True)

print "Building documentation..."
doxylog = open("../../../../logs/Mantid/doxy.log","w")
sp.call("doxygen Mantid.doxyfile",stderr=doxylog,shell=True,cwd="Build")
doxylog.close()
print "Done building documentation"

print "Building Mantid framework..."
sconslog = open("../../../../logs/Mantid/scons.log","w")
sconserr = open("../../../../logs/Mantid/sconsErr.log","w")
buildargs = ["-j2","skiptest=1"]
if platform.system() != 'Darwin':
    buildargs.append("matlab=1")
sp.call("python build.py "+' '.join(buildargs),stdout=sconslog,stderr=sconserr,shell=True)
sconslog.close()
sconserr.close()
print "Mantid build complete."

buildNotification.sendBuildCompleted("Mantid")
