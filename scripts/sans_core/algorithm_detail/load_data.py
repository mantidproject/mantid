# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods, invalid-name, fixme, unused-argument
# pylint: disable=R0922e
"""Implementation for the SANSLoad algorithm

This module contains the loading strategies for the currently supported files. Data can be loaded as the
SCATTER ENTRY of the reduction or the TRANSMISSION/DIRECT ENTRY of the reduction

SCATTER ENTRY:
The data of the scatter entry  is loaded as two parts -- the scatter data and the monitors. In the
old version this was loaded together and then split later on, which was very inefficient.

TRANSMISSION/DIRECT ENTRY:
The data is expected to be of histogram-type. The data is loaded as a whole. Most reductions will only use the
monitors but some machines can use a region on the detector as a monitor substitute.

The data can be loaded from:

Standard Nexus-based files directly from the machine:
    Histogram-based file
    Event-based file (only for SCATTER ENTRY)
    Multi-period histogram-based file
    Multi-period event-based file (only for SCATTER ENTRY)
    Single period selection of a multi-period histogram-based file
    Single period selection of a multi-period event-based file (only for SCATTER ENTRY)

Added nexus-based files:
    Added histogram-based file
    Added event-based file (only for SCATTER ENTRY)
    Added multi-period histogram-based file
    Added multi-period event-based file (only for SCATTER ENTRY)
    Single period selection of an added multi-period histogram-based file
    Single period selection of an added multi-period event-based file (only for SCATTER ENTRY)

Standard Raw-based files directly from the machine:
    Histogram-based file
    Multi-period histogram-based file
    Single period selection of a multi-period histogram-based file


CACHING:
Adding to the cache(ADS) is supported for the TubeCalibration file.
Reading from the cache is supported for all files. This avoids data reloads if the correct file is already in the
cache.
"""

from abc import ABCMeta, abstractmethod
import os
from mantid.kernel import config
from mantid.api import AnalysisDataService
from sans_core.common.file_information import SANSFileInformationFactory, FileType, get_extension_for_file_type, find_full_file_path
from sans_core.common.constants import (
    EMPTY_NAME,
    SANS_SUFFIX,
    TRANS_SUFFIX,
    MONITOR_SUFFIX,
    CALIBRATION_WORKSPACE_TAG,
    SANS_FILE_TAG,
    OUTPUT_WORKSPACE_GROUP,
    OUTPUT_MONITOR_WORKSPACE,
    OUTPUT_MONITOR_WORKSPACE_GROUP,
)
from sans_core.common.enums import SANSFacility, SANSDataType, SANSInstrument
from sans_core.common.general_functions import create_child_algorithm
from sans_core.common.log_tagger import set_tag, has_tag, get_tag
from sans_core.state.StateObjects.StateData import StateData
from sans_core.algorithm_detail.calibration import apply_calibration


# ----------------------------------------------------------------------------------------------------------------------
# General functions
# ----------------------------------------------------------------------------------------------------------------------
def update_file_information(file_information_dict, factory, data_type, file_name):
    info = factory.create_sans_file_information(file_name)
    file_information_dict.update({data_type: info})


def get_file_and_period_information_from_data(data):
    """
    Get the file information and period information from a StateData object

    :param data: a StateData object
    :return: a map of SANSFileInformation objects and map of period information
    """
    file_information_factory = SANSFileInformationFactory()
    file_information = dict()
    period_information = dict()
    if data.sample_scatter:
        update_file_information(file_information, file_information_factory, SANSDataType.SAMPLE_SCATTER, data.sample_scatter)
        period_information.update({SANSDataType.SAMPLE_SCATTER: data.sample_scatter_period})
    if data.sample_transmission:
        update_file_information(file_information, file_information_factory, SANSDataType.SAMPLE_TRANSMISSION, data.sample_transmission)
        period_information.update({SANSDataType.SAMPLE_TRANSMISSION: data.sample_transmission_period})
    if data.sample_direct:
        update_file_information(file_information, file_information_factory, SANSDataType.SAMPLE_DIRECT, data.sample_direct)
        period_information.update({SANSDataType.SAMPLE_DIRECT: data.sample_direct_period})
    if data.can_scatter:
        update_file_information(file_information, file_information_factory, SANSDataType.CAN_SCATTER, data.can_scatter)
        period_information.update({SANSDataType.CAN_SCATTER: data.can_scatter_period})
    if data.can_transmission:
        update_file_information(file_information, file_information_factory, SANSDataType.CAN_TRANSMISSION, data.can_transmission)
        period_information.update({SANSDataType.CAN_TRANSMISSION: data.can_transmission_period})
    if data.can_direct:
        update_file_information(file_information, file_information_factory, SANSDataType.CAN_DIRECT, data.can_direct)
        period_information.update({SANSDataType.CAN_DIRECT: data.can_direct_period})
    return file_information, period_information


