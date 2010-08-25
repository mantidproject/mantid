from mantidsimple import *
from shutil import copyfile

def _padZero(runNum, inst='SANS2D'):
  if inst.upper() == 'SANS2D' : numDigits = 8
  elif inst.upper() == 'LOQ' : numDigits = 5
  else : raise NotImplementedError('The arguement inst must be set to SANS or LOQ')

  run = str(runNum).zfill(numDigits)
  return run
  
##########################################
# returns true if ext is in the tuple allTypes, ext
# is intended to be a file extension and allTypes a
# list of allowed extensions. '*' at the end is supported
def _isType(ext, allTypes):
  for oneType in allTypes:
    oneType = str(oneType)
    if oneType.endswith('*') :
      oneType = oneType[0:len(oneType)-1]
      if ext.startswith(oneType) :
        return True
    else :
      if ext == oneType :
        return True
  return False

def _loadWS(entry, ext, inst, wsName, rawTypes) :
  try :
    runNum = int(entry)                          #the user entered something that translates to a run number, convert it to a file 
    filename=inst+_padZero(runNum, inst)+ext
  except ValueError :                            #we don't have a run number, assume it's a valid filename
    filename = entry
    dummy, ext = os.path.splitext(filename)

  mantid.sendLogMessage('reading file:   '+filename)

  props = Load(Filename=filename,OutputWorkspace=wsName)

  path = props.getPropertyValue('FileName')
  path, fName = os.path.split(path)

  logFile = None
  #file types of .raw need their log files to be copied too
  if _isType(ext, rawTypes):
    logFile = fName.rpartition('.')[0]+'.log'
  
  return path, fName, logFile

def _copyLog(lastPath, logFile, pathout):
  try :
    logFile = lastPath+'/'+logFile
    copyfile(logFile, pathout+os.path.basename(logFile))
  except Exception, reason:
    error = 'Error copying log file ' + logFile + ' to directory' + pathout
    print error
    mantid.sendLogMessage(error)

def add_runs(pathout, runs, inst='sans2d', defType='.nxs', rawTypes=('.raw', '.s*', 'add')):
  pathout += '/'+inst+'/'
  if not defType.startswith('.') : defType = '.'+defType

  #these input arguments need to be arrays of strings, inforce this
  if type(runs) == str : runs = (runs, )
  if type(rawTypes) == str : rawTypes = (rawTypes, )

  indices=range(len(runs)-1)

  if len(runs) < 1 : return                    #check if there is at least one file in the list

  userEntry = runs[0]
  #we need to catch all exceptions to ensure that a dialog box is raised with the error
  try :
    lastPath, lastFile, logFile = _loadWS(userEntry, defType, inst, 'AddFilesSumTempory', rawTypes)

    for i in indices:
      userEntry = runs[i+1]
      lastPath, lastFile, logFile = _loadWS(userEntry, defType, inst,'AddFilesNewTempory', rawTypes)
      Plus('AddFilesSumTempory', 'AddFilesNewTempory', 'AddFilesSumTempory')
      mantid.deleteWorkspace("AddFilesNewTempory")

  except ValueError, reason:
    error = 'Error opening file ' + userEntry+': ' + reason.message
    print error
    mantid.sendLogMessage(error)
    if mantid.workspaceExists('AddFilesSumTempory')  : mantid.deleteWorkspace('AddFilesSumTempory')
    return ""
  except Exception, reason:
    error = 'Error finding files: ' + reason.message
    print error
    mantid.sendLogMessage(error)
    if mantid.workspaceExists('AddFilesSumTempory') : mantid.deleteWorkspace('AddFilesSumTempory')
    if mantid.workspaceExists('AddFilesNewTempory') : mantid.deleteWorkspace("AddFilesNewTempory")
    return ""

  lastFile = lastFile.rpartition('.')[0]
  # now save the added file
  outFile = pathout+lastFile+"-add."+'nxs'
  mantid.sendLogMessage('writing file:   '+outFile)
  SaveNexusProcessed("AddFilesSumTempory", outFile)
  mantid.deleteWorkspace("AddFilesSumTempory")
  
  if not logFile is None:
    _copyLog(lastPath, logFile, pathout)

  return 'The following file has been created:\n'+outFile