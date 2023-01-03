# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=invalid-name
import numpy as np
import os
from shutil import copyfile

from mantid.api import WorkspaceGroup
from mantid.kernel import Logger
from mantid.simpleapi import (
    CloneWorkspace,
    config,
    ConjoinWorkspaces,
    DeleteWorkspace,
    Load,
    LoadEventNexus,
    LoadNexus,
    LoadSampleDetailsFromRaw,
    mtd,
    Rebin,
    RenameWorkspace,
    SaveNexusProcessed,
    UnGroupWorkspace,
)
from SANSUtility import (
    AddOperation,
    transfer_special_sample_logs,
    bundle_added_event_data_as_group,
    WorkspaceType,
    get_workspace_type,
    getFileAndName,
)

sanslog = Logger("SANS")
_NO_INDIVIDUAL_PERIODS = -1
ADD_FILES_SUM_TEMPORARY = "AddFilesSumTemporary"
ADD_FILES_SUM_TEMPORARY_MONITORS = "AddFilesSumTemporary_monitors"
ADD_FILES_NEW_TEMPORARY = "AddFilesNewTemporary"
ADD_FILES_NEW_TEMPORARY_MONITORS = "AddFilesNewTemporary_monitors"


def add_runs(  # noqa: C901
    runs,
    inst="sans2d",
    defType=".nxs",
    rawTypes=(".raw", ".s*", "add", ".RAW"),
    lowMem=False,
    binning="Monitors",
    saveAsEvent=False,
    isOverlay=False,
    time_shifts=None,
    outFile=None,
    outFile_monitors=None,
    save_directory=None,
    estimate_logs=False,
):
    if inst.upper() == "SANS2DTUBES":
        inst = "SANS2D"

    # Check if there is at least one file in the list
    if len(runs) < 1:
        return

    if not defType.startswith("."):
        defType = "." + defType

    # Create the correct format of adding files
    if time_shifts is None:
        time_shifts = []
    adder = AddOperation(isOverlay, time_shifts)

    # These input arguments need to be arrays of strings, enforce this
    if isinstance(runs, str):
        runs = (runs,)
    if isinstance(rawTypes, str):
        rawTypes = (rawTypes,)

    if lowMem:
        if _can_load_periods(runs, defType, rawTypes):
            period = 1
    else:
        period = _NO_INDIVIDUAL_PERIODS

    userEntry = runs[0]

    counter_run = 0

    while True:
        isFirstDataSetEvent = False
        try:
            lastPath, lastFile, logFile, num_periods, isFirstDataSetEvent = _load_ws(
                userEntry, defType, inst, ADD_FILES_SUM_TEMPORARY, rawTypes, period
            )

            is_not_allowed_instrument = inst.upper() not in {"SANS2D", "LARMOR", "ZOOM"}
            if is_not_allowed_instrument and isFirstDataSetEvent:
                error = "Adding event data not supported for " + inst + " for now"
                print(error)
                sanslog.error(error)
                for workspaceName in (ADD_FILES_SUM_TEMPORARY, ADD_FILES_SUM_TEMPORARY_MONITORS):
                    if workspaceName in mtd:
                        DeleteWorkspace(workspaceName)
                return ""

            for i in range(len(runs) - 1):
                userEntry = runs[i + 1]
                lastPath, lastFile, logFile, dummy, is_data_set_event = _load_ws(
                    userEntry, defType, inst, ADD_FILES_NEW_TEMPORARY, rawTypes, period
                )

                if is_data_set_event != isFirstDataSetEvent:
                    error = "Datasets added must be either ALL histogram data or ALL event data"
                    print(error)
                    sanslog.error(error)
                    for workspaceName in (
                        ADD_FILES_SUM_TEMPORARY,
                        ADD_FILES_SUM_TEMPORARY_MONITORS,
                        ADD_FILES_NEW_TEMPORARY,
                        ADD_FILES_NEW_TEMPORARY_MONITORS,
                    ):
                        if workspaceName in mtd:
                            DeleteWorkspace(workspaceName)
                    return ""

                adder.add(
                    LHS_workspace=ADD_FILES_SUM_TEMPORARY,
                    RHS_workspace=ADD_FILES_NEW_TEMPORARY,
                    output_workspace=ADD_FILES_SUM_TEMPORARY,
                    run_to_add=counter_run,
                    estimate_logs=estimate_logs,
                )

                if isFirstDataSetEvent:
                    adder.add(
                        LHS_workspace=ADD_FILES_SUM_TEMPORARY_MONITORS,
                        RHS_workspace=ADD_FILES_NEW_TEMPORARY_MONITORS,
                        output_workspace=ADD_FILES_SUM_TEMPORARY_MONITORS,
                        run_to_add=counter_run,
                        estimate_logs=estimate_logs,
                    )
                DeleteWorkspace(ADD_FILES_NEW_TEMPORARY)
                if isFirstDataSetEvent:
                    DeleteWorkspace(ADD_FILES_NEW_TEMPORARY_MONITORS)
                # Increment the run number
                counter_run += 1
        except ValueError as e:
            error = "Error opening file {}: {}".format(userEntry, str(e))
            print(error)
            sanslog.error(error)
            if ADD_FILES_SUM_TEMPORARY in mtd:
                DeleteWorkspace(ADD_FILES_SUM_TEMPORARY)
            return ""
        except Exception as e:
            # We need to catch all exceptions to ensure that a dialog box is raised with the error
            error = "Error finding files: {}".format(str(e))
            print(error)
            sanslog.error(error)
            for workspaceName in (ADD_FILES_SUM_TEMPORARY, ADD_FILES_NEW_TEMPORARY):
                if workspaceName in mtd:
                    DeleteWorkspace(workspaceName)
            return ""

        # In case of event file force it into a histogram workspace if this is requested
        if isFirstDataSetEvent and not saveAsEvent:
            handle_saving_event_workspace_when_saving_as_histogram(binning, runs, defType, inst)

        lastFile = os.path.splitext(lastFile)[0]
        # Now save the added file
        prefix = save_directory if save_directory else ""
        if outFile is None:
            outFile = prefix + lastFile + "-add." + "nxs"
        if outFile_monitors is None:
            outFile_monitors = prefix + lastFile + "-add_monitors." + "nxs"
        sanslog.notice("Writing file: {}".format(outFile))

        if period == 1 or period == _NO_INDIVIDUAL_PERIODS:
            # Replace the file the first time around
            SaveNexusProcessed(InputWorkspace=ADD_FILES_SUM_TEMPORARY, Filename=outFile, Append=False)
            # If we are saving event data, then we need to save also the monitor file
            if isFirstDataSetEvent and saveAsEvent:
                SaveNexusProcessed(InputWorkspace=ADD_FILES_SUM_TEMPORARY_MONITORS, Filename=outFile_monitors, Append=False)

        else:
            # Then append
            SaveNexusProcessed(ADD_FILES_SUM_TEMPORARY, outFile, Append=True)
            if isFirstDataSetEvent and saveAsEvent:
                SaveNexusProcessed(ADD_FILES_SUM_TEMPORARY_MONITORS, outFile_monitors, Append=True)

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
        filename, ext = _make_filename(runs[0], defType, inst)
        workspace_type = get_workspace_type(filename)
        is_multi_period = True if workspace_type is WorkspaceType.MultiperiodEvent else False
        outFile = bundle_added_event_data_as_group(outFile, outFile_monitors, is_multi_period)

    # This adds the path to the filename
    path, base = os.path.split(outFile)
    if path == "" or base not in os.listdir(path):
        # Try the default save directory
        path_prefix = save_directory if save_directory else config["defaultsave.directory"]
        path = path_prefix + path
        # If the path is still an empty string check in the current working directory
        if path == "":
            path = os.getcwd()
        assert base in os.listdir(path)
    path_out = path
    if logFile:
        _copy_log(lastPath, logFile, path_out)

    return "The following file has been created:\n" + outFile


