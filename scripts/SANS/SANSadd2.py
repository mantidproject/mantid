# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
#pylint: disable=invalid-name
from __future__ import (absolute_import, division, print_function)
import os
from mantid.simpleapi import *
from mantid.kernel import Logger
from mantid.api import WorkspaceGroup

from SANSUtility import (AddOperation, transfer_special_sample_logs,
                         bundle_added_event_data_as_group, WorkspaceType,
                         get_workspace_type, getFileAndName)
from shutil import copyfile
import numpy as np

sanslog = Logger("SANS")
_NO_INDIVIDUAL_PERIODS = -1
ADD_FILES_SUM_TEMPORARY = "AddFilesSumTemporary"
ADD_FILES_SUM_TEMPORARY_MONITORS = "AddFilesSumTemporary_monitors"
ADD_FILES_NEW_TEMPORARY = "AddFilesNewTempory"
ADD_FILES_NEW_TEMPORARY_MONITORS = "AddFilesNewTempory_monitors"


def add_runs(runs, # noqa: C901
             inst='sans2d',
             defType='.nxs',
             rawTypes=('.raw', '.s*', 'add','.RAW'),
             lowMem=False,
             binning='Monitors',
             saveAsEvent=False,
             isOverlay=False,
             time_shifts=None,
             outFile=None,
             outFile_monitors=None,
             save_directory=None):
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

            is_not_allowed_instrument = inst.upper() not in {'SANS2D', 'LARMOR', 'ZOOM'}
            if is_not_allowed_instrument and isFirstDataSetEvent:
                error = 'Adding event data not supported for ' + inst + ' for now'
                print(error)
                sanslog.error(error)
                for workspaceName in (ADD_FILES_SUM_TEMPORARY,ADD_FILES_SUM_TEMPORARY_MONITORS):
                    if workspaceName in mtd:
                        DeleteWorkspace(workspaceName)
                return ""

            for i in range(len(runs)-1):
                userEntry = runs[i+1]
                lastPath, lastFile, logFile, dummy, isDataSetEvent = _loadWS(
                    userEntry, defType, inst, ADD_FILES_NEW_TEMPORARY, rawTypes, period)

                if isDataSetEvent != isFirstDataSetEvent:
                    error = 'Datasets added must be either ALL histogram data or ALL event data'
                    print(error)
                    sanslog.error(error)
                    for workspaceName in (ADD_FILES_SUM_TEMPORARY, ADD_FILES_SUM_TEMPORARY_MONITORS,
                                          ADD_FILES_NEW_TEMPORARY, ADD_FILES_NEW_TEMPORARY_MONITORS):
                        if workspaceName in mtd:
                            DeleteWorkspace(workspaceName)
                    return ""

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
            print(error)
            sanslog.error(error)
            if ADD_FILES_SUM_TEMPORARY in mtd :
                DeleteWorkspace(ADD_FILES_SUM_TEMPORARY)
            return ""
        except Exception as e:
            error = 'Error finding files: ' + str(e)
            print(error)
            sanslog.error(error)
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
        outFile = lastFile+'-add.'+'nxs' if outFile is None else outFile
        outFile_monitors = lastFile+'-add_monitors.'+'nxs' if outFile_monitors is None else outFile_monitors
        if save_directory:
            outFile = save_directory + outFile
            outFile_monitors = save_directory + outFile_monitors
        sanslog.notice('writing file:   '+outFile)

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
        filename, ext = _makeFilename(runs[0], defType, inst)
        workspace_type = get_workspace_type(filename)
        is_multi_period = True if workspace_type is WorkspaceType.MultiperiodEvent else False
        outFile = bundle_added_event_data_as_group(outFile, outFile_monitors, is_multi_period)

  #this adds the path to the filename
    path,base = os.path.split(outFile)
    if path == '' or base not in os.listdir(path):
        # Try the default save directory
        path_prefix = save_directory if save_directory else config["defaultsave.directory"]
        path = path_prefix + path
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

    sanslog.notice(binning)
    Rebin(InputWorkspace=ADD_FILES_SUM_TEMPORARY,OutputWorkspace='AddFilesSumTempory_Rebin',Params=binning,
          PreserveEvents=False)

    # loading the nexus file using LoadNexus is necessary because it has some metadata
    # that is not in LoadEventNexus. This must be fixed.
    filename, ext = _makeFilename(runs[0], defType, inst)
    workspace_type = get_workspace_type(filename)
    if workspace_type is WorkspaceType.MultiperiodEvent:
        # If we are dealing with multi-period event workspaces then there is no way of getting any other
        # sample log information hence we use make a copy of the monitor workspace and use that instead
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


