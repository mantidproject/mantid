import os

def getSVNRevision():
  put, get = os.popen4("svnversion .")
  line=get.readline()
  #remove non alphanumeric indicators
  line=line.replace("M","")
  line=line.replace("S","")
  line=line.replace("P","")
  line=line.strip("")
  versionList = line.split(":")
  #get the max version no
  try:
    maxVersion = 0
    for versionStr in versionList:
      if int(versionStr) > maxVersion:
        maxVersion = int(versionStr)
    return str(maxVersion)
  except:
    return versionList[0].rstrip()
#end def

v = '1.0.' +getSVNRevision()

f = open('Kernel/inc/MantidKernel/MantidVersion.h','w')

f.write('//This file is automatically created by Mantid/Code/Mantid/build.bat(sh)\n')
f.write('#ifndef MANTID_VERSION\n')
f.write('#define MANTID_VERSION "')
f.write(v)
f.write('"\n#endif\n')

f.close()