def handle_saving_event_workspace_when_saving_as_histogram(binning, runs, def_type, inst):
    ws_in_monitor = mtd[ADD_FILES_SUM_TEMPORARY_MONITORS]
    if binning == "Monitors":
        mon_x = ws_in_monitor.dataX(0)
        binning = str(mon_x[0])
        bin_gap = mon_x[1] - mon_x[0]
        binning = binning + "," + str(bin_gap)
        for j in range(2, len(mon_x)):
            next_bin_gap = mon_x[j] - mon_x[j - 1]
            if next_bin_gap != bin_gap:
                bin_gap = next_bin_gap
                binning = binning + "," + str(mon_x[j - 1]) + "," + str(bin_gap)
        binning = binning + "," + str(mon_x[len(mon_x) - 1])

    sanslog.notice(binning)
    Rebin(InputWorkspace=ADD_FILES_SUM_TEMPORARY, OutputWorkspace="AddFilesSumTemporary_Rebin", Params=binning, PreserveEvents=False)

    # loading the nexus file using LoadNexus is necessary because it has some metadata
    # that is not in LoadEventNexus. This must be fixed.
    filename, ext = _make_filename(runs[0], def_type, inst)
    workspace_type = get_workspace_type(filename)
    if workspace_type is WorkspaceType.MultiperiodEvent:
        # If we are dealing with multi-period event workspaces then there is no way of getting any other
        # sample log information hence we use make a copy of the monitor workspace and use that instead
        # of the reloading the first file again
        CloneWorkspace(InputWorkspace=ADD_FILES_SUM_TEMPORARY_MONITORS, OutputWorkspace=ADD_FILES_SUM_TEMPORARY)
    else:
        LoadNexus(Filename=filename, OutputWorkspace=ADD_FILES_SUM_TEMPORARY, SpectrumMax=ws_in_monitor.getNumberHistograms())
    # User may have selected a binning which is different from the default
    Rebin(InputWorkspace=ADD_FILES_SUM_TEMPORARY, OutputWorkspace=ADD_FILES_SUM_TEMPORARY, Params=binning)
    # For now the monitor binning must be the same as the detector binning
    # since otherwise both cannot exist in the same output histogram file
    Rebin(InputWorkspace=ADD_FILES_SUM_TEMPORARY_MONITORS, OutputWorkspace=ADD_FILES_SUM_TEMPORARY_MONITORS, Params=binning)

    ws_in_monitor = mtd[ADD_FILES_SUM_TEMPORARY_MONITORS]
    wsOut = mtd[ADD_FILES_SUM_TEMPORARY]
    ws_in_detector = mtd["AddFilesSumTemporary_Rebin"]

    # We loose added sample log information since we reload a single run workspace
    # and conjoin with the added workspace. In order to preserve some added sample
    # logs we need to transfer them at this point
    transfer_special_sample_logs(from_ws=ws_in_detector, to_ws=wsOut)

    mon_n = ws_in_monitor.getNumberHistograms()
    for i in range(mon_n):
        wsOut.setY(i, ws_in_monitor.dataY(i))
        wsOut.setE(i, ws_in_monitor.dataE(i))
    ConjoinWorkspaces(wsOut, ws_in_detector, CheckOverlapping=True)

    if "AddFilesSumTemporary_Rebin" in mtd:
        DeleteWorkspace("AddFilesSumTemporary_Rebin")


