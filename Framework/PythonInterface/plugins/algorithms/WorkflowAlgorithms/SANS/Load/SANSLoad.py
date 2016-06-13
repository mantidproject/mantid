from abc import (ABCMeta, abstractmethod)
from mantid.api import (AlgorithmManager, WorkspaceGroup, AnalysisDataService)
from SANSFileInformation import (SANSInstrument, SANSFileInformationFactory, SANSFileType, get_extension_for_file_type)
from State.SANSStateData import (SANSStateData, SANSDataType)


# ---------------------------------------------------------
# GLOBALS
# ---------------------------------------------------------
MONITOR_SUFFIX = "_monitors"


# -------------------
# Free functions
# -------------------
def create_unmanaged_algorithm(name, **kwargs):
    alg = AlgorithmManager.createUnmanaged(name)
    alg.initialize()
    alg.setChild(True)
    for key, value in kwargs.iteritems():
        alg.setProperty(key, value)
    return alg


def update_file_information(file_information_dict, factory, data_type, file_name):
    info = factory.create_sans_file_information(file_name)
    file_information_dict.update({data_type: info})


def get_file_and_period_information_from_data(data):
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
    if ((to_check is SANSDataType.SampleTransmission) or (to_check is SANSDataType.SampleDirect) or
       (to_check is SANSDataType.CanTransmission) or (to_check is SANSDataType.CanDirect)):
        is_transmission = True
    else:
        is_transmission = False
    return is_transmission


def get_expected_workspace_names(file_information, is_transmission, period):
    suffix_file_type = get_extension_for_file_type(file_information)
    if is_transmission:
        suffix_data = "trans"
    else:
        suffix_data = "sans"

    run_number = file_information.get_run_number()

    # Three possibilities:
    #  1. No period data => 22024_sans_nxs
    #  2. Period data, but wants all => 22025p1_sans_nxs,  22025p2_sans_nxs, ...
    #  3. Period data, select particular period => 22025p3_sans_nxs
    if file_information.get_number_of_periods() == 1:
        workspace_name = "{}_{}_{}".format(run_number, suffix_data, suffix_file_type)
        names = [workspace_name]
    elif file_information.get_number_of_periods() > 1 and period is SANSStateData.ALL_PERIODS:
        workspace_names = []
        for period in range(0, file_information.get_number_of_periods()):
            workspace_names.append("{}p{}_{}_{}".format(run_number, period, suffix_data, suffix_file_type))
        names = workspace_names
    elif file_information.get_number_of_periods() > 1 and period is not SANSStateData.ALL_PERIODS:
        workspace_name = "{}p{}_{}_{}".format(run_number, period, suffix_data, suffix_file_type)
        names = [workspace_name]
    else:
        raise RuntimeError("SANSLoad: Cannot create workspace names.")

    print "]]]]]]]]]]]]]]]]"
    print file_information.get_number_of_periods()

    return names


def get_loader_info_for_isis_nexus(file_information, period):
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
    loader_name = "LoadNexusMonitors"
    # TODO how to load period
    loader_options = {"Filename": file_information.get_file_name()}
    loader_options.update({"LoadEventMonitors": True,
                           "LoadHistoMonitors": True})
    return loader_name, loader_options


def get_loader_info_for_raw(file_information, period):
    loader_name = "LoadRaw"
    loader_options = {"Filename": file_information.get_file_name()}
    loader_options.update({"LoadMonitors": "Separate"})
    if period is not SANSStateData.ALL_PERIODS:
        loader_options.update({"PeriodList": period})
    return loader_name, loader_options


def get_loading_strategy(file_information, period, is_transmission):
    if is_transmission and file_information.get_type() == SANSFileType.ISISNexus:
        loader_name, loader_options = get_loader_info_for_isis_nexus_transmission(file_information, period)
    elif file_information.get_type() == SANSFileType.ISISNexus:
        loader_name, loader_options = get_loader_info_for_isis_nexus(file_information, period)
    elif file_information.get_type() == SANSFileType.ISISRaw:
        loader_name, loader_options = get_loader_info_for_raw(file_information, period)
    elif file_information.get_type() == SANSFileType.ISISNexusAdded:
        pass
        # TODO: Add loader for added
    else:
        raise RuntimeError("SANSLoad: Cannot load SANS file of type {}".format(str(file_information.get_type())))
    loader = create_unmanaged_algorithm(loader_name, **loader_options)
    return loader


def add_workspaces_to_analysis_data_service(workspaces, workspace_names, is_monitor):
    if is_monitor:
        workspace_names = [workspace_name + MONITOR_SUFFIX for workspace_name in workspace_names]

    # If workspace is a group workspace then we can iterate, else we cannot
    is_group_workspace = isinstance(workspaces, WorkspaceGroup)

    if is_group_workspace and len(workspaces) != len(workspace_names):
        raise RuntimeError("SANSLoad: There is a mismatch between the generated names and the length of"
                           " the WorkspaceGroup. The workspace has {} entries and there are {} "
                           "workspace names".format(len(workspaces), len(workspace_names)))

    # If we have a single workspace we want to make it iterable
    if not is_group_workspace:
        workspaces = [workspaces]

    for index in range(0, len(workspaces)):
        if not AnalysisDataService.doesExist(workspace_names[index]):
            AnalysisDataService.addOrReplace(workspace_names[index], workspaces[index])


