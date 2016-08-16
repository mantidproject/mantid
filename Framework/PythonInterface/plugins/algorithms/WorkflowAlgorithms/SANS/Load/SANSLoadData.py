# pylint: disable=too-few-public-methods, invalid-name, fixme, unused-argument
# pylint: disable=R0922
""" Implementation for the SANSLoad algorithm"""

from abc import (ABCMeta, abstractmethod)
from mantid.api import (AnalysisDataService)
from SANS2.Common.SANSFileInformation import (SANSFileInformationFactory, SANSFileType, get_extension_for_file_type, find_full_file_path)
from SANS2.State.SANSStateData import (SANSStateData)
from SANS2.Common.SANSConstants import (SANSConstants)
from SANS2.Common.SANSEnumerations import (SANSInstrument, SANSDataType, convert_from_data_type_to_string)
from SANS2.Common.SANSFunctions import (create_unmanaged_algorithm)
from SANS2.Common.SANSLogTagger import (set_tag, has_tag, get_tag)
from SANS.Load.SANSCalibration import apply_calibration


# -------------------
# Free functions
# -------------------
def update_file_information(file_information_dict, factory, data_type, file_name):
    info = factory.create_sans_file_information(file_name)
    file_information_dict.update({data_type: info})


def get_file_and_period_information_from_data(data):
    """
    Get the file information and period information from a SANSStateData object

    :param data: a SANSStateData object
    :return: a map of SANSFileInformation objects and map of period information
    """
    file_information_factory = SANSFileInformationFactory()
    file_information = dict()
    period_information = dict()
    if data.sample_scatter is not None:
        update_file_information(file_information, file_information_factory,
                                SANSDataType.SampleScatter, data.sample_scatter)
        period_information.update({SANSDataType.SampleScatter: data.sample_scatter_period})
    if data.sample_transmission is not None:
        update_file_information(file_information, file_information_factory,
                                SANSDataType.SampleTransmission, data.sample_transmission)
        period_information.update({SANSDataType.SampleTransmission: data.sample_transmission_period})
    if data.sample_direct is not None:
        update_file_information(file_information, file_information_factory,
                                SANSDataType.SampleDirect, data.sample_direct)
        period_information.update({SANSDataType.SampleDirect: data.sample_direct_period})
    if data.can_scatter is not None:
        update_file_information(file_information, file_information_factory,
                                SANSDataType.CanScatter, data.can_scatter)
        period_information.update({SANSDataType.CanScatter: data.can_scatter_period})
    if data.can_transmission is not None:
        update_file_information(file_information, file_information_factory,
                                SANSDataType.CanTransmission, data.can_transmission)
        period_information.update({SANSDataType.CanTransmission: data.can_transmission})
    if data.can_direct is not None:
        update_file_information(file_information, file_information_factory,
                                SANSDataType.CanDirect, data.can_direct)
        period_information.update({SANSDataType.CanDirect: data.can_direct})
    return file_information, period_information


def is_transmission_type(to_check):
    """
    Checks if a SANSDataType object is of transmission type.

    :param to_check: A SANSDataType object.
    :return: true if the SANSDataType object is a transmission object (transmission or direct) else false.
    """
    if ((to_check is SANSDataType.SampleTransmission) or (to_check is SANSDataType.SampleDirect) or
            (to_check is SANSDataType.CanTransmission) or (to_check is SANSDataType.CanDirect)):
        is_transmission = True
    else:
        is_transmission = False
    return is_transmission


