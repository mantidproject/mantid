# Determines upper limit of Mantid.msi file size in MB
import os
import math

sz = int(math.ceil(float(os.stat('Mantid.msi')[6])/1000000))
pfile = open('mantid_version.txt','a')
pfile.write('\n'+str(sz)+'\n')
pfile.close()

