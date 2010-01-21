
import os
import subprocess
import sys

execfile("release_version.py")

retcode = subprocess.call("scons "+' '.join(sys.argv[1:]), shell=True)
if retcode != 0:
    p = subprocess.call("python ../Third_Party/src/scons-local/scons.py "+' '.join(sys.argv[1:]), shell=True)
