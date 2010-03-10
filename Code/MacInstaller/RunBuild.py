import subprocess as sp

stdlog = open("../../../../logs/Installer/build.log","w")
stderr = open("../../../../logs/Installer/error.log","w")

sp.call("./buildPackage.sh",stdout=stdlog,stderr=stderr,shell=True)

stdlog.close()
stderr.close()

