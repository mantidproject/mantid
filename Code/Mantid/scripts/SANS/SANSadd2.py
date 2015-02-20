import os
from mantid.simpleapi import *
from mantid.kernel import Logger
sanslog = Logger("SANS")
from shutil import copyfile

_NO_INDIVIDUAL_PERIODS = -1

def add_runs(runs, inst='sans2d', defType='.nxs', rawTypes=('.raw', '.s*', 'add','.RAW'), lowMem=False, binning='Monitors'):
    if inst.upper() == "SANS2DTUBES":
        inst = "SANS2D"
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
                logger.notice(error)
                for workspaceName in ('AddFilesSumTempory','AddFilesSumTempory_monitors'):
                    if workspaceName in mtd:
                        DeleteWorkspace(workspaceName)
                return ""

            for i in range(len(runs)-1):
                userEntry = runs[i+1]
                lastPath, lastFile, logFile, dummy, isDataSetEvent = _loadWS(
          userEntry, defType, inst,'AddFilesNewTempory', rawTypes, period)

                if isDataSetEvent != isFirstDataSetEvent:
                    error = 'Datasets added must be either ALL histogram data or ALL event data'
                    print error
                    logger.notice(error)
                    for workspaceName in ('AddFilesSumTempory','AddFilesNewTempory'):
                        if workspaceName in mtd:
                            DeleteWorkspace(workspaceName)
                    return ""

                Plus(LHSWorkspace='AddFilesSumTempory',RHSWorkspace= 'AddFilesNewTempory',OutputWorkspace= 'AddFilesSumTempory')
                if isFirstDataSetEvent:
                    Plus(LHSWorkspace='AddFilesSumTempory_monitors',RHSWorkspace= 'AddFilesNewTempory_monitors',OutputWorkspace= 'AddFilesSumTempory_monitors')
                DeleteWorkspace("AddFilesNewTempory")
                if isFirstDataSetEvent:
                    DeleteWorkspace("AddFilesNewTempory_monitors")

        except ValueError as e:
            error = 'Error opening file ' + userEntry+': ' + str(e)
            print error
            logger.notice(error)
            if 'AddFilesSumTempory' in mtd  : DeleteWorkspace('AddFilesSumTempory')
            return ""
        except Exception as e:
            error = 'Error finding files: ' + str(e)
            print error
            logger.notice(error)
            for workspaceName in ('AddFilesSumTempory','AddFilesNewTempory'):
                if workspaceName in mtd:
                    DeleteWorkspace(workspaceName)
            return ""

    # in case of event file force it into a histogram workspace
        if isFirstDataSetEvent:
            wsInMonitor = mtd['AddFilesSumTempory_monitors']
            if binning == 'Monitors':
                monX = wsInMonitor.dataX(0)
                binning = str(monX[0])
                binGap = monX[1] - monX[0]
                binning = binning + "," + str(binGap)
                for j in range(2,len(monX)):
                    nextBinGap = monX[j] - monX[j-1]
                    if nextBinGap != binGap:
                        binGap = nextBinGap
                        binning = binning + "," + str(monX[j-1]) + "," + str(binGap)
                binning = binning + "," + str(monX[len(monX)-1])

            logger.notice(binning)
            Rebin(InputWorkspace='AddFilesSumTempory',OutputWorkspace='AddFilesSumTempory_Rebin',Params= binning, PreserveEvents=False)

        # loading the nexus file using LoadNexus is necessary because it has some metadata
        # that is not in LoadEventNexus. This must be fixed.
            filename, ext = _makeFilename(runs[0], defType, inst)
            LoadNexus(Filename=filename, OutputWorkspace='AddFilesSumTempory', SpectrumMax=wsInMonitor.getNumberHistograms())
        # User may have selected a binning which is different from the default
            Rebin(InputWorkspace='AddFilesSumTempory',OutputWorkspace='AddFilesSumTempory',Params= binning)
        # For now the monitor binning must be the same as the detector binning
        # since otherwise both cannot exist in the same output histogram file
            Rebin(InputWorkspace='AddFilesSumTempory_monitors',OutputWorkspace='AddFilesSumTempory_monitors',Params= binning)

            wsInMonitor = mtd['AddFilesSumTempory_monitors']
            wsOut = mtd['AddFilesSumTempory']
            wsInDetector = mtd['AddFilesSumTempory_Rebin']

            mon_n = wsInMonitor.getNumberHistograms()
            for i in range(mon_n):
                wsOut.setY(i,wsInMonitor.dataY(i))
                wsOut.setE(i,wsInMonitor.dataE(i))
            ConjoinWorkspaces(wsOut, wsInDetector, CheckOverlapping=True)

            if 'AddFilesSumTempory_Rebin' in mtd : DeleteWorkspace('AddFilesSumTempory_Rebin')

        lastFile = os.path.splitext(lastFile)[0]
    # now save the added file
        outFile = lastFile+'-add.'+'nxs'
        logger.notice('writing file:   '+outFile)
        if period == 1 or period == _NO_INDIVIDUAL_PERIODS:
      #replace the file the first time around
            SaveNexusProcessed(InputWorkspace="AddFilesSumTempory",
                               Filename=outFile, Append=False)
        else:
      #then append
            SaveNexusProcessed("AddFilesSumTempory", outFile, Append=True)

        DeleteWorkspace("AddFilesSumTempory")
        if isFirstDataSetEvent:
            DeleteWorkspace("AddFilesSumTempory_monitors")

        if period == num_periods:
            break

        if period == _NO_INDIVIDUAL_PERIODS:
            break
        else:
            period += 1

  #this adds the path to the filename

    path,base = os.path.split(outFile)
    if path == '' or base not in os.listdir(path):
        path = config['defaultsave.directory'] + path
        assert(base in os.listdir(path))
    pathout = path
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
        runNum = int(entry)                                                  #the user entered something that translates to a run number, convert it to a file
        filename=inst+_padZero(runNum, inst)+ext
    except ValueError :                                                        #we don't have a run number, assume it's a valid filename
        filename = entry
        dummy, ext = os.path.splitext(filename)

    return filename, ext

