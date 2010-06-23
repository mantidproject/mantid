import subprocess as sp
import time

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