def is_transmission_type(to_check):
    """
    Checks if a SANSDataType object is of transmission type.

    Transmission type data are transmission and direct files.
    :param to_check: A SANSDataType object.
    :return: true if the SANSDataType object is a transmission object (transmission or direct) else false.
    """
    return (
        (to_check is SANSDataType.SAMPLE_TRANSMISSION)
        or (to_check is SANSDataType.SAMPLE_DIRECT)
        or (to_check is SANSDataType.CAN_TRANSMISSION)
        or (to_check is SANSDataType.CAN_DIRECT)
    )


def get_expected_file_tags(file_information, is_transmission, period):
    """
    Creates the expected file tags for SANS workspaces.

    :param file_information: a file information object
    :param is_transmission: if the file information is for a transmission or not
    :param period: the period of interest
    :return: a list of file tags
    """
    suffix_file_type = get_extension_for_file_type(file_information)
    suffix_data = TRANS_SUFFIX if is_transmission else SANS_SUFFIX
    file_path = file_information.get_file_name()

    # Three possibilities:
    #  1. No period data => 22024_sans_nxs
    #  2. Period data, but wants all => 22025p1_sans_nxs,  22025p2_sans_nxs, ...
    #  3. Period data, select particular period => 22025p3_sans_nxs
    if file_information.get_number_of_periods() == 1:
        file_tag_name = "{0}_{1}_{2}".format(file_path, suffix_data, suffix_file_type)
        names = [file_tag_name]
    elif file_information.get_number_of_periods() > 1 and period is StateData.ALL_PERIODS:
        file_tag_names = []
        for period in range(1, file_information.get_number_of_periods() + 1):
            file_tag_names.append("{0}p{1}_{2}_{3}".format(file_path, period, suffix_data, suffix_file_type))
        names = file_tag_names
    elif file_information.get_number_of_periods() > 1 and period is not StateData.ALL_PERIODS:
        file_tag_name = "{0}p{1}_{2}_{3}".format(file_path, period, suffix_data, suffix_file_type)
        names = [file_tag_name]
    else:
        raise RuntimeError("SANSLoad: Cannot create workspace names.")
    return names


def is_data_transmission_and_event_mode(file_infos):
    """
    Checks if a file is used as a transmission workspace and contains event-mode data. This is not allowed.

    :param file_infos: a dict of DataType vs FileInformation objects
    :return: True if the file setting is bad else False
    """
    is_bad_file_setting = False
    for key, value in list(file_infos.items()):
        if is_transmission_type(key) and value.is_event_mode():
            is_bad_file_setting = True
            break
    return is_bad_file_setting


# ----------------------------------------------------------------------------------------------------------------------
# Caching workspaces
# ----------------------------------------------------------------------------------------------------------------------
def add_workspaces_to_analysis_data_service(workspaces, workspace_names, is_monitor):
    """
    Adds a list of workspaces to the ADS.

    :param workspaces: list of workspaces
    :param workspace_names: the names under which they are to be published
    :param is_monitor: if the workspace is a monitor or not
    """
    if is_monitor:
        workspace_names = [workspace_name + MONITOR_SUFFIX for workspace_name in workspace_names]
    if len(workspaces) != len(workspace_names):
        raise RuntimeError(
            "SANSLoad: There is a mismatch between the generated names and the length of"
            " the WorkspaceGroup. The workspace has {0} entries and there are {1} "
            "workspace names".format(len(workspaces), len(workspace_names))
        )

    for index in range(0, len(workspaces)):
        if not AnalysisDataService.doesExist(workspace_names[index]):
            AnalysisDataService.addOrReplace(workspace_names[index], workspaces[index])


def publish_workspaces_to_analysis_data_service(workspaces, workspace_monitors, workspace_names):
    """
    Publish data workspaces and monitor workspaces to the ADS.

    :param workspaces: a list of data workspaces (scatter, transmission, direct)
    :param workspace_monitors:  a list of monitor workspaces
    :param workspace_names: the workspace names
    :return:
    """
    add_workspaces_to_analysis_data_service(workspaces, workspace_names, is_monitor=False)

    # If the workspace monitor exists, then add it to the ADS as well
    if workspace_monitors:
        add_workspaces_to_analysis_data_service(workspace_monitors, workspace_names, is_monitor=True)