def remove_unwanted_workspaces(workspace_name, temp_workspace_name, period):
    # Delete all entries execept for the period which is requested
    workspaces_to_keep = temp_workspace_name + "_" + str(period)
    group_workspace = mtd[temp_workspace_name]
    workspace_names_to_remove = [element.name() for element in group_workspace if element.name() != workspaces_to_keep]
    for to_remove in workspace_names_to_remove:
        DeleteWorkspace(to_remove)
    # We need to ungroup the group workspace which only contains now a single workspace
    UnGroupWorkspace(group_workspace)
    RenameWorkspace(InputWorkspace=workspaces_to_keep, OutputWorkspace=workspace_name)


def _loadWS(entry, ext, inst, wsName, rawTypes, period=_NO_INDIVIDUAL_PERIODS) :
    filename, ext = _makeFilename(entry, ext, inst)
    sanslog.notice('reading file:     '+filename)

    isDataSetEvent = False
    workspace_type = get_workspace_type(filename)
    if workspace_type is WorkspaceType.MultiperiodHistogram:
        if period != _NO_INDIVIDUAL_PERIODS:
            outWs = Load(Filename=filename, OutputWorkspace=wsName, EntryNumber=period)
        else:
            outWs = Load(Filename=filename, OutputWorkspace=wsName)
    elif workspace_type is WorkspaceType.Histogram:
        outWs = Load(Filename=filename, OutputWorkspace=wsName)
    elif workspace_type is WorkspaceType.Event or workspace_type is WorkspaceType.MultiperiodEvent:
        isDataSetEvent = True
        temp_ws_name = wsName + "_event_temp"
        temp_ws_name_monitors = temp_ws_name + "_monitors"
        ws_name_monitors = wsName + "_monitors"

        LoadEventNexus(Filename=filename, OutputWorkspace=temp_ws_name, LoadMonitors=True)
        outWs = mtd[temp_ws_name]
        # If we are dealing with a multiperid workspace then we must can only use a single period at a
        # time, hence we reload from disk the whole data set every time which is very bad and must be
        # cached in the future
        if isinstance(outWs, WorkspaceGroup):
            remove_unwanted_workspaces(wsName, temp_ws_name, period)
            remove_unwanted_workspaces(ws_name_monitors, temp_ws_name_monitors, period)
        else:
            RenameWorkspace(InputWorkspace=temp_ws_name, OutputWorkspace=wsName)
            RenameWorkspace(InputWorkspace=temp_ws_name_monitors, OutputWorkspace=ws_name_monitors)

        runDetails = mtd[wsName].getRun()
        timeArray = runDetails.getLogData("proton_charge").times
        # There should never be a time increment in the proton charge larger than say "two weeks"
        # SANS2D currently is run at 10 frames per second. This may be increated to 5Hz
        # (step of 0.2 sec). Although time between frames may be larger due to having the SMP veto switched on,
        # but hopefully not longer than two weeks!
        for i in range(len(timeArray)-1):
        # cal time dif in seconds
            timeDif = (timeArray[i+1]-timeArray[i]) / np.timedelta64(1, 's')
            if timeDif > 172800:
                sanslog.warning('Time increments in the proton charge log of ' + filename + ' are suspicious large.' +
                                ' For example a time difference of ' + str(timeDif) + " seconds has been observed.")
                break
    else:
        outWs = Load(Filename=filename,OutputWorkspace=wsName)

    full_path, __ = getFileAndName(filename)
    path, fName = os.path.split(full_path)
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
        outWs = mtd[wsName]
        run = outWs.getRun()
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
            sanslog.notice("Could not find log file %s" % logFile)
    except Exception:
        error = 'Error copying log file ' + logFile + ' to directory ' + pathout+'\n'
        print(error)
        sanslog.error(error)


if __name__ == '__main__':
    add_runs(('16183','16197'),'SANS2D','.nxs')