def _can_load_periods(runs, def_type, raw_types):
    """
    Searches through the supplied list of run file names and
    returns False if some appear to be raw files else True
    """
    for i in runs:
        dummy, ext = os.path.splitext(i)
        if ext == "":
            ext = def_type
        if _is_type(ext, raw_types):
            return False
    # No raw files were found, assume we can specify the period number for each
    return True


def _make_filename(entry, ext, inst):
    """
    If entry not already a valid filename make it into one
    """
    try:
        run_num = int(entry)  # The user entered something that translates to a run number, convert it to a file
        filename = inst + _pad_zero(run_num, inst) + ext
    except ValueError:  # We don't have a run number, assume it's a valid filename
        filename = entry
        dummy, ext = os.path.splitext(filename)

    return filename, ext


def remove_unwanted_workspaces(workspace_name, temp_workspace_name, period):
    # Delete all entries except for the period which is requested
    workspaces_to_keep = temp_workspace_name + "_" + str(period)
    group_workspace = mtd[temp_workspace_name]
    workspace_names_to_remove = [element.name() for element in group_workspace if element.name() != workspaces_to_keep]
    for to_remove in workspace_names_to_remove:
        DeleteWorkspace(to_remove)
    # We need to ungroup the group workspace which only contains now a single workspace
    UnGroupWorkspace(group_workspace)
    RenameWorkspace(InputWorkspace=workspaces_to_keep, OutputWorkspace=workspace_name)


