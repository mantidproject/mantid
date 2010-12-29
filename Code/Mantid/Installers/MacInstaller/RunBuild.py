import sys
import subprocess as sp
import time

retcode = sp.call("svn up --accept theirs-full --non-interactive --trust-server-cert", shell=True)
if retcode != 0:
    sys.exit(1)

stdlog = open("../../../../logs/Installer/build.log","w")
stderr = open("../../../../logs/Installer/error.log","w")

buildStart = time.time()
sp.call("./buildPackage.sh",stdout=stdlog,stderr=stderr,shell=True)
buildFinish = time.time()

buildTime = buildFinish - buildStart

BuildTimeLog = open("../../../../logs/Installer/timebuild.log", "w")
BuildTimeLog.write(str(buildTime))
BuildTimeLog.close()

stdlog.close()
stderr.close()

