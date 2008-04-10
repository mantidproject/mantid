import os
import platform
import sys
import shutil
from socket import gethostname

def procHeaderPath(ln,keyname,out):
  ## Strips the header components if in existance and returns:
  ## Yes this is ugly repetition in a loop.
  if ln.startswith(keyname):
    out=ln[len(keyname)+1:].strip('\n')
    if out[-1]!='/':
      out+='/'
  return out

def procHeader(ln,keyname,out):
  ## Strips the header components if in existance and returns:
  ## Yes this is ugly repetition in a loop.
  if ln.startswith(keyname):
    out=ln[len(keyname)+1:].strip('\n')
  return out
  
def getCPPFiles(codeFolder) :
    listCpps = []
    if os.path.exists(codeFolder):
	files = os.listdir(codeFolder)
	for file in files:
		if file.endswith('.cpp'):
			listCpps.append(codeFolder + '/' +  file)
#			print file
    return listCpps

def getSharedObjects(listCpps, env) :
    listSharedObjects = []
    for f in listCpps :
        listSharedObjects.append(os.path.splitext(f)[0]+env['SHOBJSUFFIX'])
    return listSharedObjects
    
def collectIncludes(start, dest):
	try:
		shutil.rmtree(os.path.abspath(dest))
	except:
		print "collectIncludes: could not delete old folder\n"
	
	uppath=os.path.abspath(start)
	dirpath=os.listdir(uppath)
	incpaths = []
	
	while(len(dirpath)):
		try:
			item=dirpath.pop(0)
			for file in os.listdir(item):
				fullPath= item+"/"+ file
				if (file=="inc"):
					incpaths.insert(0,os.path.abspath(fullPath))
				elif (os.path.isdir(fullPath) and (not file.endswith('svn'))
				and (not file.endswith('test'))):
					dirpath.append(fullPath[3:])
		except:
			pass
	
	for path in incpaths:
		dirpath=os.listdir(path)
		
		while (len(dirpath)):
			item=dirpath.pop(0)
			if (os.path.isdir(path + '/' + item) and(item[0] != '.')):
				shutil.copytree(os.path.abspath(path + '/' + item), dest + '/' + item)