def get_expected_file_tags(file_information, is_transmission, period):
    """
    Creates the expected file tags for SANS workspaces.

    :param file_information: a file information object
    :param is_transmission: if the file inforamation is for a transmission or not
    :param period: the period of interest
    :return: a list of file tags
    """
    suffix_file_type = get_extension_for_file_type(file_information)
    if is_transmission:
        suffix_data = SANSConstants.trans_suffix
    else:
        suffix_data = SANSConstants.sans_suffix
    file_path = file_information.get_file_name()

    # Three possibilities:
    #  1. No period data => 22024_sans_nxs
    #  2. Period data, but wants all => 22025p1_sans_nxs,  22025p2_sans_nxs, ...
    #  3. Period data, select particular period => 22025p3_sans_nxs
    if file_information.get_number_of_periods() == 1:
        file_tag_name = "{0}_{1}_{2}".format(file_path, suffix_data, suffix_file_type)
        names = [file_tag_name]
    elif file_information.get_number_of_periods() > 1 and period is SANSStateData.ALL_PERIODS:
        file_tag_names = []
        for period in range(1, file_information.get_number_of_periods() + 1):
            file_tag_names.append("{0}p{1}_{2}_{3}".format(file_path, period, suffix_data, suffix_file_type))
        names = file_tag_names
    elif file_information.get_number_of_periods() > 1 and period is not SANSStateData.ALL_PERIODS:
        file_tag_name = "{0}p{1}_{2}_{3}".format(file_path, period, suffix_data, suffix_file_type)
        names = [file_tag_name]
    else:
        raise RuntimeError("SANSLoad: Cannot create workspace names.")
    return names


def get_expected_workspace_names(file_information, is_transmission, period):
    """
    Creates the expected names for SANS workspaces.

    SANS scientists expect the load workspaces to have certain, typical names. For example, the file SANS2D00022024.nxs
    which is used as a transmission workspace translates into 22024_trans_nxs.
    :param file_information: a file information object
    :param is_transmission: if the file inforamation is for a transmission or not
    :param period: the period of interest
    :return: a list of workspace names
    """
    suffix_file_type = get_extension_for_file_type(file_information)
    if is_transmission:
        suffix_data = SANSConstants.trans_suffix
    else:
        suffix_data = SANSConstants.sans_suffix

    run_number = file_information.get_run_number()

    # Three possibilities:
    #  1. No period data => 22024_sans_nxs
    #  2. Period data, but wants all => 22025p1_sans_nxs,  22025p2_sans_nxs, ...
    #  3. Period data, select particular period => 22025p3_sans_nxs
    if file_information.get_number_of_periods() == 1:
        workspace_name = "{0}_{1}_{2}".format(run_number, suffix_data, suffix_file_type)
        names = [workspace_name]
    elif file_information.get_number_of_periods() > 1 and period is SANSStateData.ALL_PERIODS:
        workspace_names = []
        for period in range(1, file_information.get_number_of_periods() + 1):
            workspace_names.append("{0}p{1}_{2}_{3}".format(run_number, period, suffix_data, suffix_file_type))
        names = workspace_names
    elif file_information.get_number_of_periods() > 1 and period is not SANSStateData.ALL_PERIODS:
        workspace_name = "{0}p{1}_{2}_{3}".format(run_number, period, suffix_data, suffix_file_type)
        names = [workspace_name]
    else:
        raise RuntimeError("SANSLoad: Cannot create workspace names.")
    return names


def get_loader_info_for_isis_nexus(file_information, period):
    """
    Get name and the options for the loading algorithm.

    This takes a SANSFileInformation object and provides the inputs for the adequate loading strategy.
    :param file_information: a SANSFileInformation object.
    :param period: the period to load.
    :return: the name of the load algorithm and the selected load options.
    """
    loader_options = {"Filename": file_information.get_file_name()}
    if file_information.is_event_mode():
        loader_name = "LoadEventNexus"
        # TODO: Multiperiod event files
        loader_options.update({"LoadMonitors": True})
    else:
        loader_name = "LoadISISNexus"
        loader_options.update({"LoadMonitors": "Separate"})
        if period is not SANSStateData.ALL_PERIODS:
            loader_options.update({"EntryNumber": period})
    return loader_name, loader_options


def get_loader_info_for_isis_nexus_transmission(file_information, period):
    """
    Get the load algorithm information for an nexus transmission file for ISIS

    :param file_information: a SANSFileInformation object.
    :param period: the period to load.
    :return: the name of the load algorithm and the selected load options.
    """
    loader_name = "LoadNexusMonitors"
    # TODO how to load a single period?
    loader_options = {"Filename": file_information.get_file_name()}
    loader_options.update({"LoadEventMonitors": True,
                           "LoadHistoMonitors": True})
    return loader_name, loader_options