def has_loaded_correctly_from_ads(file_information, workspaces, period):
    """
    Checks if the workspaces which were supposed to be loaded from the ADS were loaded.

    This might be important for multi-period data.
    :param file_information: A SANSFileInformation object.
    :param workspaces: a list of workspaces.
    :param period: the selected period
    :return: true if loading from the ADS was successful else false.
    """
    number_of_workspaces = len(workspaces)
    number_of_periods = file_information.get_number_of_periods()

    # Different cases: single-period, multi-period, multi-period with one period selected
    if number_of_periods == 1:
        is_valid = True if number_of_workspaces == 1 else False
    elif number_of_periods > 1 and period is not StateData.ALL_PERIODS:
        is_valid = True if number_of_workspaces == 1 else False
    elif number_of_periods > 1 and period is StateData.ALL_PERIODS:
        is_valid = True if number_of_workspaces == number_of_periods else False
    else:
        raise RuntimeError(
            "SANSLoad: Loading data from the ADS has resulted in the a mismatch between the number of "
            "period information and the number of loaded workspaces"
        )
    return is_valid


def is_calibration_correct(workspace, calibration_file):
    """
    Check if the calibration has been applied. If no calibration has been specified then none should be have
    been applied.

    :param workspace: the workspace to check.
    :param calibration_file: the path to the calibration file.
    :return: True if the calibration file matches or if none was set and the path is empty, else False
    """
    has_calibration = has_tag(CALIBRATION_WORKSPACE_TAG, workspace)
    return (has_calibration and calibration_file == get_tag(CALIBRATION_WORKSPACE_TAG, workspace)) or (
        not has_calibration and not calibration_file
    )


def get_workspaces_from_ads_if_exist(file_tags, full_calibration_file_path, workspaces):
    """
    Retrieves workspaces from the ADS depending on their file tags and calibration file tags which would have been
    set by the sans loading mechanism when they were loaded the first time.

    :param file_tags: a list of file tags which we look for on the workspaces on the ADS
    :param full_calibration_file_path: the calibration file name which we look for on the workspaces on the ADS
    :param workspaces: a list of workspaces which is being updated in this function.
    """
    for workspace_name in AnalysisDataService.getObjectNames():
        workspace = AnalysisDataService.retrieve(workspace_name)
        try:
            if has_tag(SANS_FILE_TAG, workspace):
                file_tag = get_tag(SANS_FILE_TAG, workspace)
                if file_tag in file_tags and is_calibration_correct(workspace, full_calibration_file_path):
                    workspaces.append(workspace)
        except RuntimeError:
            continue


def use_cached_workspaces_from_ads(file_information, is_transmission, period, calibration_file_name):
    """
    Use cached workspaces from the ADS. This goes through the workspaces on the ADS and check on their sample logs
    if there is an entry called sans_original_file_name and

    This optimization uses already loaded workspaces from the ADS.
    :param file_information: a SANSFileInformation object.
    :param is_transmission: true if the workspaces are of transmission type
    :param period: the selected period.
    :param calibration_file_name: the name of the calibration file
    :return: a list of workspaces and a list of monitors loaded from the ADS.
    """
    workspaces = []
    workspace_monitors = []

    full_calibration_file_path = find_full_file_path(calibration_file_name) if calibration_file_name else ""

    # Get the expected sans_original_workspace tag entries
    file_tags = get_expected_file_tags(file_information, is_transmission, period)
    get_workspaces_from_ads_if_exist(file_tags, full_calibration_file_path, workspaces)

    if not is_transmission:
        file_tags_monitors = [file_tag + MONITOR_SUFFIX for file_tag in file_tags]
        get_workspaces_from_ads_if_exist(file_tags_monitors, full_calibration_file_path, workspace_monitors)

    # Check if all required workspaces could be found on the ADS. For now, we allow only full loading, ie we don't
    # allow picking up some child workspaces of a multi-period file from the ADS and having to load others. Either
    # all are found in the ADS or we have to reload again. If we are loading a scatter workspace and the monitors
    # are not complete, then we have to load the regular workspaces as well
    if not has_loaded_correctly_from_ads(file_information, workspaces, period):
        workspaces = []
    if not is_transmission and not has_loaded_correctly_from_ads(file_information, workspace_monitors, period):
        workspaces = []
        workspace_monitors = []

    return workspaces, workspace_monitors


