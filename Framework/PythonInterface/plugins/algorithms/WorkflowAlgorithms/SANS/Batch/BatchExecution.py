from copy import deepcopy
from collections import namedtuple

from SANS2.Common.SANSFunctions import create_unmanaged_algorithm
from SANS2.Common.SANSEnumerations import SANSDataType

batch_reduction_return_bundle = namedtuple('batch_reduction_return_bundle', 'state, lab, hab, merged')


def single_reduction_for_batch(state, use_optimizations, save_to_file=False):
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
    single_reduction_options = {}
    reduction_alg = create_unmanaged_algorithm(single_reduction_name, **single_reduction_options)

    batch_reduction_return_bundles = []
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
        if save_to_file:
            save_workspaces_to_file(reduced_lab, reduced_hab, reduced_merged)
        else:
            batch_reduction_return_bundles.append(batch_reduction_return_bundle(state=reduction_package.state,
                                                                                lab=reduced_lab,
                                                                                hab=reduced_hab,
                                                                                merged=reduced_merged))
    return batch_reduction_return_bundles


def provide_loaded_data(state, use_optimizations, workspace_to_name, workspace_to_monitor):
    # Load the data
    state_serialized = state.property_manager
    load_name = "SANSLoad"
    load_options = {"SANSState": state_serialized,
                    "PublishToCache": use_optimizations,
                    "UseCached": use_optimizations,
                    "MoveWorkspace": False}
    load_alg = create_unmanaged_algorithm(load_name, **load_options)
    load_alg.execute()

    # Retrieve the data
    workspace_to_count = {SANSDataType.SampleScatter: "NumberOfSampleScatterWorkspaces",
                          SANSDataType.SampleTransmission: "NumberOfSampleTransmissionWorkspaces",
                          SANSDataType.SampleDirect: "NumberOfSampleDirectWorkspaces",
                          SANSDataType.CanScatter: "NumberOfCanScatterWorkspaces",
                          SANSDataType.CanTransmission: "NumberOfCanTransmissionWorkspaces",
                          SANSDataType.CanDirect: "NumberOfCanDirectWorkspaces"}

    workspaces = get_workspaces_from_load_algorithm(load_alg, workspace_to_count, workspace_to_name)
    monitors = get_workspaces_from_load_algorithm(load_alg, workspace_to_count, workspace_to_monitor)

    return workspaces, monitors


def get_workspaces_from_load_algorithm(load_alg, workspace_to_count, workspace_name_dict):
    workspace_output = {}
    for workspace_type, workspace_name in workspace_name_dict.items():
        count_id = workspace_to_count[workspace_type]
        number_of_workspaces = load_alg.getProperty(count_id).value
        workspaces = []
        if number_of_workspaces > 1:
            workspaces = get_multi_period_workspaces(load_alg, workspace_name_dict[workspace_type],
                                                     number_of_workspaces)
        else:
            workspace_id = workspace_name_dict[workspace_type]
            workspace = load_alg.getProperty(workspace_id).value
            if workspace is not None:
                workspaces.append(workspace)
        # Add the workspaces to the to the output
        workspace_output.update({workspace_type: workspaces})
    return workspace_output


def get_multi_period_workspaces(load_alg, workspace_name, number_of_workspaces):
    # Create an output name for each workspace and retrieve it from the load algorithm
    workspaces = []
    for index in range(1, number_of_workspaces + 1):
        output_name = workspace_name + "_" + str(index)
        workspaces.append(load_alg.getProperty(output_name).value)
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
    # Go through the elements of the reduction package and set them on the reduction algorithm
    # Set the SANSState
    state = reduction_package.state
    state_dict = state.property_manager
    reduction_alg.setProperty("SANSState", state_dict)

    # Set the workspaces
    workspaces = reduction_package.workspaces
    for workspace_type, workspace in workspaces.items():
        if workspace is not None:
            reduction_alg.setProperty(workspace_to_name[workspace_type], workspace)

    # Set the monitors
    monitors = reduction_package.monitors
    for workspace_type, monitor in monitors.items():
        if monitor is not None:
            reduction_alg.setProperty(workspace_to_monitor[workspace_type], monitor)


def save_workspaces_to_file(reduced_lab, reduced_hab, reduced_merge):
    # TODO implement saving
    if reduced_lab is not None:
        pass

    if reduced_hab is not None:
        pass

    if reduced_merge is not None:
        pass
