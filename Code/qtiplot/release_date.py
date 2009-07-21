import os
from datetime import date

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
    return versionList[0]
#end def

svn = getSVNRevision()
f = open('qtiplot/src/Mantid/MantidPlotReleaseDate.h','w')
f.write('#ifndef MANTIDPLOT_RELEASE_DATE\n')
f.write('#define MANTIDPLOT_RELEASE_DATE "')
f.write(date.today().strftime("%d %b %Y"))
f.write(' (Version 1.0')
f.write(', SVN R'+ svn )
f.write(')')
f.write('"\n#endif\n\n')
f.write('#ifndef MANTIDPLOT_RELEASE_VERSION\n')
f.write('#define MANTIDPLOT_RELEASE_VERSION "')
f.write('1.0.'+ svn )
f.write('"\n#endif\n')
f.close()