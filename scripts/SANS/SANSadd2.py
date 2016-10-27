#pylint: disable=invalid-name
import os
from mantid.simpleapi import *
from mantid.kernel import Logger
from mantid.api import WorkspaceGroup, IEventWorkspace

from SANSUtility import (bundle_added_event_data_as_group, AddOperation, transfer_special_sample_logs)
from shutil import copyfile

sanslog = Logger("SANS")
_NO_INDIVIDUAL_PERIODS = -1
ADD_FILES_SUM_TEMPORARY = "AddFilesSumTemporary"
ADD_FILES_SUM_TEMPORARY_MONITORS = "AddFilesSumTemporary_monitors"
ADD_FILES_NEW_TEMPORARY = "AddFilesNewTempory"
ADD_FILES_NEW_TEMPORARY_MONITORS = "AddFilesNewTempory_monitors"


class WorkspaceWrapper(object):
    def __init__(self, workspace):
        if workspace is None:
            raise ValueError("WorkspaceWrapper: No valid workspace was provided.")
        self._workspace = workspace
        self._is_group_workspace = self._check_if_group_workspace(self._workspace)
        self._reference_workspace = self._workspace[0] if self._is_group_workspace else self._workspace
        self._is_event_data = self._check_if_is_event_data()

    @staticmethod
    def _check_if_group_workspace(workspace):
        return True if isinstance(workspace, WorkspaceGroup) else False

    def _check_if_is_event_data(self):
        return isinstance(self._reference_workspace, IEventWorkspace)

    def get_workspace(self, period=1):
        if self._is_group_workspace:
            ws = self._workspace[period-1]
        else:
            ws = self._workspace
        return ws

    def getRun(self):
        return self._reference_workspace.getRun()

    def getHistory(self):
        return self._reference_workspace.getHistory()

    @property
    def is_event_workspace(self):
        return self._is_event_data


def add_multi_period_workspace(multi_period_workspace, adder, run_to_add):
    group_workspace_name = multi_period_workspace.name()
    workspace_sum = multi_period_workspace[0]
    for index in range(1, len(multi_period_workspace)):
        adder.add(LHS_workspace=workspace_sum, RHS_workspace=multi_period_workspace[index],
                  output_workspace=workspace_sum, run_to_add=run_to_add)
    # Now delete all workspaces exepct for the first one, starting from the back
    for index in reversed(range(1, len(multi_period_workspace))):
        DeleteWorkspace(multi_period_workspace[index])
    UnGroupWorkspace(multi_period_workspace)
    RenameWorkspace(workspace_sum, group_workspace_name)


def handle_loaded_multi_period_file(workspace_name, monitor_name, adder, run_to_add, is_event):
    workspace_group = mtd[workspace_name]
    add_multi_period_workspace(multi_period_workspace=workspace_group, adder=adder, run_to_add=run_to_add)
    if is_event:
        workspace_group_monitors = mtd[monitor_name]
        add_multi_period_workspace(multi_period_workspace=workspace_group_monitors, adder=adder, run_to_add=run_to_add)