def tag_workspaces_with_file_names(workspaces, file_information, is_transmission, period, is_monitor):
    """
    Set a sample log element for the used original file. Note that the calibration file name is set

    :param workspaces: a dict of workspaces
    :param file_information: a SANSFileInformation object
    :param is_transmission: if is transmission.
    :param period: the selected period.
    :param is_monitor: if we are dealing with a monitor
    """
    # Set tag for the original file name from which the workspace was loaded

    file_tags = get_expected_file_tags(file_information, is_transmission, period)
    if len(file_tags) != len(workspaces):
        raise RuntimeError("Issue while tagging the loaded data. The number of tags does not match the number of workspaces.")
    for file_tag, workspace in zip(file_tags, workspaces):
        if not has_tag(SANS_FILE_TAG, workspace):
            if is_monitor:
                file_tag += MONITOR_SUFFIX
            set_tag(SANS_FILE_TAG, file_tag, workspace)


# ----------------------------------------------------------------------------------------------------------------------
# Loading strategies
# ----------------------------------------------------------------------------------------------------------------------


# -----------------
# Added data loader
# -----------------
def run_added_loader(loader, file_information, is_transmission, period, parent_alg):
    """
    Runs the loader for added workspaces.

    This is a complicated matter. The added workspaces can be histogram- or event-based and can consist of
    multi-period data.
    1. Histogram Data: Since we use LoadNexusProcessed we cannot load the monitors separately. We have to make use of
       the algorithm ExtractMonitors in order to split the detector from the monitor
       (if we are dealing with non-transmission data)
    2. Event Data: The added event data and the corresponding monitor data set are stored as two separate units in the
       file. There are several cases to consider
       i. We only have one period, this means that the first entry is the added event data and the second
       entry is the added monitor data
       ii. We have N periods. The first N entries are the added event data and the second N entries are the
       corresponding monitors.
       iii. We have N periods but only want to load period K. We get again only two entries but we need to
       request the kth entry for the added event data and the k + NumPeriods entry for the monitor.

    :param loader: a handles to a preset load algorithm
    :param file_information: the FileInformation object
    :param is_transmission: if  the set is a transmission
    :param period: the selected period
    :param parent_alg: a handle to the parent algorithm
    :return: workspaces and monitors
    """

    def extract_histogram_data(load_alg, num_periods, selected_period):
        ws_collection = []
        if num_periods == 1:
            ws_collection.append(load_alg.getProperty("OutputWorkspace").value)
        elif num_periods > 1 and selected_period is not StateData.ALL_PERIODS:
            ws_collection.append(load_alg.getProperty("OutputWorkspace").value)
        else:
            for index in range(1, num_periods + 1):
                ws_collection.append(load_alg.getProperty(OUTPUT_WORKSPACE_GROUP + str(index)).value)
        return ws_collection

    def extract_event_data(load_alg, num_periods, selected_period):
        ws_collection = []
        ws_monitor_collection = []
        if num_periods == 1 or (num_periods > 1 and selected_period is not StateData.ALL_PERIODS):
            # First get the added event data
            period_to_load = selected_period if selected_period is not StateData.ALL_PERIODS else 1
            offset = num_periods
            load_alg.setProperty("EntryNumber", period_to_load)
            load_alg.execute()
            ws_collection.append(load_alg.getProperty("OutputWorkspace").value)

            # Second get the added monitor data
            load_alg.setProperty("EntryNumber", period_to_load + offset)
            load_alg.execute()
            ws_monitor_collection.append(load_alg.getProperty("OutputWorkspace").value)
        else:
            load_alg.execute()
            workspace_indices = list(range(1, number_of_periods + 1))
            monitor_indices = list(range(number_of_periods + 1, number_of_periods * 2 + 1))
            for workspace_index, monitor_index in zip(workspace_indices, monitor_indices):
                ws_collection.append(load_alg.getProperty(OUTPUT_WORKSPACE_GROUP + str(workspace_index)).value)
                ws_monitor_collection.append(load_alg.getProperty(OUTPUT_WORKSPACE_GROUP + str(monitor_index)).value)
        return ws_collection, ws_monitor_collection

    workspaces = []
    workspace_monitors = []
    number_of_periods = file_information.get_number_of_periods()

    if file_information.is_event_mode():
        if is_transmission:
            if number_of_periods > 1:
                raise RuntimeError("A multiperiod transmission file is not currently supported")
            _, trans_monitors = extract_event_data(loader, number_of_periods, period)
            # Despite the fact transmission runs are only monitors, existing code places
            # them into workspace as they are the only relevant data (from LoadNexusMonitors)
            # so we need to match the convention
            workspaces.extend(trans_monitors)
        else:
            workspaces, workspace_monitors = extract_event_data(loader, number_of_periods, period)
    else:
        # In the case of histogram data we need to consider the following.
        # The data is combined with the monitors since we load with LoadNexusProcessed. Hence we need to split the
        # workspace at this point with ExtractMonitors if we are not looking at a transmission run.
        loader.execute()
        workspace_collection = extract_histogram_data(loader, number_of_periods, period)
        if not is_transmission:
            extract_name = "ExtractMonitors"
            extract_options = {"DetectorWorkspace": "dummy1", "MonitorWorkspace": "dummy2"}
            extract_alg = create_child_algorithm(parent_alg, extract_name, **extract_options)
            for workspace in workspace_collection:
                extract_alg.setProperty("InputWorkspace", workspace)
                extract_alg.execute()
                workspaces.append(extract_alg.getProperty("DetectorWorkspace").value)
                workspace_monitors.append(extract_alg.getProperty("MonitorWorkspace").value)
        else:
            for workspace in workspace_collection:
                workspaces.append(workspace)
    return workspaces, workspace_monitors


