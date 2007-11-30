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
			print file
			listCpps.append(codeFolder + '/' +  file)
    return listCpps
