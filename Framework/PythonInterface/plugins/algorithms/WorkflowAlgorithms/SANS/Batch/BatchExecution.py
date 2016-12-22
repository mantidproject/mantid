from copy import deepcopy
from collections import namedtuple
from mantid.api import AnalysisDataService

from SANS2.Common.SANSFunctions import create_unmanaged_algorithm
from SANS2.Common.SANSEnumerations import (SANSDataType, SaveType)
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.State.SANSStateFunctions import get_output_workspace_name_from_workspace
from SANS2.Common.SANSFileInformation import (get_extension_for_file_type, SANSFileInformationFactory)
from SANS2.State.SANSStateData import SANSStateData


class OutputMode(object):
    class SaveToFile(object):
        pass

    class PublishToADS(object):
        pass

batch_reduction_return_bundle = namedtuple('batch_reduction_return_bundle', 'state, lab, hab, merged')


def single_reduction_for_batch(state, use_optimizations, output_modes):
    """
    Runs a single reduction.

    This function creates reduction packages which essentially contain information for a single valid reduction, run it
    and store the results according to the user specified setting (output_modes). Although this is considered a single
    reduction it can contain still several reductions since the SANSState object can at this point contain slice
    settings which require on reduction per time slice.
    :param state: a SANSState object
    :param use_optimizations: if true then the optimizations of child algorithms are enabled.
    :param output_modes: a list of output options, i.e. saving to file and/or publishing to ADS
    """
    # Load the data
    workspace_to_name = {SANSDataType.SampleScatter: "SampleScatterWorkspace",
                         SANSDataType.SampleTransmission: "SampleTransmissionWorkspace",
                         SANSDataType.SampleDirect: "SampleDirectWorkspace",
                         SANSDataType.CanScatter: "CanScatterWorkspace",
                         SANSDataType.CanTransmission: "CanTransmissionWorkspace",
                         SANSDataType.CanDirect: "CanDirectWorkspace"}

    workspace_to_monitor = {SANSDataType.SampleScatter: "SampleScatterMonitorWorkspace",
                            SANSDataType.CanScatter: "SampleScatterMonitorWorkspace"}
    workspaces, monitors = provide_loaded_data(state, use_optimizations, workspace_to_name, workspace_to_monitor)

    # Split into individual bundles which can be reduced individually
    reduction_packages = get_reduction_packages(state, workspaces, monitors)

    # Run single reduction
    single_reduction_name = "SANSSingleReduction"
    single_reduction_options = {"UseOptimizations": use_optimizations}
    reduction_alg = create_unmanaged_algorithm(single_reduction_name, **single_reduction_options)

    for reduction_package in reduction_packages:
        # Set the properties on the algorithm
        set_properties_for_reduction_algorithm(reduction_alg, reduction_package,
                                               workspace_to_name, workspace_to_monitor)
        # Run the reduction
        reduction_alg.execute()
        # Get the output of the algorithm
        reduced_lab = reduction_alg.getProperty("OutputWorkspaceLAB").value
        reduced_hab = reduction_alg.getProperty("OutputWorkspaceHAB").value
        reduced_merged = reduction_alg.getProperty("OutputWorkspaceMerged").value

        # Perform output
        if OutputMode.PublishToADS in output_modes:
            publish_to_ads(reduced_lab, reduced_hab, reduced_merged)
        if OutputMode.SaveToFile in output_modes:
            save_workspaces_to_file(reduced_lab, reduced_hab, reduced_merged, reduction_package.state)


