# Determines upper limit of Mantid.msi file size in MB
import os
import math
import sys

if len(sys.argv) != 2:
    exit(1)
msiname = sys.argv[1]

sz = int(math.ceil(float(os.stat(msiname)[6])/1000000))
pfile = open('mantid_version.txt','a')
pfile.write('\n'+str(sz)+'\n')
pfile.close()