def loader_for_added_isis_nexus(file_information, is_transmission, period, parent_alg):
    """
    Get the name and options for the load algorithm for ISIS nexus.

    :param file_information: a SANSFileInformation object.
    :param is_transmission: if the current file corresponds to transmission data
    :param period: the period to load
    :param parent_alg: a handle to the parent algorithm
    :return: the name of the load algorithm and the selected load options
    """
    loader_name = "LoadNexusProcessed"
    loader_options = {
        "Filename": file_information.get_file_name(),
        "OutputWorkspace": EMPTY_NAME,
        "LoadHistory": True,
        "FastMultiPeriod": True,
    }
    if period != StateData.ALL_PERIODS:
        loader_options.update({"EntryNumber": period})
    loader_alg = create_child_algorithm(parent_alg, loader_name, **loader_options)
    return run_added_loader(loader_alg, file_information, is_transmission, period, parent_alg)


# -----------------
# Nexus data loader
# -----------------
def extract_multi_period_event_workspace(loader, index, output_workspace_property_name, parent_alg):
    """
    Extract a single workspace from a WorkspaceGroup.

    Note that we need to perform a CloneWorkspace operation because this is the only way to get an individual workspace
    from a WorkspaceGroup. They are extremely "sticky" and using the indexed access will only provide a weak pointer
    which means that we will have a dead reference once the WorkspaceGroup goes out of scope
    :param loader: an executed LoadEventNexus algorithm
    :param index: an index variable into the GroupWorkspace, not that it is offset by 1
    :param output_workspace_property_name: the name of the output workspace property, i.e. OutputWorkspace or
                                           MonitorWorkspace
    :param parent_alg: a handle to the parent algorithm
    :return: a single workspace
    """
    group_workspace = loader.getProperty(output_workspace_property_name).value
    group_workspace_index = index - 1
    workspace_of_interest = group_workspace[group_workspace_index]

    clone_name = "CloneWorkspace"
    clone_options = {"InputWorkspace": workspace_of_interest, "OutputWorkspace": EMPTY_NAME}
    clone_alg = create_child_algorithm(parent_alg, clone_name, **clone_options)
    clone_alg.execute()
    return clone_alg.getProperty("OutputWorkspace").value


def loader_for_isis_nexus(file_information, is_transmission, period, parent_alg):
    """
    Get name and the options for the loading algorithm.

    This takes a SANSFileInformation object and provides the inputs for the adequate loading strategy.
    :param file_information: a SANSFileInformation object.
    :param is_transmission: if the workspace is a transmission workspace.
    :param period: the period to load.
    :param parent_alg: a handle to the parent algorithm
    :return: the name of the load algorithm and the selected load options.
    """
    loader_options = {"Filename": file_information.get_file_name(), "OutputWorkspace": EMPTY_NAME}
    if file_information.is_event_mode() and not is_transmission:
        loader_name = "LoadEventNexus"
        # Note that currently we don't have a way to only load one monitor
        loader_options.update({"LoadMonitors": True})
    elif not file_information.is_event_mode() and not is_transmission:
        loader_name = "LoadISISNexus"
        loader_options.update({"LoadMonitors": "Separate", "EntryNumber": 0})
        if period != StateData.ALL_PERIODS:
            loader_options.update({"EntryNumber": period})
    elif file_information.is_event_mode() and is_transmission:
        # We have the rare case of an event file which is used for transmission calculations. In this case
        # we only extract the monitors
        loader_name = "LoadNexusMonitors"
    else:
        # We must be dealing with a transmission file, we need to load the whole file.
        # The file itself will most of the time only contain monitors anyway, but sometimes the detector
        # is used as a sort of monitor, hence we cannot sort out the monitors.
        loader_name = "LoadISISNexus"
        loader_options.update({"LoadMonitors": "Include", "EntryNumber": 0})
        if period != StateData.ALL_PERIODS:
            loader_options.update({"EntryNumber": period})
    loader_alg = create_child_algorithm(parent_alg, loader_name, **loader_options)
    return run_loader(loader_alg, file_information, is_transmission, period, parent_alg)


