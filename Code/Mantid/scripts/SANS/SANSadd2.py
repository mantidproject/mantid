from mantidsimple import *
from shutil import copyfile

_NO_INDIVIDUAL_PERIODS = -1

def add_runs(runs, inst='sans2d', defType='.nxs', rawTypes=('.raw', '.s*', 'add'), lowMem=False, binning='Monitors'):
  #check if there is at least one file in the list
  if len(runs) < 1 : return

  if not defType.startswith('.') : defType = '.'+defType

  #these input arguments need to be arrays of strings, enforce this
  if type(runs) == str : runs = (runs, )
  if type(rawTypes) == str : rawTypes = (rawTypes, )

  if lowMem:
    lowMem = _can_load_periods(runs, defType, rawTypes)
  if lowMem:
    period = 1
  else:
    period = _NO_INDIVIDUAL_PERIODS
  
  userEntry = runs[0] 
  
  while(True):
      
    isFirstDataSetEvent = False
    #we need to catch all exceptions to ensure that a dialog box is raised with the error
    try :
      lastPath, lastFile, logFile, num_periods, isFirstDataSetEvent = _loadWS(
        userEntry, defType, inst, 'AddFilesSumTempory', rawTypes, period)
      
      # if event data prevent loop over periods makes no sense
      if isFirstDataSetEvent:
          period = _NO_INDIVIDUAL_PERIODS    
          
      if inst.upper() != 'SANS2D' and isFirstDataSetEvent:      
        error = 'Adding event data not supported for ' + inst + ' for now'
        print error
        mantid.sendLogMessage(error)
        if mantid.workspaceExists('AddFilesSumTempory') : mantid.deleteWorkspace('AddFilesSumTempory')
        if mantid.workspaceExists('AddFilesSumTempory_monitors') : mantid.deleteWorkspace('AddFilesSumTempory_monitors')        
        return "" 
  
      for i in range(len(runs)-1):
        userEntry = runs[i+1]
        lastPath, lastFile, logFile, dummy, isDataSetEvent = _loadWS(
          userEntry, defType, inst,'AddFilesNewTempory', rawTypes, period)
                  
        if isDataSetEvent != isFirstDataSetEvent:
            error = 'Datasets added must be either ALL histogram data or ALL event data'
            print error
            mantid.sendLogMessage(error)
            if mantid.workspaceExists('AddFilesSumTempory')  : mantid.deleteWorkspace('AddFilesSumTempory')
            if mantid.workspaceExists('AddFilesNewTempory')  : mantid.deleteWorkspace('AddFilesNewTempory')
            return ""
        
        Plus('AddFilesSumTempory', 'AddFilesNewTempory', 'AddFilesSumTempory')
        if isFirstDataSetEvent:
            Plus('AddFilesSumTempory_monitors', 'AddFilesNewTempory_monitors', 'AddFilesSumTempory_monitors')
        mantid.deleteWorkspace("AddFilesNewTempory")
        if isFirstDataSetEvent:
            mantid.deleteWorkspace("AddFilesNewTempory_monitors")      
              
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

    # in case of event file force it into a histogram workspace
    if isFirstDataSetEvent:
        wsInMonitor = mtd['AddFilesSumTempory_monitors']
        if binning == 'Monitors':
            monX = wsInMonitor.dataX(i)
            binning = str(monX[0])
            binGap = monX[1] - monX[0]
            binning = binning + "," + str(binGap)
            for j in range(2,len(monX)):
                nextBinGap = monX[j] - monX[j-1]
                if nextBinGap != binGap:
                    binGap = nextBinGap
                    binning = binning + "," + str(monX[j-1]) + "," + str(binGap)    
            binning = binning + "," + str(monX[len(monX)-1])
            
        mantid.sendLogMessage(binning)        
        Rebin('AddFilesSumTempory','AddFilesSumTempory_Rebin', binning, PreserveEvents=False)
        
        filename, ext = _makeFilename(runs[0], defType, inst)
        LoadNexus(filename, OutputWorkspace='AddFilesSumTempory')
        # User may have selected a binning which is different from the default
        Rebin('AddFilesSumTempory','AddFilesSumTempory', binning)
        # For now the monitor binning must be the same as the detector binning
        # since otherwise both cannot exist in the same output histogram file
        Rebin('AddFilesSumTempory_monitors','AddFilesSumTempory_monitors', binning)
        
        wsInMonitor = mtd['AddFilesSumTempory_monitors']
        wsOut = mtd['AddFilesSumTempory']    
        wsInDetector = mtd['AddFilesSumTempory_Rebin']
        for i in range(4):
            outY = wsOut.dataY(i)
            outE = wsOut.dataE(i)
            monitorY = wsInMonitor.readY(i)
            monitorE = wsInMonitor.readE(i)            
            for j in range(len(outY)):
                outY[j] = monitorY[j]
                outE[j] = monitorE[j]
                
                
        for i in range(4, 73732):
            outY = wsOut.dataY(i+4)
            outE = wsOut.dataE(i+4)            
            detectorY = wsInDetector.readY(i)
            detectorE = wsInDetector.readE(i)            
            for j in range(len(outY)):
                outY[j] = detectorY[j]   
                outE[j] = detectorE[j]                         
                       
        if mantid.workspaceExists('AddFilesSumTempory_Rebin') : mantid.deleteWorkspace('AddFilesSumTempory_Rebin')

    lastFile = os.path.splitext(lastFile)[0]
    # now save the added file
    outFile = lastFile+'-add.'+'nxs'
    mantid.sendLogMessage('writing file:   '+outFile)
    if period == 1 or period == _NO_INDIVIDUAL_PERIODS:
      #replace the file the first time around
      sav = SaveNexusProcessed("AddFilesSumTempory", outFile, Append=False)
    else:
      #then append
      sav = SaveNexusProcessed("AddFilesSumTempory", outFile, Append=True)
     
    mantid.deleteWorkspace("AddFilesSumTempory")
    if isFirstDataSetEvent:
            mantid.deleteWorkspace("AddFilesSumTempory_monitors")
  
    if period == num_periods:
      break

    if period == _NO_INDIVIDUAL_PERIODS:
      break
    else:
      period += 1

  #this adds the path to the filename
  outFile = sav.getPropertyValue('Filename')
  pathout = os.path.split(outFile)[0]
  if logFile:
    _copyLog(lastPath, logFile, pathout)

  return 'The following file has been created:\n'+outFile