def _load_ws(entry, ext, inst, ws_name, raw_types, period=_NO_INDIVIDUAL_PERIODS):
    filename, ext = _make_filename(entry, ext, inst)
    sanslog.notice("reading file:\t{}".format(filename))

    is_data_set_event = False
    workspace_type = get_workspace_type(filename)
    if workspace_type is WorkspaceType.MultiperiodHistogram:
        if period != _NO_INDIVIDUAL_PERIODS:
            outWs = Load(Filename=filename, OutputWorkspace=ws_name, EntryNumber=period)
        else:
            outWs = Load(Filename=filename, OutputWorkspace=ws_name)
    elif workspace_type is WorkspaceType.Histogram:
        outWs = Load(Filename=filename, OutputWorkspace=ws_name)
    elif workspace_type is WorkspaceType.Event or workspace_type is WorkspaceType.MultiperiodEvent:
        is_data_set_event = True
        temp_ws_name = ws_name + "_event_temp"
        temp_ws_name_monitors = temp_ws_name + "_monitors"
        ws_name_monitors = ws_name + "_monitors"

        LoadEventNexus(Filename=filename, OutputWorkspace=temp_ws_name, LoadMonitors=True)
        outWs = mtd[temp_ws_name]
        # If we are dealing with a multiperiod workspace then we must can only use a single period at a
        # time, hence we reload from disk the whole data set every time which is very bad and must be
        # cached in the future
        if isinstance(outWs, WorkspaceGroup):
            remove_unwanted_workspaces(ws_name, temp_ws_name, period)
            remove_unwanted_workspaces(ws_name_monitors, temp_ws_name_monitors, period)
        else:
            RenameWorkspace(InputWorkspace=temp_ws_name, OutputWorkspace=ws_name)
            RenameWorkspace(InputWorkspace=temp_ws_name_monitors, OutputWorkspace=ws_name_monitors)

        run_details = mtd[ws_name].getRun()
        time_array = run_details.getLogData("proton_charge").times

        # There should never be a time increment in the proton charge larger than say "two weeks"
        # SANS2D currently is run at 10 frames per second. This may be incremented to 5Hz
        # (step of 0.2 sec). Although time between frames may be larger due to having the SMP veto switched on,
        # but hopefully not longer than two weeks!
        for i in range(len(time_array) - 1):
            # Cal time dif in seconds
            time_diff = (time_array[i + 1] - time_array[i]) / np.timedelta64(1, "s")
            if time_diff > 172800:
                sanslog.warning(
                    "Time increments in the proton charge log of {} are suspiciously large. "
                    "For example, a time difference of {} seconds has "
                    "been observed.".format(filename, str(time_diff))
                )
                break
    else:
        outWs = Load(Filename=filename, OutputWorkspace=ws_name)

    full_path, __ = getFileAndName(filename)
    path, f_name = os.path.split(full_path)
    if path.find("/") == -1:
        # Looks like we're on a windows system, convert the directory separators
        path = path.replace("\\", "/")

    if _is_type(ext, raw_types):
        LoadSampleDetailsFromRaw(InputWorkspace=ws_name, Filename=path + "/" + f_name)

    # Change below when logs in Nexus files work  file types of .raw need their log files to be copied too
    # if isType(ext, raw_types):
    log_file = os.path.splitext(f_name)[0] + ".log"
    try:
        outWs = mtd[ws_name]
        run = outWs.getRun()
        num_periods = run.getLogData("nperiods").value
    except Exception:
        # Assume the run file didn't support multi-period data and so there is only one period
        num_periods = 1

    return path, f_name, log_file, num_periods, is_data_set_event


def _pad_zero(run_num, inst):
    num_digits = config.getInstrument(inst).zeroPadding(0)
    return str(run_num).zfill(num_digits)


##########################################
# returns true if ext is in the tuple allTypes, ext
# is intended to be a file extension and allTypes a
# list of allowed extensions. '*' at the end is supported


def _is_type(ext, all_types):
    for one_type in all_types:
        one_type = str(one_type)
        if one_type.endswith("*"):
            one_type = one_type[0 : len(one_type) - 1]
            if ext.startswith(one_type):
                return True
        else:
            if ext == one_type:
                return True
    return False


def _copy_log(last_path, log_file, path_out):
    try:
        log_file = os.path.join(last_path, log_file)
        if os.path.exists(log_file):
            copyfile(log_file, os.path.join(path_out, os.path.basename(log_file)))
        else:
            sanslog.notice("Could not find log file %s" % log_file)
    except Exception:
        error = "Error copying log file {} to directory {}\n".format(log_file, path_out)
        print(error)
        sanslog.error(error)


if __name__ == "__main__":
    add_runs(("16183", "16197"), "SANS2D", ".nxs")