# ---------------
# Raw data loader
# ---------------
def loader_for_raw(file_information, is_transmission, period, parent_alg):
    """
    Get the load algorithm information for an raw file

    :param file_information: a SANSFileInformation object.
    :param is_transmission: if the workspace is a transmission workspace.
    :param period: the period to load.
    :param parent_alg: a handle to the parent algorithm
    :return: the name of the load algorithm and the selected load options.
    """
    loader_name = "LoadRaw"
    loader_options = {"Filename": file_information.get_file_name(), "OutputWorkspace": EMPTY_NAME}
    if is_transmission:
        loader_options.update({"LoadMonitors": "Include"})
    else:
        loader_options.update({"LoadMonitors": "Separate"})

    if period != StateData.ALL_PERIODS:
        loader_options.update({"PeriodList": period})
    loader_alg = create_child_algorithm(parent_alg, loader_name, **loader_options)
    workspaces, monitor_workspaces = run_loader(loader_alg, file_information, is_transmission, period, parent_alg)

    # Add the sample details to the loaded workspace
    sample_name = "LoadSampleDetailsFromRaw"
    sample_options = {"Filename": file_information.get_file_name()}
    sample_alg = create_child_algorithm(parent_alg, sample_name, **sample_options)

    for workspace in workspaces:
        sample_alg.setProperty("InputWorkspace", workspace)
        sample_alg.execute()

    for monitor_workspace in monitor_workspaces:
        sample_alg.setProperty("InputWorkspace", monitor_workspace)
        sample_alg.execute()

    return workspaces, monitor_workspaces


# ---------------
# General
# ---------------
def run_loader(loader, file_information, is_transmission, period, parent_alg):
    """
    Runs the load algorithm.

    This is a generalization which works for Raw and Nexus files which come directly from the instrument.
    :param loader: a handle to the selected load algorithm/strategy
    :param file_information: a SANSFileInformation object
    :param is_transmission: if the workspace is a transmission workspace.
    :param period: the selected period.
    :param parent_alg: a handle to the parent algorithm
    :return: a list of workspaces and a list of monitor workspaces
    """
    loader.execute()

    # Get all output workspaces
    number_of_periods = file_information.get_number_of_periods()

    workspaces = []
    # Either we have a single-period workspace or we want a single period from a multi-period workspace in which case
    # we extract it via OutputWorkspace or we want all child workspaces of a multi-period workspace in which case we
    # need to extract it via OutputWorkspace_1, OutputWorkspace_2, ...
    # Important note: We cannot just grab the individual periods from the GroupWorkspace since all we get from
    # the group workspace is a weak pointer, which invalidates our handle as soon as the group workspace goes
    # out of scope. All of this makes sense for the ADS, but is a pain otherwise.
    if number_of_periods == 1:
        workspaces.append(loader.getProperty("OutputWorkspace").value)
    elif number_of_periods > 1 and period is not StateData.ALL_PERIODS:
        if file_information.is_event_mode():
            workspaces.append(extract_multi_period_event_workspace(loader, period, "OutputWorkspace", parent_alg))
        else:
            workspaces.append(loader.getProperty("OutputWorkspace").value)
    else:
        for index in range(1, number_of_periods + 1):
            if file_information.is_event_mode():
                workspaces.append(extract_multi_period_event_workspace(loader, index, "OutputWorkspace", parent_alg))
            else:
                workspaces.append(loader.getProperty(OUTPUT_WORKSPACE_GROUP + str(index)).value)

    workspace_monitors = []
    if not is_transmission:
        if number_of_periods == 1:
            workspace_monitors.append(loader.getProperty(OUTPUT_MONITOR_WORKSPACE).value)
        elif number_of_periods > 1 and period is not StateData.ALL_PERIODS:
            if file_information.is_event_mode():
                workspace_monitors.append(extract_multi_period_event_workspace(loader, period, OUTPUT_MONITOR_WORKSPACE, parent_alg))
            else:
                workspace_monitors.append(loader.getProperty(OUTPUT_MONITOR_WORKSPACE).value)
        else:
            for index in range(1, number_of_periods + 1):
                if file_information.is_event_mode():
                    workspace_monitors.append(extract_multi_period_event_workspace(loader, index, OUTPUT_MONITOR_WORKSPACE, parent_alg))
                else:
                    workspace_monitors.append(loader.getProperty(OUTPUT_MONITOR_WORKSPACE_GROUP + str(index)).value)
    if workspaces:
        tag_workspaces_with_file_names(workspaces, file_information, is_transmission, period, is_monitor=False)
    if workspace_monitors:
        tag_workspaces_with_file_names(workspace_monitors, file_information, is_transmission, period, is_monitor=True)
    return workspaces, workspace_monitors


