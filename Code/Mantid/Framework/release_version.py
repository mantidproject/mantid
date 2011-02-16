import os

HEADER='Kernel/inc/MantidKernel/MantidVersion.h'
HEADERPATH=os.path.join(os.path.dirname(__file__), HEADER)

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
  
  if verbose:
    print "Writing version %s into %s " % (version, HEADERPATH)

  f = open(HEADER,'w')

  f.write('//This file is automatically created by Mantid/Code/Mantid/build.bat(sh)\n')
  f.write('#ifndef MANTID_VERSION\n')
  f.write('#define MANTID_VERSION "')
  f.write(version)
  f.write('"\n#endif\n')

  f.close()
  
def getMantidVersion():
  """Get the Mantid Version from the chosen file or the default
  """
  try:
    version_file = open(HEADERPATH, 'r')
    version = "0.0"
    for line in version_file:
        if line.startswith('#define MANTID_VERSION'):
            items = line.split('"')
            if len(items) == 3:
                version = items[1]
                break
    version_file.close()
    return version.strip()
  except:
    return "0.0"
#end def


if __name__ == "__main__":
  writeMantidVersion()
