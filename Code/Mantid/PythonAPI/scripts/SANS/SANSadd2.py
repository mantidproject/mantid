from mantidsimple import *
from shutil import copyfile

def _padZero(runNum, inst='SANS'):
  if inst.upper() == 'SANS' : numDigits = 8
  elif inst.upper() == 'LOQ' : numDigits = 5
  else : raise NotImplemented('The arguement inst must be set to SANS or LOQ')

  run = str(runNum).zfill(numDigits)
  return run

def _loadWS(entry, type='nxs', inst='sans2d', wsName='AddFilesNewTempory') :
  try :
    runNum = int(entry)                          #the user entered something that translates to a run number, convert it to a file 
    filename=inst+_padZero(runNum, inst)+'.'+type
  except ValueError :                            #we don't have a run number, assume it's a valid filename
    filename = entry

  mantid.sendLogMessage('reading file:   '+filename)

  if type.lower() == 'nxs' :
    props = LoadNexus(Filename=filename,OutputWorkspace=wsName)
  elif type.lower() == 'raw' :
    props = LoadRaw(Filename=filename,OutputWorkspace=wsName)
  else :
    raise LookupError('The extension must be one of raw or nxs')

  path = props.getPropertyValue('FileName')

  path, fName = os.path.split(path)
  return path, fName

def add_runs(pathout, runs, inst='sans2d', fileType='nxs'):
  pathout += '/'+inst+'/'

  if type(runs) == str : runs = (runs, )

  indices=range(len(runs)-1)

  if len(runs) < 1 : return                    #check if there is at least one file in the list

  userEntry = runs[0]
  #we need to catch all exceptions to ensure that a dialog box is raised with the error
  try :
    lastPath, lastFile = _loadWS(userEntry, fileType, inst, 'AddFilesSumTempory')

    for i in indices:
      userEntry = runs[i+1]
      lastPath, lastFile = _loadWS(userEntry, fileType, inst,'AddFilesNewTempory')
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
  lastFile += '.log'
  try :
    copyfile(lastPath+'/'+lastFile, pathout+os.path.basename(lastFile))
  except Exception, reason:
    error = 'Error copying log file ' + lastFile
    print error
    mantid.sendLogMessage(error)
    return

  return 'The following file has been created:\n'+outFile