def add_runs(runs, inst='sans2d', defType='.nxs', rawTypes=('.raw', '.s*', 'add','.RAW'), lowMem=False,
             binning='Monitors', saveAsEvent=False, isOverlay = False, time_shifts=None):
    if inst.upper() == "SANS2DTUBES":
        inst = "SANS2D"
  #check if there is at least one file in the list
    if len(runs) < 1 :
        return

    if not defType.startswith('.') :
        defType = '.'+defType

    # Create the correct format of adding files
    if time_shifts is None:
        time_shifts = []
    adder = AddOperation(isOverlay, time_shifts)

  #these input arguments need to be arrays of strings, enforce this
    if isinstance(runs, str) :
        runs = (runs, )
    if isinstance(rawTypes, str) :
        rawTypes = (rawTypes, )

    if lowMem:
        lowMem = _can_load_periods(runs, defType, rawTypes)
    if lowMem:
        period = 1
    else:
        period = _NO_INDIVIDUAL_PERIODS

    userEntry = runs[0]

    counter_run = 0

    while True:

        isFirstDataSetEvent = False
        is_first_data_set_group_workspace = False
    #we need to catch all exceptions to ensure that a dialog box is raised with the error
        try :
            lastPath, lastFile, logFile, num_periods, isFirstDataSetEvent = _loadWS(
                userEntry, defType, inst, ADD_FILES_SUM_TEMPORARY, rawTypes, period)

      # if event data prevent loop over periods makes no sense
            if isFirstDataSetEvent:
                period = _NO_INDIVIDUAL_PERIODS

            is_not_allowed_instrument = inst.upper() != 'SANS2D' and inst.upper() != 'LARMOR'
            if is_not_allowed_instrument and isFirstDataSetEvent:
                error = 'Adding event data not supported for ' + inst + ' for now'
                print error
                logger.notice(error)
                for workspaceName in (ADD_FILES_SUM_TEMPORARY,ADD_FILES_SUM_TEMPORARY_MONITORS):
                    if workspaceName in mtd:
                        DeleteWorkspace(workspaceName)
                return ""

            # If the loaded workspace is a multi-period workspace then we need to add those files together first
            # In this way we get rid of the group workspace
            is_first_data_set_group_workspace = isinstance(mtd[ADD_FILES_SUM_TEMPORARY], WorkspaceGroup)
            if is_first_data_set_group_workspace:
                handle_loaded_multi_period_file(workspace_name= ADD_FILES_SUM_TEMPORARY,
                                                monitor_name=ADD_FILES_SUM_TEMPORARY_MONITORS,
                                                adder=adder, run_to_add=1e10, is_event=isFirstDataSetEvent)

            for i in range(len(runs)-1):
                userEntry = runs[i+1]
                lastPath, lastFile, logFile, dummy, isDataSetEvent = _loadWS(
                    userEntry, defType, inst, ADD_FILES_NEW_TEMPORARY, rawTypes, period)

                if isDataSetEvent != isFirstDataSetEvent:
                    error = 'Datasets added must be either ALL histogram data or ALL event data'
                    print error
                    logger.notice(error)
                    for workspaceName in (ADD_FILES_SUM_TEMPORARY, ADD_FILES_SUM_TEMPORARY_MONITORS,
                                          ADD_FILES_NEW_TEMPORARY, ADD_FILES_NEW_TEMPORARY_MONITORS):
                        if workspaceName in mtd:
                            DeleteWorkspace(workspaceName)
                    return ""

                # If the loaded workspace is a multi-period workspace then we need to add those files together first
                # In this way we get rid of the group worksapce
                if isinstance(mtd[ADD_FILES_NEW_TEMPORARY], WorkspaceGroup):
                    handle_loaded_multi_period_file(workspace_name=ADD_FILES_NEW_TEMPORARY,
                                                    monitor_name=ADD_FILES_NEW_TEMPORARY_MONITORS,
                                                    adder=adder, run_to_add=counter_run, is_event=isDataSetEvent)

                adder.add(LHS_workspace=ADD_FILES_SUM_TEMPORARY,RHS_workspace=ADD_FILES_NEW_TEMPORARY,
                          output_workspace=ADD_FILES_SUM_TEMPORARY, run_to_add=counter_run)

                if isFirstDataSetEvent:
                    adder.add(LHS_workspace=ADD_FILES_SUM_TEMPORARY_MONITORS,
                              RHS_workspace= ADD_FILES_NEW_TEMPORARY_MONITORS,
                              output_workspace=ADD_FILES_SUM_TEMPORARY_MONITORS,
                              run_to_add = counter_run)
                DeleteWorkspace(ADD_FILES_NEW_TEMPORARY)
                if isFirstDataSetEvent:
                    DeleteWorkspace(ADD_FILES_NEW_TEMPORARY_MONITORS)
                # Increment the run number
                counter_run +=1
        except ValueError as e:
            error = 'Error opening file ' + userEntry+': ' + str(e)
            print error
            logger.notice(error)
            if ADD_FILES_SUM_TEMPORARY in mtd :
                DeleteWorkspace(ADD_FILES_SUM_TEMPORARY)
            return ""
        except Exception as e:
            error = 'Error finding files: ' + str(e)
            print error
            logger.notice(error)
            for workspaceName in (ADD_FILES_SUM_TEMPORARY, ADD_FILES_NEW_TEMPORARY):
                if workspaceName in mtd:
                    DeleteWorkspace(workspaceName)
            return ""
    # in case of event file force it into a histogram workspace if this is requested
        if isFirstDataSetEvent and not saveAsEvent:
            handle_saving_event_workspace_when_saving_as_histogram(binning, is_first_data_set_group_workspace,
                                                                   runs, defType, inst)

        lastFile = os.path.splitext(lastFile)[0]
    # now save the added file
        outFile = lastFile+'-add.'+'nxs'
        outFile_monitors = lastFile+'-add_monitors.'+'nxs'
        logger.notice('writing file:   '+outFile)


        if period == 1 or period == _NO_INDIVIDUAL_PERIODS:
        #replace the file the first time around
            SaveNexusProcessed(InputWorkspace=ADD_FILES_SUM_TEMPORARY,
                               Filename=outFile, Append=False)
            # If we are saving event data, then we need to save also the monitor file
            if isFirstDataSetEvent and saveAsEvent:
                SaveNexusProcessed(InputWorkspace=ADD_FILES_SUM_TEMPORARY_MONITORS,
                                   Filename=outFile_monitors, Append=False)

        else:
      #then append
            SaveNexusProcessed(ADD_FILES_SUM_TEMPORARY, outFile, Append=True)
            if isFirstDataSetEvent and saveAsEvent:
                SaveNexusProcessed(ADD_FILES_SUM_TEMPORARY_MONITORS, outFile_monitors , Append=True)

        DeleteWorkspace(ADD_FILES_SUM_TEMPORARY)
        if isFirstDataSetEvent:
            DeleteWorkspace(ADD_FILES_SUM_TEMPORARY_MONITORS)

        if period == num_periods:
            break

        if period == _NO_INDIVIDUAL_PERIODS:
            break
        else:
            period += 1

    if isFirstDataSetEvent and saveAsEvent:
        outFile = bundle_added_event_data_as_group(outFile, outFile_monitors)

  #this adds the path to the filename
    path,base = os.path.split(outFile)
    if path == '' or base not in os.listdir(path):
        # Try the default save directory
        path = config['defaultsave.directory'] + path
        # If the path is still an empty string check in the current working directory
        if path == '':
            path = os.getcwd()
        assert base in os.listdir(path)
    pathout = path
    if logFile:
        _copyLog(lastPath, logFile, pathout)

    return 'The following file has been created:\n'+outFile