def get_loader_strategy(file_information):
    """
    Selects a loading strategy depending on the file type and if we are dealing with a transmission

    :param file_information: a SANSFileInformation object.
    :return: a handle to the correct loading function/strategy.
    """
    if file_information.get_type() == FileType.ISIS_NEXUS:
        loader = loader_for_isis_nexus
    elif file_information.get_type() == FileType.ISIS_RAW:
        loader = loader_for_raw
    elif file_information.get_type() == FileType.ISIS_NEXUS_ADDED:
        loader = loader_for_added_isis_nexus
    else:
        raise RuntimeError("SANSLoad: Cannot load SANS file of type {0}".format(str(file_information.get_type())))
    return loader


def load_isis(data_type, file_information, period, use_cached, calibration_file_name, parent_alg):
    """
    Loads workspaces according a SANSFileInformation object for ISIS.

    This function will select the correct loading strategy based on the information provided in the file_information.
    :param data_type: the data type, i.e. sample scatter, sample transmission, etc.
    :param file_information: a SANSFileInformation object.
    :param period: the selected period.
    :param use_cached: use cached workspaces on the ADS.
    :param calibration_file_name: the calibration file name. Note that this is only used for cached loading of data
                                  workspaces and not for loading of calibration files. We just want to make sure that
                                  the potentially cached data has had the correct calibration file applied to it.
    :param parent_alg: a handle to the parent algorithm.
    :return: a SANSDataType-Workspace map for data workspaces and a SANSDataType-Workspace map for monitor workspaces
    """
    workspace = []
    workspace_monitor = []

    is_transmission = is_transmission_type(data_type)

    # Make potentially use of loaded workspaces. For now we can only identify them by their name
    if use_cached:
        workspace, workspace_monitor = use_cached_workspaces_from_ads(file_information, is_transmission, period, calibration_file_name)

    # Load the workspace if required. We need to load it if there is no workspace loaded from the cache or, in the case
    # of scatter, ie. non-trans, there is no monitor workspace. There are several ways to load the data
    if len(workspace) == 0 or (len(workspace_monitor) == 0 and not is_transmission):
        loader = get_loader_strategy(file_information)
        workspace, workspace_monitor = loader(file_information, is_transmission, period, parent_alg=parent_alg)

    # Associate the data type with the workspace
    workspace_pack = {data_type: workspace}
    workspace_monitor_pack = {data_type: workspace_monitor} if len(workspace_monitor) > 0 else None

    return workspace_pack, workspace_monitor_pack


# ----------------------------------------------------------------------------------------------------------------------
# Load classes
# ----------------------------------------------------------------------------------------------------------------------
class SANSLoadData(metaclass=ABCMeta):
    """Base class for all SANSLoad implementations."""

    @abstractmethod
    def do_execute(self, data_info, use_cached, publish_to_ads, progress, parent_alg, adjustment_info):
        pass

    def execute(self, data_info, use_cached, publish_to_ads, progress, parent_alg, adjustment_info):
        SANSLoadData._validate(data_info)
        return self.do_execute(data_info, use_cached, publish_to_ads, progress, parent_alg, adjustment_info)

    @staticmethod
    def _validate(data_info):
        if not isinstance(data_info, StateData):
            raise ValueError(
                "SANSLoad: The provided state information is of the wrong type. It must be of type StateData,but was {0}".format(
                    str(type(data_info))
                )
            )
        data_info.validate()