def _loadWS(entry, ext, inst, wsName, rawTypes, period=_NO_INDIVIDUAL_PERIODS) :

    filename, ext = _makeFilename(entry, ext, inst)

    logger.notice('reading file:     '+filename)

    if period != _NO_INDIVIDUAL_PERIODS:
      #load just a single period
        outWs = Load(Filename=filename, OutputWorkspace=wsName, EntryNumber=period)
    else:
        outWs = Load(Filename=filename,OutputWorkspace=wsName)

    props = outWs.getHistory().lastAlgorithm()

    isDataSetEvent = False
    wsDataSet = mtd[wsName]
    if hasattr(wsDataSet, 'getNumberEvents'):
        isDataSetEvent = True

    if isDataSetEvent:
        LoadEventNexus(Filename=filename,OutputWorkspace=wsName, LoadMonitors=True)
        runDetails = mtd[wsName].getRun()
        timeArray = runDetails.getLogData("proton_charge").times
    # There should never be a time increment in the proton charge larger than say "two weeks"
    # SANS2D currently is run at 10 frames per second. This may be increated to 5Hz
    # (step of 0.2 sec). Although time between frames may be larger due to having the SMP veto switched on,
    # but hopefully not longer than two weeks!
        for i in range(len(timeArray)-1):
      # cal time dif in seconds
            timeDif = (timeArray[i+1].total_nanoseconds()-timeArray[i].total_nanoseconds())*1e-9
            if timeDif > 172800:
                sanslog.warning('Time increments in the proton charge log of ' + filename + ' are suspicious large.' +
                          ' For example a time difference of ' + str(timeDif) + " seconds has been observed.")
                break

    path = props.getPropertyValue('FileName')
    path, fName = os.path.split(path)
    if path.find('/') == -1:
    #looks like we're on a windows system, convert the directory separators
        path = path.replace('\\', '/')

    if _isType(ext, rawTypes):
        LoadSampleDetailsFromRaw(InputWorkspace=wsName,Filename= path+'/'+fName)

    logFile = None
  #change below when logs in Nexus files work  file types of .raw need their log files to be copied too
    if True:#_isType(ext, rawTypes):
        logFile = os.path.splitext(fName)[0]+'.log'

    try:
        samp = mtd[wsName].getRun()
        numPeriods = samp.getLogData('nperiods').value
    except:
    #assume the run file didn't support multi-period data and so there is only one period
        numPeriods = 1

    return path, fName, logFile, numPeriods, isDataSetEvent

def _padZero(runNum, inst):
    numDigits = config.getInstrument(inst).zeroPadding(0)
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
        logFile = os.path.join(lastPath, logFile)
        if os.path.exists(logFile):
            copyfile(logFile, os.path.join(pathout, os.path.basename(logFile)))
        else:
            logger.notice("Could not find log file %s" % logFile)
    except Exception, reason:
        error = 'Error copying log file ' + logFile + ' to directory ' + pathout+'\n'
        print error
        logger.notice(error)


if __name__ == '__main__':
    add_runs(('16183','16197'),'SANS2D','.nxs')