def handle_saving_event_workspace_when_saving_as_histogram(binning, is_first_data_set_group_workspace, runs, defType, inst):
    wsInMonitor = mtd[ADD_FILES_SUM_TEMPORARY_MONITORS]
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
    Rebin(InputWorkspace=ADD_FILES_SUM_TEMPORARY,OutputWorkspace='AddFilesSumTempory_Rebin',Params=binning,
            PreserveEvents=False)

    # loading the nexus file using LoadNexus is necessary because it has some metadata
    # that is not in LoadEventNexus. This must be fixed.
    filename, ext = _makeFilename(runs[0], defType, inst)
    if is_first_data_set_group_workspace:
        # If we are dealing with multi-period event workspaces then there is no way of getting any other
        # sample log inforamtion hence we use make a copy of the monitor workspace and use that instead
        # of the reloading the first file again
        CloneWorkspace(InputWorkspace=ADD_FILES_SUM_TEMPORARY_MONITORS, OutputWorkspace=ADD_FILES_SUM_TEMPORARY)
    else:
        LoadNexus(Filename=filename, OutputWorkspace=ADD_FILES_SUM_TEMPORARY, SpectrumMax=wsInMonitor.getNumberHistograms())
    # User may have selected a binning which is different from the default
    Rebin(InputWorkspace=ADD_FILES_SUM_TEMPORARY,OutputWorkspace=ADD_FILES_SUM_TEMPORARY,Params= binning)
    # For now the monitor binning must be the same as the detector binning
    # since otherwise both cannot exist in the same output histogram file
    Rebin(InputWorkspace=ADD_FILES_SUM_TEMPORARY_MONITORS,OutputWorkspace=ADD_FILES_SUM_TEMPORARY_MONITORS,Params= binning)

    wsInMonitor = mtd[ADD_FILES_SUM_TEMPORARY_MONITORS]
    wsOut = mtd[ADD_FILES_SUM_TEMPORARY]
    wsInDetector = mtd['AddFilesSumTempory_Rebin']

    # We loose added sample log information since we reload a single run workspace
    # and conjoin with the added workspace. In order to preserve some added sample
    # logs we need to transfer them at this point
    transfer_special_sample_logs(from_ws = wsInDetector, to_ws = wsOut)

    mon_n = wsInMonitor.getNumberHistograms()
    for i in range(mon_n):
        wsOut.setY(i, wsInMonitor.dataY(i))
        wsOut.setE(i, wsInMonitor.dataE(i))
    ConjoinWorkspaces(wsOut, wsInDetector, CheckOverlapping=True)

    if 'AddFilesSumTempory_Rebin' in mtd :
        DeleteWorkspace('AddFilesSumTempory_Rebin')

def _can_load_periods(runs, defType, rawTypes):
    """
    Searches through the supplied list of run file names and
    returns False if some appear to be raw files else True
  """
    for i in runs:
        dummy, ext = os.path.splitext(i)
        if ext == '':
            ext = defType
        if _isType(ext, rawTypes):
            return False
  #no raw files were found, assume we can specify the period number for each
    return True


def _makeFilename(entry, ext, inst) :
    """
    If entry not already a valid filename make it into one
  """
    try :
        runNum = int(entry)  #the user entered something that translates to a run number, convert it to a file
        filename=inst+_padZero(runNum, inst)+ext
    except ValueError:  #we don't have a run number, assume it's a valid filename
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

    workspace_wrapper = WorkspaceWrapper(outWs)
    isDataSetEvent = workspace_wrapper.is_event_workspace
    if isDataSetEvent:
        LoadEventNexus(Filename=filename, OutputWorkspace=wsName, LoadMonitors=True)
        workspace_wrapper = WorkspaceWrapper(mtd[wsName])
        runDetails = workspace_wrapper.getRun()
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

    props = workspace_wrapper.getHistory().lastAlgorithm()
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
        run = workspace_wrapper.getRun()
        numPeriods = run.getLogData('nperiods').value
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
    except Exception:
        error = 'Error copying log file ' + logFile + ' to directory ' + pathout+'\n'
        print error
        logger.notice(error)

if __name__ == '__main__':
    add_runs(('16183','16197'),'SANS2D','.nxs')