class SANSLoadDataISIS(SANSLoadData):
    """Load implementation of SANSLoad for ISIS data"""

    def do_execute(self, data_info, use_cached, publish_to_ads, progress, parent_alg, adjustment_info):
        # Get all entries from the state file
        file_infos, period_infos = get_file_and_period_information_from_data(data_info)

        # Several important remarks regarding the loading
        # 1. Scatter files are loaded as with monitors and the data in two separate workspaces.
        # 2. Transmission files are loaded entirely. If they are event mode then only the monitor is loaded as the data.
        # 3. Added data is handled differently because it is already processed data.

        workspaces = {}
        workspace_monitors = {}

        calibration_file = adjustment_info.calibration if adjustment_info else None

        for key, value in list(file_infos.items()):
            # Loading
            report_message = "Loading {0}".format(key.value)
            progress.report(report_message)

            workspace_pack, workspace_monitors_pack = load_isis(key, value, period_infos[key], use_cached, calibration_file, parent_alg)

            # Add them to the already loaded workspaces
            workspaces.update(workspace_pack)
            if workspace_monitors_pack is not None:
                workspace_monitors.update(workspace_monitors_pack)

        # Apply the calibration if any exists.
        if calibration_file:
            report_message = "Applying calibration."
            progress.report(report_message)
            apply_calibration(calibration_file, workspaces, workspace_monitors, use_cached, publish_to_ads, parent_alg)

        # Apply corrections for transmission workspaces
        transmission_correction = get_transmission_correction(data_info)
        transmission_correction.correct(workspaces, parent_alg)

        return workspaces, workspace_monitors


class SANSLoadDataFactory(object):
    """A factory for SANSLoadData."""

    def __init__(self):
        super(SANSLoadDataFactory, self).__init__()

    @staticmethod
    def _get_facility(state):
        data = state.data
        # Get the correct loader based on the sample scatter file from the data sub state
        data.validate()
        file_info, _ = get_file_and_period_information_from_data(data)
        sample_scatter_info = file_info[SANSDataType.SAMPLE_SCATTER]
        return sample_scatter_info.get_facility()

    @staticmethod
    def create_loader(state):
        """
        Provides the appropriate loader.

        :param state: a SANSState object
        :return: the corresponding loader
        """
        facility = SANSLoadDataFactory._get_facility(state)
        if facility is SANSFacility.ISIS:
            loader = SANSLoadDataISIS()
        else:
            raise RuntimeError("SANSLoaderFactory: Other instruments are not implemented yet.")
        return loader


# -------------------------------------------------
#  Corrections for a loaded transmission workspace
# -------------------------------------------------


class TransmissionCorrection(metaclass=ABCMeta):
    @abstractmethod
    def correct(self, workspaces, parent_alg):
        pass


class NullTransmissionCorrection(TransmissionCorrection):
    def correct(self, workspaces, parent_alg):
        pass


class LOQTransmissionCorrection(TransmissionCorrection):
    def correct(self, workspaces, parent_alg):
        """
        For LOQ we want to apply a different instrument definition for the transmission runs.

        :param workspaces: a dictionary of data types, e.g. SampleScatter vs. a workspace
        :param parent_alg: a handle to the parent algorithm
        """
        # Get the transmission and the direct workspaces and apply the correction to them
        workspace_which_require_transmission_correction = []
        for data_type, _ in list(workspaces.items()):
            if is_transmission_type(data_type):
                workspace_which_require_transmission_correction.append(workspaces[data_type])

        # We want to apply a different instrument for the transmission runs

        for workspace in workspace_which_require_transmission_correction:
            assert len(workspace) == 1
            workspace = workspace[0]
            instrument = workspace.getInstrument()
            has_m4 = instrument.getComponentByName("monitor4")
            if has_m4 is None:
                trans_definition_file = os.path.join(config.getString("instrumentDefinition.directory"), "LOQ_trans_Definition.xml")
            else:
                trans_definition_file = os.path.join(config.getString("instrumentDefinition.directory"), "LOQ_trans_Definition_M4.xml")
            # Done
            instrument_name = "LoadInstrument"
            instrument_options = {"Workspace": workspace, "Filename": trans_definition_file, "RewriteSpectraMap": False}
            instrument_alg = create_child_algorithm(parent_alg, instrument_name, **instrument_options)
            instrument_alg.execute()


def get_transmission_correction(data_info):
    instrument_type = data_info.instrument

    if instrument_type is SANSInstrument.LOQ:
        return LOQTransmissionCorrection()
    else:
        return NullTransmissionCorrection()