def get_loader_info_for_raw(file_information, period):
    """
    Get the load algorithm information for an raw file

    :param file_information: a SANSFileInformation object.
    :param period: the period to load.
    :return: the name of the load algorithm and the selected load options.
    """
    loader_name = "LoadRaw"
    loader_options = {"Filename": file_information.get_file_name()}
    loader_options.update({"LoadMonitors": "Separate"})
    if period is not SANSStateData.ALL_PERIODS:
        loader_options.update({"PeriodList": period})
    return loader_name, loader_options


def get_loading_strategy(file_information, period, is_transmission):
    """
    Selects a loading strategy depending on the file type and if we are dealing with a transmission

    :param file_information: a SANSFileInformation object.
    :param period: the period to load.
    :param is_transmission: if the file to load is a transmission type, i.e. for transmission or direct.
    :return: the name of the load algorithm and the selected load options.
    """
    if is_transmission and file_information.get_type() == SANSFileType.ISISNexus:
        loader_name, loader_options = get_loader_info_for_isis_nexus_transmission(file_information, period)
    elif file_information.get_type() == SANSFileType.ISISNexus:
        loader_name, loader_options = get_loader_info_for_isis_nexus(file_information, period)
    elif file_information.get_type() == SANSFileType.ISISRaw:
        loader_name, loader_options = get_loader_info_for_raw(file_information, period)
    elif file_information.get_type() == SANSFileType.ISISNexusAdded:
        loader_name = None
        loader_options = {}
        raise NotImplementedError("SANSLoad: Cannot load SANS file of type {0}".format(str(file_information.get_type())))
        # TODO: Add loader for added
    else:
        raise RuntimeError("SANSLoad: Cannot load SANS file of type {0}".format(str(file_information.get_type())))
    loader = create_unmanaged_algorithm(loader_name, **loader_options)
    return loader


def add_workspaces_to_analysis_data_service(workspaces, workspace_names, is_monitor):
    """
    Adds a list of workspaces to the ADS.

    :param workspaces: list of workspaces
    :param workspace_names: the names under which they are to be published
    :param is_monitor: if the workspace is a monitor or not
    """
    if is_monitor:
        workspace_names = [workspace_name + SANSConstants.monitor_suffix for workspace_name in workspace_names]
    if len(workspaces) != len(workspace_names):
        raise RuntimeError("SANSLoad: There is a mismatch between the generated names and the length of"
                           " the WorkspaceGroup. The workspace has {0} entries and there are {1} "
                           "workspace names".format(len(workspaces), len(workspace_names)))

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
    elif number_of_periods > 1 and period is not SANSStateData.ALL_PERIODS:
        is_valid = True if number_of_workspaces == 1 else False
    elif number_of_periods > 1 and period is SANSStateData.ALL_PERIODS:
        is_valid = True if number_of_workspaces == number_of_periods else False
    else:
        raise RuntimeError("SANSLoad: Loading data from the ADS has resulted in the a mismatch between the number of "
                           "period information and the number of loaded workspaces")
    return is_valid


def is_calibration_correct(workspace, calibration_file):
    is_correct = True
    if has_tag(SANSConstants.Calibration.calibration_workspace_tag, workspace):
        is_correct = calibration_file == get_tag(SANSConstants.Calibration.calibration_workspace_tag, workspace)
    return is_correct


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
            if has_tag(SANSConstants.sans_file_tag, workspace):
                file_tag = get_tag(SANSConstants.sans_file_tag, workspace)
                if file_tag in file_tags and is_calibration_correct(workspace, full_calibration_file_path):
                    workspaces.append(workspace)
        except RuntimeError:
            continue


