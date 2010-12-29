import os

def getSVNRevision():
  try:
    #To remove deprecation warning in later python version
    import subprocess
    p = subprocess.Popen("svnversion .", shell=True, bufsize=10000,
          stdin=subprocess.PIPE, stdout=subprocess.PIPE, close_fds=True)
    (put, get) = (p.stdin, p.stdout)
  except:
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
    
def getFileVersion():
  try:
    VERSIONFILE = '../../MantidVersion.txt'
    f = open(VERSIONFILE,'r')
    line = f.readline()
    f.close()
    return line.rstrip()
  except:
    return "0.0"


def writeMantidVersion(verbose=False):
  version = getFileVersion() + '.' +getSVNRevision()
  HEADER = 'Kernel/inc/MantidKernel/MantidVersion.h'

  if verbose:
    print "Writing version %s into %s " % (version, HEADER)

  f = open(HEADER,'w')

  f.write('//This file is automatically created by Mantid/Code/Mantid/build.bat(sh)\n')
  f.write('#ifndef MANTID_VERSION\n')
  f.write('#define MANTID_VERSION "')
  f.write(version)
  f.write('"\n#endif\n')

  f.close()

if __name__ == "__main__":
  writeMantidVersion()
