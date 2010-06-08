import sys
import os
import subprocess as sp
import platform
import buildNotification

buildNotification.sendBuildStarted("Mantid")

retcode = sp.call("svn up --accept theirs-full --non-interactive --trust-server-cert", shell=True)
if retcode != 0:
    sys.exit(1)
svnlog = open("../../../../logs/Mantid/svn.log","w")
sp.call("svn log -v -rBASE",stdout=svnlog,shell=True)
svnlog.close()

sp.call("svn up --accept theirs-full --non-interactive --trust-server-cert ../Third_Party", shell=True)
sp.call("svn up --accept theirs-full --non-interactive --trust-server-cert ../../Test", shell=True)
sp.call("svn up --accept theirs-full --non-interactive --trust-server-cert ../../Images", shell=True)

doxylog = open("../../../../logs/Mantid/doxy.log","w")
sp.call("doxygen Mantid.doxyfile",stderr=doxylog,shell=True,cwd="Build")
doxylog.close()

sconslog = open("../../../../logs/Mantid/scons.log","w")
sconserr = open("../../../../logs/Mantid/sconsErr.log","w")
buildargs = ["-j2","skiptest=1"]
if platform.system() == 'Windows':
	# Set the appropriate environment
	if platform.architecture()[0] == '64bit':
		buildargs.append("win64=1")
	else:
		buildargs.append("matlab=1")
elif platform.system() == 'Linux':
    buildargs.append("gcc44=1")
sp.call("python build.py "+' '.join(buildargs),stdout=sconslog,stderr=sconserr,shell=True)
sconslog.close()
sconserr.close()

buildNotification.sendBuildCompleted("Mantid")