def publish_workspaces_to_analysis_data_service(workspaces, workspace_monitors, workspace_names):
    add_workspaces_to_analysis_data_service(workspaces, workspace_names, is_monitor=False)

    # If the workspace monitor exists, then add it to the ADS as well
    if workspace_monitors is not None:
        add_workspaces_to_analysis_data_service(workspace_monitors, workspace_names, is_monitor=True)


def un_group_workspaces(workspace):
    pass


def run_loader(loader, is_transmission):
    loader.setProperty("OutputWorkspace", "dummy")
    loader.execute()
    if is_transmission:
        workspace = loader.getProperty("OutputWorkspace").value
        workspace_monitor = None
    else:
        workspace = loader.getProperty("OutputWorkspace").value
        workspace_monitor = loader.getProperty("MonitorWorkspace").value
        assert (workspace_monitor is not None)
    assert (workspace is not None)
    return workspace, workspace_monitor


def load_isis(data_type, file_information, period, use_loaded, publish_to_ads):
    workspace = None
    workspace_monitor = None

    is_transmission = is_transmission_type(data_type)

    # Get the workspace name
    workspace_names = get_expected_workspace_names(file_information, is_transmission, period)

    # Make potentially use of loaded workspaces. For now we can only identify them by their name
    # TODO: Add tag into sample logs
    if use_loaded:
        pass #workspace, workspace_monitors = use_loaded_workspaces_from_analysis_data_service(workspace_names)

    # Load the workspace if required.
    if workspace is None or workspace_monitor is None:
        # Get the loading strategy
        loader = get_loading_strategy(file_information, period, is_transmission)
        workspace, workspace_monitor = run_loader(loader, is_transmission)

    # If we are dealing with a workspace group then un-group it here
    #workspace = ungroup_workspaces(workspace)
    #workspace_monitor = ungroup_workspaces(workspace_monitor)

    # Publish to ADS if required
    if publish_to_ads:
        publish_workspaces_to_analysis_data_service(workspace, workspace_monitor, workspace_names)

    # Associate the data type with the workspace
    workspace_pack = {data_type: workspace}
    workspace_monitor_pack = {data_type: workspace_monitor} if workspace_monitor is not None else None

    return workspace_pack, workspace_monitor_pack

# --------------------------------------------
# Put functions for other implementations here:
# --------------------------------------------


# -------------------------------------------------
# Load classes
# -------------------------------------------------
class SANSLoad(object):
    __metaclass__ = ABCMeta

    @abstractmethod
    def do_execute(self, data_info, use_loaded, publish_to_ads):
        pass

    def execute(self, data_info, use_loaded, publish_to_ads):
        SANSLoad._validate(data_info)
        return self.do_execute(data_info, use_loaded, publish_to_ads)

    @staticmethod
    def _validate(data_info):
        if not isinstance(data_info, SANSStateData):
            raise ValueError("SANSLoad: The provided state information is of the wrong type. It must be"
                             " of type SANSStateData,but was {}".format(str(type(data_info))))
        data_info.validate()


class SANSLoadISIS(SANSLoad):
    def do_execute(self, data_info, use_loaded, publish_to_ads):
        # Get all entries from the state file
        file_info, period_info = get_file_and_period_information_from_data(data_info)

        # Scatter files and Transmission/Direct files have to be loaded slightly differently,
        # hence we separate the loading process.
        # TODO: make parallel with multiprocessing (check how ws_handle is being passed)
        workspaces = dict()
        workspace_monitors = dict()
        for key, value in file_info.iteritems():
            workspace_pack, workspace_monitors_pack = load_isis(key, value, period_info[key],
                                                                use_loaded, publish_to_ads)
            workspaces.update(workspace_pack)
            if workspace_monitors_pack is not None:
                workspace_monitors.update(workspace_monitors_pack)
        return workspaces, workspace_monitors


class SANSLoadFactory(object):
    def __init__(self):
        super(SANSLoadFactory, self).__init__()

    @staticmethod
    def _get_instrument_type(data):
        # Get the correct loader based on the sample scatter file
        data.validate()
        file_info, _ = get_file_and_period_information_from_data(data)
        sample_scatter_info = file_info[SANSDataType.SampleScatter]
        return sample_scatter_info.get_instrument()

    @staticmethod
    def create_loader(data):
        instrument_type = SANSLoadFactory._get_instrument_type(data)
        if instrument_type is SANSInstrument.LARMOR or instrument_type is SANSInstrument.LOQ or\
           instrument_type is SANSInstrument.SANS2D:
            loader = SANSLoadISIS()
        else:
            loader = None
            NotImplementedError("SANSLoaderFactory: Other instruments are not implemented yet.")
        return loader