def use_cached_workspaces_from_ads(file_information,  is_transmission,  period, calibration_file_name):
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

    full_calibration_file_path = find_full_file_path(calibration_file_name)

    # Get the expected sans_original_workspace tag entries
    file_tags = get_expected_file_tags(file_information, is_transmission, period)
    get_workspaces_from_ads_if_exist(file_tags, full_calibration_file_path, workspaces)

    if not is_transmission:
        file_tags_monitors = [file_tag + SANSConstants.monitor_suffix for file_tag in file_tags]
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


def tag_workspaces_with_file_names(workspaces, file_information, is_transmission, period,  is_monitor):
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
        raise RuntimeError("Issue while tagging the loaded data. The number of tags does not match the number "
                           "of workspaces.")

    for file_tag, workspace in zip(file_tags, workspaces):
        if not has_tag(SANSConstants.sans_file_tag, workspace):
            if is_monitor:
                file_tag += SANSConstants.monitor_suffix
            set_tag(SANSConstants.sans_file_tag, file_tag, workspace)


def run_loader(loader, file_information, is_transmission, period):
    """
    Runs the load algorithm.

    :param loader: a handle to the selected load algorithm/strategy
    :param file_information: a SANSFileInformation object
    :param is_transmission: if the workspace is a transmission workspace.
    :param period: the selected period.
    :return: a list of workspaces and a list of monitor workspaces
    """
    loader.setProperty(SANSConstants.output_workspace, SANSConstants.dummy)
    loader.execute()

    # Get all output workspaces
    number_of_periods = file_information.get_number_of_periods()
    workspaces = []
    # Either we have a single-period workspace or we want a single period from a multi-period workspace in which case
    # we extract it via OutputWorkspace or we want all child workspaces of a multi-period workspace in which case we
    # need to extract it via OutputWorkspace_1, OutputWorkspace_2, ...
    if number_of_periods == 1 or (number_of_periods > 1 and period is not SANSStateData.ALL_PERIODS):
        workspaces.append(loader.getProperty(SANSConstants.output_workspace).value)
    else:
        for index in range(1, number_of_periods + 1):
            workspaces.append(loader.getProperty(SANSConstants.output_workspace_group + str(index)).value)

    workspace_monitors = []
    if not is_transmission:
        if number_of_periods == 1 or (number_of_periods > 1 and period is not SANSStateData.ALL_PERIODS):
            workspace_monitors.append(loader.getProperty(SANSConstants.output_monitor_workspace).value)
        else:
            for index in range(1, number_of_periods + 1):
                workspace_monitors.append(loader.getProperty(SANSConstants.output_monitor_workspace_group +
                                                             str(index)).value)
    if workspaces:
        tag_workspaces_with_file_names(workspaces, file_information, is_transmission, period, is_monitor=False)
    if workspace_monitors:
        tag_workspaces_with_file_names(workspace_monitors, file_information, is_transmission, period, is_monitor=True)
    return workspaces, workspace_monitors


def load_isis(data_type, file_information, period, use_cached, calibration_file_name):
    """
    Loads workspaces according a SANSFileInformation object for ISIS.

    :param data_type: the data type, i.e. sample scatter, sample transmission, etc.
    :param file_information: a SANSFileInformation object.
    :param period: the selected period.
    :param use_cached: use cached workspaces on the ADS.
    :param calibration_file_name: the calibration file name. Note that this is only used for cached loading of data
                                  workspaces and not for loading of calibration files. We just want to make sure that
                                  the potentially cached data has had the correct calibration file applied to it.
    :return: a SANSDataType-Workspace map for data workspaces and a SANSDataType-Workspace map for monitor workspaces
    """
    workspace = []
    workspace_monitor = []

    is_transmission = is_transmission_type(data_type)
    # Make potentially use of loaded workspaces. For now we can only identify them by their name
    if use_cached:
        workspace, workspace_monitor = use_cached_workspaces_from_ads(file_information, is_transmission, period,
                                                                      calibration_file_name)

    # Load the workspace if required. We need to load it if there is no workspace loaded from the cache or, in the case
    # of scatter, ie. non-trans, there is no monitor workspace
    if len(workspace) == 0 or (len(workspace_monitor) == 0 and not is_transmission):
        # Get the loading strategy
        loader = get_loading_strategy(file_information, period, is_transmission)
        workspace, workspace_monitor = run_loader(loader, file_information, is_transmission, period)

    # Associate the data type with the workspace
    workspace_pack = {data_type: workspace}
    workspace_monitor_pack = {data_type: workspace_monitor} if len(workspace_monitor) > 0 else None

    return workspace_pack, workspace_monitor_pack