def _can_load_periods(runs, defType, rawTypes):
  """
    Searches through the supplied list of run file names and
    returns False if some appear to be raw files else True
  """ 
  for i in runs:
    dummy, ext = os.path.splitext(i)
    if ext == '': ext = defType
    if _isType(ext, rawTypes):
      return False
  #no raw files were found, assume we can specify the period number for each
  return True


def _makeFilename(entry, ext, inst) :
  """
    If entry not already a valid filename make it into one
  """     
  try :
    runNum = int(entry)                          #the user entered something that translates to a run number, convert it to a file 
    filename=inst+_padZero(runNum, inst)+ext
  except ValueError :                            #we don't have a run number, assume it's a valid filename
    filename = entry
    dummy, ext = os.path.splitext(filename)
    
  return filename, ext    

def _loadWS(entry, ext, inst, wsName, rawTypes, period=_NO_INDIVIDUAL_PERIODS) :
    
  filename, ext = _makeFilename(entry, ext, inst)

  mantid.sendLogMessage('reading file:   '+filename)

  if period != _NO_INDIVIDUAL_PERIODS:
      #load just a single period
      props = Load(Filename=filename,OutputWorkspace=wsName, EntryNumber=period)
  else:
      props = Load(Filename=filename,OutputWorkspace=wsName)

  isDataSetEvent = False
  wsDataSet = mtd[wsName]
  if hasattr(wsDataSet, 'getNumberEvents'):
      isDataSetEvent = True
         
  if isDataSetEvent:
#      if period != _NO_INDIVIDUAL_PERIODS:
#          #load just a single period
#          LoadEventNexus(Filename=filename,OutputWorkspace=wsName, EntryNumber=period, LoadMonitors=True)
#      else:
          LoadEventNexus(Filename=filename,OutputWorkspace=wsName, LoadMonitors=True)      
          
  path = props.getPropertyValue('FileName')
  path, fName = os.path.split(path)
  if path.find('/') == -1:
    #looks like we're on a windows system, convert the directory separators
    path = path.replace('\\', '/')
    
  if _isType(ext, rawTypes):
    LoadSampleDetailsFromRaw(wsName, path+'/'+fName)

  logFile = None
  #change below when logs in Nexus files work  file types of .raw need their log files to be copied too
  if True:#_isType(ext, rawTypes):
    logFile = os.path.splitext(fName)[0]+'.log'
    
  try:
    samp = mtd[wsName].getSampleDetails()
    numPeriods = samp.getLogData('nperiods').value
  except:
    #assume the run file didn't support multi-period data and so there is only one period
    numPeriods = 1
  
  return path, fName, logFile, numPeriods, isDataSetEvent

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

def _copyLog(lastPath, logFile, pathout):
  try :
    logFile = lastPath+'/'+logFile
    if os.path.exists(logFile):
        copyfile(logFile, pathout+'/'+os.path.basename(logFile))
    else:
        mantid.sendLogMessage("Could not find log file %s" % logFile)
  except Exception, reason:
    error = 'Error copying log file ' + logFile + ' to directory ' + pathout+'\n'
    print error
    mantid.sendLogMessage(error)