def get_expected_workspace_names(file_information, is_transmission, period):
    """
    Creates the expected names for SANS workspaces.

    SANS scientists expect the load workspaces to have certain, typical names. For example, the file SANS2D00022024.nxs
    which is used as a transmission workspace translates into 22024_trans_nxs.
    :param file_information: a file information object
    :param is_transmission: if the file information is for a transmission or not
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


def set_output_workspace_on_load_algorithm_for_one_workspace_type(load_options, load_workspace_name, file_name, period,
                                                                  is_transmission, file_info_factory,
                                                                  load_monitor_name=None):
    file_info = file_info_factory.create_sans_file_information(file_name)
    workspace_names = get_expected_workspace_names(file_info, is_transmission=is_transmission, period=period)
    count = 0
    # Now we set the load options if we are dealing with multi-period data, then we need to
    for workspace_name in workspace_names:
        if count == 0:
            load_options.update({load_workspace_name: workspace_name})
            if load_monitor_name is not None:
                monitor_name = workspace_name + "_monitors"
                load_options.update({load_monitor_name: monitor_name})
        else:
            load_workspace_name_for_period = load_workspace_name + "_" + str(count)
            load_options.update({load_workspace_name_for_period: workspace_name})
            if load_monitor_name is not None:
                load_monitor_name_for_period = load_monitor_name + "_" + str(count)
                monitor_name = workspace_name + "_monitors"
                load_options.update({load_monitor_name_for_period: monitor_name})
        count += 1


def set_output_workspaces_on_load_algorithm(load_options, state):
    data = state.data
    file_information_factory = SANSFileInformationFactory()

    # SampleScatter and SampleScatterMonitor
    set_output_workspace_on_load_algorithm_for_one_workspace_type(load_options=load_options,
                                                                  load_workspace_name="SampleScatterWorkspace",
                                                                  file_name=data.sample_scatter,
                                                                  period=data.sample_scatter_period,
                                                                  is_transmission=False,
                                                                  file_info_factory=file_information_factory,
                                                                  load_monitor_name="SampleScatterMonitorWorkspace")

    # SampleTransmission
    sample_transmission = data.sample_transmission
    if sample_transmission:
        set_output_workspace_on_load_algorithm_for_one_workspace_type(load_options=load_options,
                                                                      load_workspace_name="SampleTransmissionWorkspace",
                                                                      file_name=sample_transmission,
                                                                      period=data.sample_transmission_period,
                                                                      is_transmission=True,
                                                                      file_info_factory=file_information_factory)
    # SampleDirect
    sample_direct = data.sample_direct
    if sample_direct:
        set_output_workspace_on_load_algorithm_for_one_workspace_type(load_options=load_options,
                                                                      load_workspace_name="SampleDirectWorkspace",
                                                                      file_name=sample_direct,
                                                                      period=data.sample_direct_period,
                                                                      is_transmission=True,
                                                                      file_info_factory=file_information_factory)

    # CanScatter + CanMonitor
    can_scatter = data.can_scatter
    if can_scatter:
        set_output_workspace_on_load_algorithm_for_one_workspace_type(load_options=load_options,
                                                                      load_workspace_name="CanScatterWorkspace",
                                                                      file_name=can_scatter,
                                                                      period=data.can_scatter_period,
                                                                      is_transmission=False,
                                                                      file_info_factory=file_information_factory,
                                                                      load_monitor_name="CanScatterMonitorWorkspace")

    # CanTransmission
    can_transmission = data.can_transmission
    if can_transmission:
        set_output_workspace_on_load_algorithm_for_one_workspace_type(load_options=load_options,
                                                                      load_workspace_name="CanTransmissionWorkspace",
                                                                      file_name=can_transmission,
                                                                      period=data.can_transmission_period,
                                                                      is_transmission=True,
                                                                      file_info_factory=file_information_factory)
    # CanDirect
    can_direct = data.can_direct
    if can_direct:
        set_output_workspace_on_load_algorithm_for_one_workspace_type(load_options=load_options,
                                                                      load_workspace_name="CanDirectWorkspace",
                                                                      file_name=can_direct,
                                                                      period=data.can_direct_period,
                                                                      is_transmission=True,
                                                                      file_info_factory=file_information_factory)


def provide_loaded_data(state, use_optimizations, workspace_to_name, workspace_to_monitor):
    """
    Provide the data for reduction.


    :param state: a SANSState object.
    :param use_optimizations: if optimizations are enabled, then the load mechanism will search for workspaces on the
                              ADS.
    :param workspace_to_name: a map of SANSDataType vs output-property name of SANSLoad for workspaces
    :param workspace_to_monitor: a map of SANSDataType vs output-property name of SANSLoad for monitor workspaces
    :return: a list fo workspaces and a list of monitor workspaces
    """
    # Load the data
    state_serialized = state.property_manager
    load_name = "SANSLoad"
    load_options = {"SANSState": state_serialized,
                    "PublishToCache": use_optimizations,
                    "UseCached": use_optimizations,
                    "MoveWorkspace": False}

    # Set the output workspaces
    set_output_workspaces_on_load_algorithm(load_options, state)

    load_alg = create_unmanaged_algorithm(load_name, **load_options)
    load_alg.execute()

    # Retrieve the data
    workspace_to_count = {SANSDataType.SampleScatter: "NumberOfSampleScatterWorkspaces",
                          SANSDataType.SampleTransmission: "NumberOfSampleTransmissionWorkspaces",
                          SANSDataType.SampleDirect: "NumberOfSampleDirectWorkspaces",
                          SANSDataType.CanScatter: "NumberOfCanScatterWorkspaces",
                          SANSDataType.CanTransmission: "NumberOfCanTransmissionWorkspaces",
                          SANSDataType.CanDirect: "NumberOfCanDirectWorkspaces"}

    workspaces = get_workspaces_from_load_algorithm(load_alg, workspace_to_count, workspace_to_name, use_optimizations)
    monitors = get_workspaces_from_load_algorithm(load_alg, workspace_to_count, workspace_to_monitor, use_optimizations)
    return workspaces, monitors


def add_loaded_workspace_to_ads(load_alg, workspace_property_name, workspace):
    """
    Adds a workspace with the name that was set on the output of the load algorithm to the ADS


    :param load_alg: a handle to the load algorithm
    :param workspace_property_name: the workspace property name
    :param workspace: the workspace
    """
    workspace_name = load_alg.getProperty(workspace_property_name).valueAsStr
    AnalysisDataService.addOrReplace(workspace_name, workspace)


def get_workspaces_from_load_algorithm(load_alg, workspace_to_count, workspace_name_dict, use_optimizations):
    """
    Reads the workspaces from SANSLoad

    :param load_alg: a handle to the load algorithm
    :param workspace_to_count: a map from SANSDataType to the outpyt-number property name of SANSLoad for workspaces
    :param workspace_name_dict: a map of SANSDataType vs output-property name of SANSLoad for (monitor) workspaces
    :param use_optimizations: if optimizations are selected, then we save out the loaded data
    :return: a map of SANSDataType vs list of workspaces (to handle multi-period data)
    """
    workspace_output = {}
    for workspace_type, workspace_name in workspace_name_dict.items():
        count_id = workspace_to_count[workspace_type]
        number_of_workspaces = load_alg.getProperty(count_id).value
        workspaces = []
        if number_of_workspaces > 1:
            workspaces = get_multi_period_workspaces(load_alg, workspace_name_dict[workspace_type],
                                                     number_of_workspaces, use_optimizations)
        else:
            workspace_id = workspace_name_dict[workspace_type]
            workspace = load_alg.getProperty(workspace_id).value
            if workspace is not None:
                if use_optimizations:
                    add_loaded_workspace_to_ads(load_alg, workspace_id, workspace)
                workspaces.append(workspace)
        # Add the workspaces to the to the output
        workspace_output.update({workspace_type: workspaces})
    return workspace_output


def get_multi_period_workspaces(load_alg, workspace_name, number_of_workspaces, use_optimizations):
    # Create an output name for each workspace and retrieve it from the load algorithm
    workspaces = []
    for index in range(1, number_of_workspaces + 1):
        output_name = workspace_name + "_" + str(index)
        workspace = load_alg.getProperty(output_name).value
        workspaces.append(workspace)
        if use_optimizations:
            add_loaded_workspace_to_ads(load_alg, output_name, workspace)
    return workspaces


class ReductionPackage(object):
    def __init__(self, state, workspaces, monitors):
        super(ReductionPackage, self).__init__()
        self.state = state
        self.workspaces = workspaces
        self.monitors = monitors


def get_reduction_packages(state, workspaces, monitors):
    """
    This function creates a set of reduction packages which contain the necessary state for a single reduction
    as well as the required workspaces.

    There are several reasons why a state can (and should) split up:
    1. Multi-period files were loaded. This means that we need to perform one reduction per (loaded) period
    2. Event slices were specified. This means that we need to perform one reduction per event slice.

    :param state: A single state which potentially needs to be split up into several states
    :param workspaces: The workspaces contributing to the reduction
    :param monitors: The monitors contributing to the reduction
    :return: A set of "Reduction packages" where each reduction package defines a single reduction.
    """
    # First: Split the state on a per-period basis
    reduction_packages = create_initial_reduction_packages(state, workspaces, monitors)

    # Second: Split resulting reduction packages on a per-event-slice basis
    # Note that at this point all reduction packages will have the same state information. They only differ in the
    # workspaces that they use.
    if reduction_packages_require_splitting_for_event_slices(reduction_packages):
        reduction_packages = split_reduction_packages_for_event_packages(reduction_packages)

    # TODO: Third: Split resulting reduction packages on a per-wave-length-range basis
    return reduction_packages


def reduction_packages_require_splitting_for_event_slices(reduction_packages):
    """
    Creates reduction packages from a list of reduction packages by splitting up event slices.

    The SANSSingleReduction algorithm can handle only a single time slice. For each time slice, we require an individual
    reduction. Hence we split the states up at this point.
    :param reduction_packages: a list of reduction packages.
    :return: a list of reduction packages which has at leaset the same length as the input
    """
    # Determine if the event slice sub-state object contains multiple event slice requests. This is given
    # by the number of elements in start_tof
    reduction_package = reduction_packages[0]
    state = reduction_package.state
    slice_event_info = state.slice
    start_time = slice_event_info.start_time
    if start_time is not None and len(start_time) > 1:
        requires_split = True
    else:
        requires_split = False
    return requires_split


def split_reduction_packages_for_event_packages(reduction_packages):
    """
    Splits a reduction package object into several reduction package objects if it contains several event slice settings

    We want to split this up here since each event slice is a full reduction cycle in itself.
    :param reduction_packages: a list of reduction packages
    :return: a list of reduction packages where each reduction setting contains only one event slice.
    """
    # Since the state is the same for all reduction packages at this point we only need to create the split state once
    # for the first package and the apply to all the other packages. If we have 5 reduction packages and the user
    # requests 6 event slices, then we end up with 60 reductions!
    reduction_package = reduction_packages[0]
    state = reduction_package.state
    slice_event_info = state.slice
    start_time = slice_event_info.start_time
    end_time = slice_event_info.end_time

    states = []
    for start, end in zip(start_time, end_time):
        state_copy = deepcopy(state)
        slice_event_info = state_copy.slice_
        slice_event_info.start_time = [start]
        slice_event_info.end_time = [end]
        states.append(state_copy)

    # Now that we have all the states spread them across the packages
    reduction_packages_split = []
    for reduction_package in reduction_packages:
        workspaces = reduction_package.workspaces
        monitors = reduction_package.monitors
        for state in states:
            new_state = deepcopy(state)
            new_reduction_package = ReductionPackage(new_state, workspaces, monitors)
            reduction_packages_split.append(new_reduction_package)
    return reduction_packages_split


def create_initial_reduction_packages(state, workspaces, monitors):
    """
    This provides the initial split of the workspaces.

    If the data stems from multi-period data, then we need to split up the workspaces. The state object is valid
    for each one of these workspaces. Hence we need to create a deep copy of them for each reduction package.

    The way multiperiod files are handled over the different workspaces input types is:
    1. The sample scatter period determines all other periods, i.e. if the sample scatter workspace is has only
       one period, but the sample transmission has two, then only the first period is used.
    2. If the sample scatter period is not available on an other workspace type, then the last period on that
       workspace type is used.

    For the cases where the periods between the different workspaces types does not match, an information is logged.

    :param state: A single state which potentially needs to be split up into several states
    :param workspaces: The workspaces contributing to the reduction
    :param monitors: The monitors contributing to the reduction
    :return: A set of "Reduction packages" where each reduction package defines a single reduction.
    """
    # For loaded peri0d we create a package
    packages = []
    for index in range(0, len(workspaces[SANSDataType.SampleScatter])):
        workspaces_for_package = {}
        # For each workspace type, i.e sample scatter, can transmission, etc. find the correct workspace
        for workspace_type, workspace_list in workspaces.items():
            workspace = get_workspace_for_index(index, workspace_list)
            workspaces_for_package.update({workspace_type: workspace})

        # For each monitor type, find the correct workspace
        monitors_for_package = {}
        for workspace_type, workspace_list in monitors.items():
            workspace = get_workspace_for_index(index, workspace_list)
            monitors_for_package.update({workspace_type: workspace})
        state_copy = deepcopy(state)
        packages.append(ReductionPackage(state_copy, workspaces_for_package, monitors_for_package))
    return packages


def get_workspace_for_index(index, workspace_list):
    """
    Extracts the workspace from the list of workspaces. The index is set by the nth ScatterSample workspace.

    There might be situation where there is no corresponding CanXXX workspace or SampleTransmission workspace etc,
    since they are optional.

    :param index: The index of the workspace from which to extract.
    :param workspace_list: A list of workspaces.
    :return: The workspace corresponding to the index or None
    """
    if workspace_list:
        if index < len(workspace_list):
            workspace = workspace_list[index]
        else:
            workspace = None
    else:
        workspace = None
    return workspace


def set_properties_for_reduction_algorithm(reduction_alg, reduction_package, workspace_to_name, workspace_to_monitor):
    """
    Sets up everything necessary on the reduction algorithm.

    :param reduction_alg: a handle to the reduction algorithm
    :param reduction_package: a reduction package object
    :param workspace_to_name: the workspace to name map
    :param workspace_to_monitor: a workspace to monitor map
    """
    # Go through the elements of the reduction package and set them on the reduction algorithm
    # Set the SANSState
    state = reduction_package.state
    state_dict = state.property_manager
    reduction_alg.setProperty("SANSState", state_dict)

    # Set the input workspaces
    workspaces = reduction_package.workspaces
    for workspace_type, workspace in workspaces.items():
        if workspace is not None:
            reduction_alg.setProperty(workspace_to_name[workspace_type], workspace)

    # Set the monitors
    monitors = reduction_package.monitors
    for workspace_type, monitor in monitors.items():
        if monitor is not None:
            reduction_alg.setProperty(workspace_to_monitor[workspace_type], monitor)

    # Set the output workspaces
    reduction_alg.setProperty("OutputWorkspaceLAB", SANSConstants.dummy)
    reduction_alg.setProperty("OutputWorkspaceHAB", SANSConstants.dummy)
    reduction_alg.setProperty("OutputWorkspaceMerged", SANSConstants.dummy)


def save_workspaces_to_file(reduced_lab, reduced_hab, reduced_merged, state):
    """
    Saves the reduced workspaces to a file

    :param reduced_lab: a reduced workspace for LAB.
    :param reduced_hab: a reduced workspace for HAB.
    :param reduced_merged: a reduced worksapce for a merged operation.
    :param state: a SANSState object
    """
    if reduced_lab is not None:
        save_workspace_to_file(reduced_lab, state)

    if reduced_hab is not None:
        save_workspace_to_file(reduced_hab, state)

    if reduced_merged is not None:
        save_workspace_to_file(reduced_merged, state)


def publish_to_ads(reduced_lab, reduced_hab, reduced_merged):
    """
    Publish the reduced workspaces to the ADS

    :param reduced_lab: a reduced workspace for LAB.
    :param reduced_hab: a reduced workspace for HAB.
    :param reduced_merged: a reduced worksapce for a merged operation.
    """
    if reduced_lab is not None:
        do_publish_to_ads(reduced_lab)

    if reduced_hab is not None:
        do_publish_to_ads(reduced_hab)

    if reduced_merged is not None:
        do_publish_to_ads(reduced_merged)


def do_publish_to_ads(workspace):
    """
    Gets the name of the workspace and adds it to the ADS

    :param workspace: the workspace which is to be added to the ADS
    """
    workspace_name = get_output_workspace_name_from_workspace(workspace)
    AnalysisDataService.addOrReplace(workspace_name, workspace)


def save_workspace_to_file(workspace, state):
    """
    Saves the workspace to the different file formats specified in the state object.

    Note that the name of the file can be stored in the state object. If it is not supplied then the workspace
    name is used.
    :param workspace: the workspace to be stored.
    :param state: the SANSState object
    """
    save_info = state.save

    save_name = "SANSSave"
    save_options = {SANSConstants.input_workspace: workspace}
    if save_info.file_name:
        file_name = save_info.file_name
    else:
        file_name = get_output_workspace_name_from_workspace(workspace)
    save_options.update({SANSConstants.file_name: file_name})

    file_formats = save_info.file_format
    if SaveType.Nexus is file_formats:
        save_options.update({"Nexus": True})
    if SaveType.CanSAS is file_formats:
        save_options.update({"CanSAS": True})
    if SaveType.NXcanSAS is file_formats:
        save_options.update({"NXcanSAS": True})
    if SaveType.NistQxy is file_formats:
        save_options.update({"NistQxy": True})
    if SaveType.RKH is file_formats:
        save_options.update({"RKH": True})
    if SaveType.CSV is file_formats:
        save_options.update({"CSV": True})

    save_alg = create_unmanaged_algorithm(save_name, **save_options)
    save_alg.execute()