# --------------------------------------------
# Put functions for other implementations here:
# --------------------------------------------


# -------------------------------------------------
# Load classes
# -------------------------------------------------
class SANSLoadData(object):
    """ Base class for all SANSLoad implementations."""
    __metaclass__ = ABCMeta

    @abstractmethod
    def do_execute(self, data_info, use_cached, publish_to_ads):
        pass

    def execute(self, data_info, use_cached, publish_to_ads, progress):
        SANSLoadData._validate(data_info)
        return self.do_execute(data_info, use_cached, publish_to_ads, progress)

    @staticmethod
    def _validate(data_info):
        if not isinstance(data_info, SANSStateData):
            raise ValueError("SANSLoad: The provided state information is of the wrong type. It must be"
                             " of type SANSStateData,but was {0}".format(str(type(data_info))))
        data_info.validate()


class SANSLoadDataISIS(SANSLoadData):
    """Load implementation of SANSLoad for ISIS data"""
    def do_execute(self, data_info, use_cached, publish_to_ads, progress):
        # Get all entries from the state file
        file_info, period_info = get_file_and_period_information_from_data(data_info)
        # Scatter files and Transmission/Direct files have to be loaded slightly differently,
        # hence we separate the loading process.
        # TODO: make parallel with multiprocessing (check how ws_handle is being passed)
        workspaces = dict()
        workspace_monitors = dict()

        if data_info.calibration is not None:
            calibration_file = data_info.calibration
        else:
            calibration_file = ""

        for key, value in file_info.iteritems():
            # Loading
            report_message = "Loading {0}".format(convert_from_data_type_to_string(key))
            progress.report(report_message)

            workspace_pack, workspace_monitors_pack = load_isis(key, value, period_info[key],
                                                                use_cached, calibration_file)

            # Add them to the already loaded workspaces
            workspaces.update(workspace_pack)
            if workspace_monitors_pack is not None:
                workspace_monitors.update(workspace_monitors_pack)

        # Apply the calibration if any exists.
        if data_info.calibration is not None:
            report_message = "Applying calibration."
            progress.report(report_message)
            apply_calibration(calibration_file, workspaces, workspace_monitors, use_cached, publish_to_ads)

        return workspaces, workspace_monitors


class NullLoadData(SANSLoadData):
    def do_execute(self, data_info, use_cached, publish_to_ads):
        return {}, {}


class SANSLoadDataFactory(object):
    """ A factory for SANSLoadData."""
    def __init__(self):
        super(SANSLoadDataFactory, self).__init__()

    @staticmethod
    def _get_instrument_type(state):
        data = state.data
        # Get the correct loader based on the sample scatter file from the data sub state
        data.validate()
        file_info, _ = get_file_and_period_information_from_data(data)
        sample_scatter_info = file_info[SANSDataType.SampleScatter]
        return sample_scatter_info.get_instrument()

    @staticmethod
    def create_loader(state):
        """
        Provides the appropriate loader.

        :param state: a SANSState object
        :return: the corresponding loader
        """
        instrument_type = SANSLoadDataFactory._get_instrument_type(state)
        if instrument_type is SANSInstrument.LARMOR or instrument_type is SANSInstrument.LOQ or\
           instrument_type is SANSInstrument.SANS2D:
            loader = SANSLoadDataISIS()
        else:
            loader = NullLoadData()
            RuntimeError("SANSLoaderFactory: Other instruments are not implemented yet.")
        return loader
