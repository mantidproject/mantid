import sys
import os
from datetime import date

HEADER='src/MantidPlotReleaseDate.h'
HEADERPATH=os.path.join(os.path.dirname(__file__), HEADER)

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

def getFileVersion():
  try:
    VERSIONFILE = '../../MantidVersion.txt'
    f = open(VERSIONFILE,'r')
    line = f.readline()
    f.close()
    return line.rstrip()
  except:
    return "0.0"
#end def

def getMantidPlotVersion():
  """Get the MantidPlot Version 
  """
  try:
    version_file = open(HEADERPATH, 'r')
    version = "0.0.0"
    for line in version_file:
        if line.startswith('#define MANTIDPLOT_RELEASE_VERSION'):
            items = line.split('"')
            if len(items) == 3:
                version = items[1]
                break
    version_file.close()
    return version.strip()
  except:
    return "0.0.0"


def main(argv=None):
  if argv is None:
    argv = sys.argv
  svn = ''
  version=getFileVersion()
  if len(argv) > 1:
    version = argv[1]
  if len(argv) > 2:
    svn = argv[2]
  else:
    svn = getSVNRevision()
  f = open(HEADERPATH,'w')
  f.write('#ifndef MANTIDPLOT_RELEASE_DATE\n')
  f.write('#define MANTIDPLOT_RELEASE_DATE "')
  f.write(date.today().strftime("%d %b %Y"))
  f.write(' (Version ' + version)
  f.write(', SVN R'+ svn )
  f.write(')')
  f.write('"\n#endif\n\n')
  f.write('#ifndef MANTIDPLOT_RELEASE_VERSION\n')
  f.write('#define MANTIDPLOT_RELEASE_VERSION "')
  f.write(version + '.'+ svn )
  f.write('"\n#endif\n')
  f.close()
  return 0
#end def

if __name__ == "__main__":
  sys.exit(main())
