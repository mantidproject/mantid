from mantidsimple import *
from shutil import copyfile

##############################################################
# Converts a string that represents a list with commas and hyphens into a Python list
##############################################################
def readCommasAndDashes(commaSeparated):
  if commaSeparated == '' : return []
  
  #remove any leading or trailing ','
  if commaSeparated[0] == ',':
    commaSeparated = commaSeparated[1:]
  if commaSeparated[len(commaSeparated)-1] == ',':
    commaSeparated = commaSeparated[0:len(commaSeparated)-1]

  theList = []
  ranges = commaSeparated.split(',')
  
  #these strings may be single numbers or a range of numbers
  for r in ranges :
    ends = r.split('-')
    if len(ends) < 2 :                         #add a single number, maybe a number or a string
      theList.append(r)
    else :                                     #there is a hyphen
      try :                                    #intepreting this as a range of numbers
        start = int(ends[0])                
        end = int(ends[len(ends)-1])           #ranges with more than one hypen are allowed but only the end numbers are intepreted
	if start < end :                           #normally start will be the lowest number
          theList += range(start, end+1)       #range misses off the last number
        else :
          theList += range(end, start+1)       #handle an unusual situation

      except :                                 #could be a hyphen in a file name string, this is allowed
        theList.append(r)    
  return theList

def padZero(runNum, inst='SANS'):
  if inst == 'SANS' : numDigits = 8
  elif inst == 'LOQ' : numDigits = 5
  else : raise NotImplemented('The arguement inst must be set to SANS or LOQ')
  
  run = str(runNum)
  nzeros=numDigits-len(run)
  fpad=""
  for i in range(nzeros):
    fpad+="0"

  return fpad+run
  
def stringToList(commaSeparated):
  if commaSeparated == '' : return []
  #remove any leading or trailing ','
  if commaSeparated[0] == ',':
    commaSeparated = commaSeparated[1:]
  if commaSeparated[len(commaSeparated)-1] == ',':
    commaSeparated = commaSeparated[0:len(commaSeparated)-1]

  theList = []
  numbers = commaSeparated.split(',')
  for quoted in numbers :
    try :
      num = int(quoted)
      theList.append(num)
    except : #we're not demanding that the entries are all integers
      theList.append(quoted)                       
  return theList

def loadWS(entry, type='nxs', inst='sans2d', wsName='AddFilesNewTempory') :
  try :
    runNum = int(entry)                          #the user entered something that translates to a run number, convert it to a file 
    filename=inst+padZero(runNum, 'SANS')+'.'+type
  except ValueError :                            #we don't have a run number, assume it's a valid filename
    filename = entry
    
  mantid.sendLogMessage('reading file:   '+filename)

  if type == 'nxs' :
    props = LoadNexus(Filename=filename,OutputWorkspace=wsName)
  elif type == 'raw' :
    props = LoadRaw(Filename=filename,OutputWorkspace=wsName)
  else :
    raise LookupError('The extension must be one of raw or nxs')
  
  path = props.getPropertyValue('FileName')
  
  #deal with the different format for path names in windows
  if str(path).find(':\\') > -1 :
    separator = '\\'
  else :
    separator = '/'
  #getPropertyValue() returns the full path and rpartition() always returns a 3 member tuple
  pathSepName = path.rpartition(separator)
  #returns filename  filepath
  return pathSepName[2], pathSepName[0] 
 
def add_runs(pathout, runlist, inst='sans2d', type='nxs'):
  pathout += '/'+inst+'/'
  b=range(len(runlist)-1)
  
  if len(runlist) < 1 : return                    #check if there is at least one file in the list
  
  #we need to catch all exceptions to ensure that a dialog box is raised with the error
  try :
    lastFile, lastPath = loadWS(runlist[0], type, inst, 'AddFilesSumTempory')
  
    for i in b:
      lastFile, lastPath = loadWS(runlist[i+1], type, inst,'AddFilesNewTempory')
      Plus('AddFilesSumTempory', 'AddFilesNewTempory', 'AddFilesSumTempory')
      mantid.deleteWorkspace("AddFilesNewTempory")
      
  except ValueError, reason:
    error = 'Error opening file:\n' + reason.message + '\n' + lastPath+'/'+lastFile
    print error
    mantid.sendLogMessage(error)
    if mantid.workspaceExists('AddFilesSumTempory')  : mantid.deleteWorkspace('AddFilesSumTempory')
    return
  except Exception, reason:
    error = 'Error finding files: ' + reason.message
    print error
    mantid.sendLogMessage(error)
    if mantid.workspaceExists('AddFilesSumTempory') : mantid.deleteWorkspace('AddFilesSumTempory')
    if mantid.workspaceExists('AddFilesNewTempory') : mantid.deleteWorkspace("AddFilesNewTempory")
    return
      
  lastFile = lastFile.rpartition('.')[0]
  # now save the added file
  outFile = pathout+lastFile+"-add."+type
  mantid.sendLogMessage('writing file:   '+outFile)
  SaveNexusProcessed("AddFilesSumTempory", outFile)
  mantid.deleteWorkspace("AddFilesSumTempory")
  lastFile += '.log'
  copyfile(lastPath+'/'+lastFile, pathout+os.path.basename(lastFile))