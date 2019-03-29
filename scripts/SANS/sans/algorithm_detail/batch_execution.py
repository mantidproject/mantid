# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
from copy import deepcopy
from mantid.api import AnalysisDataService, WorkspaceGroup
from sans.common.general_functions import (create_managed_non_child_algorithm, create_unmanaged_algorithm,
                                           get_output_name, get_base_name_from_multi_period_name, get_transmission_output_name)
from sans.common.enums import (SANSDataType, SaveType, OutputMode, ISISReductionMode, DataType)
from sans.common.constants import (TRANS_SUFFIX, SANS_SUFFIX, ALL_PERIODS,
                                   LAB_CAN_SUFFIX, LAB_CAN_COUNT_SUFFIX, LAB_CAN_NORM_SUFFIX,
                                   HAB_CAN_SUFFIX, HAB_CAN_COUNT_SUFFIX, HAB_CAN_NORM_SUFFIX,
                                   LAB_SAMPLE_SUFFIX, HAB_SAMPLE_SUFFIX,
                                   REDUCED_HAB_AND_LAB_WORKSPACE_FOR_MERGED_REDUCTION,
                                   CAN_COUNT_AND_NORM_FOR_OPTIMIZATION,
                                   CAN_AND_SAMPLE_WORKSPACE)
from sans.common.file_information import (get_extension_for_file_type, SANSFileInformationFactory)
from sans.state.data import StateData

from qtpy import PYQT4
if PYQT4:
    try:
        from mantidplot import graph, plotSpectrum
        IN_MANTIDPLOT = True
    except ImportError:
        IN_MANTIDPLOT = False
else:
    from mantidqt.plotting.functions import plot


# ----------------------------------------------------------------------------------------------------------------------
# Functions for the execution of a single batch iteration
# ----------------------------------------------------------------------------------------------------------------------
def single_reduction_for_batch(state, use_optimizations, output_mode, plot_results, output_graph, save_can=False):
    """
    Runs a single reduction.

    This function creates reduction packages which essentially contain information for a single valid reduction, run it
    and store the results according to the user specified setting (output_mode). Although this is considered a single
    reduction it can contain still several reductions since the SANSState object can at this point contain slice
    settings which require on reduction per time slice.
    :param state: a SANSState object
    :param use_optimizations: if true then the optimizations of child algorithms are enabled.
    :param output_mode: the output mode
    :param save_can: bool. whether or not to save out can workspaces
    """
    # ------------------------------------------------------------------------------------------------------------------
    # Load the data
    # ------------------------------------------------------------------------------------------------------------------
    workspace_to_name = {SANSDataType.SampleScatter: "SampleScatterWorkspace",
                         SANSDataType.SampleTransmission: "SampleTransmissionWorkspace",
                         SANSDataType.SampleDirect: "SampleDirectWorkspace",
                         SANSDataType.CanScatter: "CanScatterWorkspace",
                         SANSDataType.CanTransmission: "CanTransmissionWorkspace",
                         SANSDataType.CanDirect: "CanDirectWorkspace"}

    workspace_to_monitor = {SANSDataType.SampleScatter: "SampleScatterMonitorWorkspace",
                            SANSDataType.CanScatter: "CanScatterMonitorWorkspace"}

    workspaces, monitors = provide_loaded_data(state, use_optimizations, workspace_to_name, workspace_to_monitor)

    # ------------------------------------------------------------------------------------------------------------------
    # Get reduction settings
    # Split into individual bundles which can be reduced individually. We split here if we have multiple periods or
    # sliced times for example.
    # ------------------------------------------------------------------------------------------------------------------
    reduction_packages = get_reduction_packages(state, workspaces, monitors)
    # ------------------------------------------------------------------------------------------------------------------
    # Run reductions (one at a time)
    # ------------------------------------------------------------------------------------------------------------------
    single_reduction_name = "SANSSingleReduction"
    single_reduction_options = {"UseOptimizations": use_optimizations,
                                "SaveCan": save_can}
    reduction_alg = create_managed_non_child_algorithm(single_reduction_name, **single_reduction_options)
    reduction_alg.setChild(False)
    # Perform the data reduction
    for reduction_package in reduction_packages:
        # -----------------------------------
        # Set the properties on the algorithm
        # -----------------------------------
        set_properties_for_reduction_algorithm(reduction_alg, reduction_package,
                                               workspace_to_name, workspace_to_monitor)

        # -----------------------------------
        #  Run the reduction
        # -----------------------------------
        reduction_alg.execute()

        # -----------------------------------
        # Get the output of the algorithm
        # -----------------------------------
        reduction_package.reduced_lab = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceLAB")
        reduction_package.reduced_hab = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceHAB")
        reduction_package.reduced_merged = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceMerged")

        reduction_package.reduced_lab_can = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceLABCan")
        reduction_package.reduced_lab_can_count = get_workspace_from_algorithm(reduction_alg,
                                                                               "OutputWorkspaceLABCanCount")
        reduction_package.reduced_lab_can_norm = get_workspace_from_algorithm(reduction_alg,
                                                                              "OutputWorkspaceLABCanNorm")
        reduction_package.reduced_hab_can = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceHABCan")
        reduction_package.reduced_hab_can_count = get_workspace_from_algorithm(reduction_alg,
                                                                               "OutputWorkspaceHABCanCount")
        reduction_package.reduced_hab_can_norm = get_workspace_from_algorithm(reduction_alg,
                                                                              "OutputWorkspaceHABCanNorm")
        reduction_package.calculated_transmission = get_workspace_from_algorithm(reduction_alg,
                                                                                 "OutputWorkspaceCalculatedTransmission")
        reduction_package.unfitted_transmission = get_workspace_from_algorithm(reduction_alg,
                                                                               "OutputWorkspaceUnfittedTransmission")
        reduction_package.calculated_transmission_can = get_workspace_from_algorithm(reduction_alg,
                                                                                     "OutputWorkspaceCalculatedTransmissionCan")
        reduction_package.unfitted_transmission_can = get_workspace_from_algorithm(reduction_alg,
                                                                                   "OutputWorkspaceUnfittedTransmissionCan")

        reduction_package.reduced_lab_sample = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceLABSample")
        reduction_package.reduced_hab_sample = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceHABSample")

        reduction_package.out_scale_factor = reduction_alg.getProperty("OutScaleFactor").value
        reduction_package.out_shift_factor = reduction_alg.getProperty("OutShiftFactor").value

        if plot_results:
            if PYQT4:
                plot_workspace(reduction_package, output_graph)
            elif output_graph:
                plot_workspace_matplotlib(reduction_package, output_graph)
        # -----------------------------------
        # The workspaces are already on the ADS, but should potentially be grouped
        # -----------------------------------
        group_workspaces_if_required(reduction_package, output_mode, save_can)

    # --------------------------------
    # Perform output of all workspaces
    # --------------------------------
    # We have three options here
    # 1. PublishToADS:
    #    * This means we can leave it as it is
    # 2. SaveToFile:
    #    * This means we need to save out the reduced data
    #    * Then we need to delete the reduced data from the ADS
    # 3. Both:
    #    * This means that we need to save out the reduced data
    #    * The data is already on the ADS, so do nothing
    if output_mode is OutputMode.SaveToFile:
        save_to_file(reduction_packages, save_can)
        delete_reduced_workspaces(reduction_packages)
    elif output_mode is OutputMode.Both:
        save_to_file(reduction_packages, save_can)

    # -----------------------------------------------------------------------
    # Clean up other workspaces if the optimizations have not been turned on.
    # -----------------------------------------------------------------------
    if not use_optimizations:
        delete_optimization_workspaces(reduction_packages, workspaces, monitors, save_can)

    out_scale_factors = [reduction_package.out_scale_factor for reduction_package in reduction_packages]
    out_shift_factors = [reduction_package.out_shift_factor for reduction_package in reduction_packages]

    return out_scale_factors, out_shift_factors


def load_workspaces_from_states(state):
    workspace_to_name = {SANSDataType.SampleScatter: "SampleScatterWorkspace",
                         SANSDataType.SampleTransmission: "SampleTransmissionWorkspace",
                         SANSDataType.SampleDirect: "SampleDirectWorkspace",
                         SANSDataType.CanScatter: "CanScatterWorkspace",
                         SANSDataType.CanTransmission: "CanTransmissionWorkspace",
                         SANSDataType.CanDirect: "CanDirectWorkspace"}

    workspace_to_monitor = {SANSDataType.SampleScatter: "SampleScatterMonitorWorkspace",
                            SANSDataType.CanScatter: "CanScatterMonitorWorkspace"}

    workspaces, monitors = provide_loaded_data(state, True, workspace_to_name, workspace_to_monitor)


# ----------------------------------------------------------------------------------------------------------------------
# Function for plotting
# ----------------------------------------------------------------------------------------------------------------------
def plot_workspace(reduction_package, output_graph):
    """
    Plotting continuous output when on MantidPlot
    This function should be deleted if and when MantidPlot is no longer a part of Mantid

    :param reduction_package: An object containing the reduced workspaces
    :param output_graph: Name to the plot window
    :return: None
    """
    if reduction_package.reduction_mode == ISISReductionMode.All:
        graph_handle = plotSpectrum([reduction_package.reduced_hab, reduction_package.reduced_lab], 0,
                                    window=graph(output_graph), clearWindow=True)
        graph_handle.activeLayer().logLogAxes()
    elif reduction_package.reduction_mode == ISISReductionMode.HAB:
        graph_handle = plotSpectrum(reduction_package.reduced_hab, 0, window=graph(output_graph), clearWindow=True)
        graph_handle.activeLayer().logLogAxes()
    elif reduction_package.reduction_mode == ISISReductionMode.LAB:
        graph_handle = plotSpectrum(reduction_package.reduced_lab, 0, window=graph(output_graph), clearWindow=True)
        graph_handle.activeLayer().logLogAxes()
    elif reduction_package.reduction_mode == ISISReductionMode.Merged:
        graph_handle = plotSpectrum([reduction_package.reduced_merged,
                                    reduction_package.reduced_hab, reduction_package.reduced_lab], 0,
                                    window=graph(output_graph), clearWindow=True)
        graph_handle.activeLayer().logLogAxes()


def plot_workspace_matplotlib(reduction_package, output_graph):
    """
    Continuous plotting using a matplotlib backend.

    :param reduction_package: An object containing the reduced workspaces
    :param output_graph: A matplotlib fig
    :return: None
    """
    plot_kwargs = {"scalex": True,
                   "scaley": True}
    if reduction_package.reduction_mode == ISISReductionMode.All:
        plot([reduction_package.reduced_hab, reduction_package.reduced_lab],
             wksp_indices=[0], overplot=True, fig=output_graph, plot_kwargs=plot_kwargs)
    elif reduction_package.reduction_mode == ISISReductionMode.HAB:
        plot([reduction_package.reduced_hab],
             wksp_indices=[0], overplot=True, fig=output_graph, plot_kwargs=plot_kwargs)
    elif reduction_package.reduction_mode == ISISReductionMode.LAB:
        plot([reduction_package.reduced_lab],
             wksp_indices=[0], overplot=True, fig=output_graph, plot_kwargs=plot_kwargs)
    elif reduction_package.reduction_mode == ISISReductionMode.Merged:
        plot([reduction_package.reduced_merged, reduction_package.reduced_hab, reduction_package.reduced_lab],
             wksp_indices=[0], overplot=True, fig=output_graph, plot_kwargs=plot_kwargs)


# ----------------------------------------------------------------------------------------------------------------------
# Functions for Data Loading
# ----------------------------------------------------------------------------------------------------------------------
def get_expected_workspace_names(file_information, is_transmission, period, get_base_name_only=False):
    """
    Creates the expected names for SANS workspaces.

    SANS scientists expect the load workspaces to have certain, typical names. For example, the file SANS2D00022024.nxs
    which is used as a transmission workspace translates into 22024_trans_nxs.
    :param file_information: a file information object
    :param is_transmission: if the file information is for a transmission or not
    :param period: the period of interest
    :param get_base_name_only: if we only want the base name and not the name with the period information
    :return: a list of workspace names
    """
    suffix_file_type = get_extension_for_file_type(file_information)
    if is_transmission:
        suffix_data = TRANS_SUFFIX
    else:
        suffix_data = SANS_SUFFIX

    run_number = file_information.get_run_number()

    # Three possibilities:
    #  1. No period data => 22024_sans_nxs
    #  2. Period data, but wants all => 22025p1_sans_nxs,  22025p2_sans_nxs, ...
    #  3. Period data, select particular period => 22025p3_sans_nxs
    if file_information.get_number_of_periods() == 1:
        workspace_name = "{0}_{1}_{2}".format(run_number, suffix_data, suffix_file_type)
        names = [workspace_name]
    elif file_information.get_number_of_periods() > 1 and period is StateData.ALL_PERIODS:
        workspace_names = []
        if get_base_name_only:
            workspace_names.append("{0}_{1}_{2}".format(run_number, suffix_data, suffix_file_type))
        else:
            for period in range(1, file_information.get_number_of_periods() + 1):
                workspace_names.append("{0}p{1}_{2}_{3}".format(run_number, period, suffix_data, suffix_file_type))
        names = workspace_names
    elif file_information.get_number_of_periods() > 1 and period is not StateData.ALL_PERIODS:
        workspace_name = "{0}p{1}_{2}_{3}".format(run_number, period, suffix_data, suffix_file_type)
        names = [workspace_name]
    else:
        raise RuntimeError("SANSLoad: Cannot create workspace names.")
    return names


def set_output_workspace_on_load_algorithm_for_one_workspace_type(load_options, load_workspace_name, file_name, period,
                                                                  is_transmission, file_info_factory,
                                                                  load_monitor_name=None):
    file_info = file_info_factory.create_sans_file_information(file_name)
    workspace_names = get_expected_workspace_names(file_info, is_transmission=is_transmission, period=period,
                                                   get_base_name_only=True)
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
                    "UseCached": use_optimizations}

    # Set the output workspaces
    set_output_workspaces_on_load_algorithm(load_options, state)

    load_alg = create_managed_non_child_algorithm(load_name, **load_options)
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

    for key, workspace_type in workspaces.items():
        for workspace in workspace_type:
            add_to_group(workspace, 'sans_interface_raw_data')
    for key, monitor_workspace_type in monitors.items():
        for monitor_workspace in monitor_workspace_type:
            add_to_group(monitor_workspace, 'sans_interface_raw_data')
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


def get_workspaces_from_load_algorithm(load_alg, workspace_to_count, workspace_name_dict):
    """
    Reads the workspaces from SANSLoad

    :param load_alg: a handle to the load algorithm
    :param workspace_to_count: a map from SANSDataType to the output-number property name of SANSLoad for workspaces
    :param workspace_name_dict: a map of SANSDataType vs output-property name of SANSLoad for (monitor) workspaces
    :return: a map of SANSDataType vs list of workspaces (to handle multi-period data)
    """
    workspace_output = {}
    for workspace_type, workspace_name in list(workspace_name_dict.items()):
        count_id = workspace_to_count[workspace_type]
        number_of_workspaces = load_alg.getProperty(count_id).value
        workspaces = []
        if number_of_workspaces > 1:
            workspaces = get_multi_period_workspaces(load_alg, workspace_name_dict[workspace_type],
                                                     number_of_workspaces)
        else:
            workspace_id = workspace_name_dict[workspace_type]
            workspace = get_workspace_from_algorithm(load_alg, workspace_id)
            if workspace is not None:
                workspaces.append(workspace)
        # Add the workspaces to the to the output
        workspace_output.update({workspace_type: workspaces})
    return workspace_output


def get_multi_period_workspaces(load_alg, workspace_name, number_of_workspaces):
    # Create an output name for each workspace and retrieve it from the load algorithm
    workspaces = []
    workspace_names = []
    for index in range(1, number_of_workspaces + 1):
        output_property_name = workspace_name + "_" + str(index)
        output_workspace_name = load_alg.getProperty(output_property_name).valueAsStr
        workspace_names.append(output_workspace_name)
        workspace = get_workspace_from_algorithm(load_alg, output_property_name)
        workspaces.append(workspace)

    # Group the workspaces
    base_name = get_base_name_from_multi_period_name(workspace_names[0])
    group_name = "GroupWorkspaces"
    group_options = {"InputWorkspaces": workspace_names,
                     "OutputWorkspace": base_name}
    group_alg = create_unmanaged_algorithm(group_name, **group_options)
    group_alg.setChild(False)
    group_alg.execute()
    return workspaces


# ----------------------------------------------------------------------------------------------------------------------
# Functions for reduction packages
# ----------------------------------------------------------------------------------------------------------------------
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
        reduction_packages = split_reduction_packages_for_event_slice_packages(reduction_packages)

    if reduction_packages_require_splitting_for_wavelength_range(reduction_packages):
        reduction_packages = split_reduction_packages_for_wavelength_range(reduction_packages)
    return reduction_packages


def reduction_packages_require_splitting_for_event_slices(reduction_packages):
    """
    Creates reduction packages from a list of reduction packages by splitting up event slices.

    The SANSSingleReduction algorithm can handle only a single time slice. For each time slice, we require an individual
    reduction. Hence we split the states up at this point.
    :param reduction_packages: a list of reduction packages.
    :return: a list of reduction packages which has at least the same length as the input
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


def reduction_packages_require_splitting_for_wavelength_range(reduction_packages):
    """
        Creates reduction packages from a list of reduction packages by splitting up wavelength ranges.

        The SANSSingleReduction algorithm can handle only a single wavelength range. For each wavelength range, we require an individual
        reduction. Hence we split the states up at this point.
        :param reduction_packages: a list of reduction packages.
        :return: a list of reduction packages which has at least the same length as the input
        """
    # Determine if the event slice sub-state object contains multiple event slice requests. This is given
    # by the number of elements in start_tof
    reduction_package = reduction_packages[0]
    state = reduction_package.state
    wavelength_info = state.wavelength
    start_wavelength = wavelength_info.wavelength_low
    if start_wavelength is not None and len(start_wavelength) > 1:
        requires_split = True
    else:
        requires_split = False
    return requires_split


def split_reduction_packages_for_wavelength_range(reduction_packages):
    reduction_packages_split = []
    for reduction_package in reduction_packages:
        state = reduction_package.state
        wavelength_info = state.wavelength
        start_wavelength = wavelength_info.wavelength_low
        end_wavelength = wavelength_info.wavelength_high

        states = []
        for start, end in zip(start_wavelength, end_wavelength):
            state_copy = deepcopy(state)

            state_copy.wavelength.wavelength_low = [start]
            state_copy.wavelength.wavelength_high = [end]

            state_copy.adjustment.normalize_to_monitor.wavelength_low = [start]
            state_copy.adjustment.normalize_to_monitor.wavelength_high = [end]

            state_copy.adjustment.calculate_transmission.wavelength_low = [start]
            state_copy.adjustment.calculate_transmission.wavelength_high = [end]

            state_copy.adjustment.wavelength_and_pixel_adjustment.wavelength_low = [start]
            state_copy.adjustment.wavelength_and_pixel_adjustment.wavelength_high = [end]

            states.append(state_copy)

        workspaces = reduction_package.workspaces
        monitors = reduction_package.monitors
        is_part_of_multi_period_reduction = reduction_package.is_part_of_multi_period_reduction
        is_part_of_event_slice_reduction = reduction_package.is_part_of_event_slice_reduction
        for state in states:
            new_state = deepcopy(state)
            new_reduction_package = ReductionPackage(state=new_state,
                                                     workspaces=workspaces,
                                                     monitors=monitors,
                                                     is_part_of_multi_period_reduction=is_part_of_multi_period_reduction,
                                                     is_part_of_event_slice_reduction=is_part_of_event_slice_reduction,
                                                     is_part_of_wavelength_range_reduction=True)
            reduction_packages_split.append(new_reduction_package)
    return reduction_packages_split


def split_reduction_packages_for_event_slice_packages(reduction_packages):
    """
    Splits a reduction package object into several reduction package objects if it contains several event slice settings

    We want to split this up here since each event slice is a full reduction cycle in itself.
    :param reduction_packages: a list of reduction packages
    :return: a list of reduction packages where each reduction setting contains only one event slice.
    """
    # Since the state is the same for all reduction packages at this point we only need to create the split state once
    # for the first package and the apply to all the other packages. If we have 5 reduction packages and the user
    # requests 6 event slices, then we end up with 60 reductions!
    reduction_packages_split = []
    for reduction_package in reduction_packages:
        state = reduction_package.state
        slice_event_info = state.slice
        start_time = slice_event_info.start_time
        end_time = slice_event_info.end_time

        states = []
        for start, end in zip(start_time, end_time):
            state_copy = deepcopy(state)
            slice_event_info = state_copy.slice
            slice_event_info.start_time = [start]
            slice_event_info.end_time = [end]
            states.append(state_copy)

        workspaces = reduction_package.workspaces
        monitors = reduction_package.monitors
        is_part_of_multi_period_reduction = reduction_package.is_part_of_multi_period_reduction

        for state in states:
            new_state = deepcopy(state)
            new_reduction_package = ReductionPackage(state=new_state,
                                                     workspaces=workspaces,
                                                     monitors=monitors,
                                                     is_part_of_multi_period_reduction=is_part_of_multi_period_reduction,
                                                     is_part_of_event_slice_reduction=True)
            reduction_packages_split.append(new_reduction_package)
    return reduction_packages_split


def create_initial_reduction_packages(state, workspaces, monitors):
    """
    This provides the initial split of the workspaces.

    If the data stems from multi-period data, then we need to split up the workspaces. The state object is valid
    for each one of these workspaces. Hence we need to create a deep copy of them for each reduction package.

    The way multi-period files are handled over the different workspaces input types is:
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

    data_info = state.data
    sample_scatter_period = data_info.sample_scatter_period
    requires_new_period_selection = len(workspaces[SANSDataType.SampleScatter]) > 1 \
                                    and sample_scatter_period == ALL_PERIODS  # noqa

    is_multi_period = len(workspaces[SANSDataType.SampleScatter]) > 1

    for index in range(0, len(workspaces[SANSDataType.SampleScatter])):
        workspaces_for_package = {}
        # For each workspace type, i.e sample scatter, can transmission, etc. find the correct workspace
        for workspace_type, workspace_list in list(workspaces.items()):
            workspace = get_workspace_for_index(index, workspace_list)
            workspaces_for_package.update({workspace_type: workspace})

        # For each monitor type, find the correct workspace
        monitors_for_package = {}
        for workspace_type, workspace_list in list(monitors.items()):
            workspace = get_workspace_for_index(index, workspace_list)
            monitors_for_package.update({workspace_type: workspace})
        state_copy = deepcopy(state)

        # Set the period on the state
        if requires_new_period_selection:
            state_copy.data.sample_scatter_period = index + 1
        packages.append(ReductionPackage(state=state_copy,
                                         workspaces=workspaces_for_package,
                                         monitors=monitors_for_package,
                                         is_part_of_multi_period_reduction=is_multi_period,
                                         is_part_of_event_slice_reduction=False))
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
    def _set_output_name(_reduction_alg, _reduction_package, _is_group, _reduction_mode, _property_name,
                         _attr_out_name, _atrr_out_name_base, multi_reduction_type, _suffix=None, transmission=False):
        if not transmission:
            _out_name, _out_name_base = get_output_name(_reduction_package.state, _reduction_mode, _is_group,
                                                        multi_reduction_type=multi_reduction_type)
        else:
            _out_name, _out_name_base = get_transmission_output_name(_reduction_package.state, _reduction_mode
                                                                     , multi_reduction_type=multi_reduction_type)

        if _suffix is not None:
            _out_name += _suffix
            _out_name_base += _suffix

        _reduction_alg.setProperty(_property_name, _out_name)
        setattr(_reduction_package, _attr_out_name, _out_name)
        setattr(_reduction_package, _atrr_out_name_base, _out_name_base)

    def _set_output_name_from_string(reduction_alg, reduction_package, algorithm_property_name, workspace_name,
                                     workspace_name_base, package_attribute_name, package_attribute_name_base):
        reduction_alg.setProperty(algorithm_property_name, workspace_name)
        setattr(reduction_package, package_attribute_name, workspace_name)
        setattr(reduction_package, package_attribute_name_base, workspace_name_base)

    def _set_lab(_reduction_alg, _reduction_package, _is_group):
        _set_output_name(_reduction_alg, _reduction_package, _is_group, ISISReductionMode.LAB,
                         "OutputWorkspaceLABCan", "reduced_lab_can_name", "reduced_lab_can_base_name",
                         multi_reduction_type, LAB_CAN_SUFFIX)

        # Lab Can Count workspace - this is a partial workspace
        _set_output_name(_reduction_alg, _reduction_package, _is_group, ISISReductionMode.LAB,
                         "OutputWorkspaceLABCanCount", "reduced_lab_can_count_name", "reduced_lab_can_count_base_name",
                         multi_reduction_type, LAB_CAN_COUNT_SUFFIX)

        # Lab Can Norm workspace - this is a partial workspace
        _set_output_name(_reduction_alg, _reduction_package, _is_group, ISISReductionMode.LAB,
                         "OutputWorkspaceLABCanNorm", "reduced_lab_can_norm_name", "reduced_lab_can_norm_base_name",
                         multi_reduction_type, LAB_CAN_NORM_SUFFIX)

        _set_output_name(_reduction_alg, _reduction_package, _is_group, ISISReductionMode.LAB,
                         "OutputWorkspaceLABSample", "reduced_lab_sample_name", "reduced_lab_sample_base_name",
                         multi_reduction_type, LAB_SAMPLE_SUFFIX)

    def _set_hab(_reduction_alg, _reduction_package, _is_group):
        # Hab Can Workspace
        _set_output_name(_reduction_alg, _reduction_package, _is_group, ISISReductionMode.HAB,
                         "OutputWorkspaceHABCan", "reduced_hab_can_name", "reduced_hab_can_base_name",
                         multi_reduction_type, HAB_CAN_SUFFIX)

        # Hab Can Count workspace - this is a partial workspace
        _set_output_name(_reduction_alg, _reduction_package, _is_group, ISISReductionMode.HAB,
                         "OutputWorkspaceHABCanCount", "reduced_hab_can_count_name", "reduced_hab_can_count_base_name",
                         multi_reduction_type, HAB_CAN_COUNT_SUFFIX)

        # Hab Can Norm workspace - this is a partial workspace
        _set_output_name(_reduction_alg, _reduction_package, _is_group, ISISReductionMode.HAB,
                         "OutputWorkspaceHABCanNorm", "reduced_hab_can_norm_name", "reduced_hab_can_norm_base_name",
                         multi_reduction_type, HAB_CAN_NORM_SUFFIX)

        _set_output_name(_reduction_alg, _reduction_package, _is_group, ISISReductionMode.HAB,
                         "OutputWorkspaceHABSample", "reduced_hab_sample_name", "reduced_hab_sample_base_name",
                         multi_reduction_type, HAB_SAMPLE_SUFFIX)

    # Go through the elements of the reduction package and set them on the reduction algorithm
    # Set the SANSState
    state = reduction_package.state
    state_dict = state.property_manager
    reduction_alg.setProperty("SANSState", state_dict)

    # Set the input workspaces
    workspaces = reduction_package.workspaces
    for workspace_type, workspace in list(workspaces.items()):
        if workspace is not None:
            reduction_alg.setProperty(workspace_to_name[workspace_type], workspace)

    # Set the monitors
    monitors = reduction_package.monitors
    for workspace_type, monitor in list(monitors.items()):
        if monitor is not None:
            reduction_alg.setProperty(workspace_to_monitor[workspace_type], monitor)

    # ------------------------------------------------------------------------------------------------------------------
    # Set the output workspaces for LAB, HAB and Merged
    # ------------------------------------------------------------------------------------------------------------------
    is_part_of_multi_period_reduction = reduction_package.is_part_of_multi_period_reduction
    is_part_of_event_slice_reduction = reduction_package.is_part_of_event_slice_reduction
    is_part_of_wavelength_range_reduction = reduction_package.is_part_of_wavelength_range_reduction
    is_group = is_part_of_multi_period_reduction or is_part_of_event_slice_reduction or is_part_of_wavelength_range_reduction
    multi_reduction_type = {"period": is_part_of_multi_period_reduction, "event_slice": is_part_of_event_slice_reduction,
                            "wavelength_range": is_part_of_wavelength_range_reduction}

    reduction_mode = reduction_package.reduction_mode
    if reduction_mode is ISISReductionMode.Merged:
        _set_output_name(reduction_alg, reduction_package, is_group, ISISReductionMode.Merged,
                         "OutputWorkspaceMerged", "reduced_merged_name", "reduced_merged_base_name", multi_reduction_type)
        _set_output_name(reduction_alg, reduction_package, is_group, ISISReductionMode.LAB,
                         "OutputWorkspaceLAB", "reduced_lab_name", "reduced_lab_base_name", multi_reduction_type)
        _set_output_name(reduction_alg, reduction_package, is_group, ISISReductionMode.HAB,
                         "OutputWorkspaceHAB", "reduced_hab_name", "reduced_hab_base_name", multi_reduction_type)
    elif reduction_mode is ISISReductionMode.LAB:
        _set_output_name(reduction_alg, reduction_package, is_group, ISISReductionMode.LAB,
                         "OutputWorkspaceLAB", "reduced_lab_name", "reduced_lab_base_name", multi_reduction_type)
    elif reduction_mode is ISISReductionMode.HAB:
        _set_output_name(reduction_alg, reduction_package, is_group, ISISReductionMode.HAB,
                         "OutputWorkspaceHAB", "reduced_hab_name", "reduced_hab_base_name", multi_reduction_type)
    elif reduction_mode is ISISReductionMode.All:
        _set_output_name(reduction_alg, reduction_package, is_group, ISISReductionMode.LAB,
                         "OutputWorkspaceLAB", "reduced_lab_name", "reduced_lab_base_name", multi_reduction_type)
        _set_output_name(reduction_alg, reduction_package, is_group, ISISReductionMode.HAB,
                         "OutputWorkspaceHAB", "reduced_hab_name", "reduced_hab_base_name", multi_reduction_type)
    else:
        raise RuntimeError("The reduction mode {0} is not known".format(reduction_mode))

    # ------------------------------------------------------------------------------------------------------------------
    # Set the output workspaces for the can reduction and the partial can reductions
    # ------------------------------------------------------------------------------------------------------------------
    # Set the output workspaces for the can reductions -- note that these will only be set if optimizations
    # are enabled
    # Lab Can Workspace
    if reduction_mode is ISISReductionMode.Merged:
        _set_lab(reduction_alg, reduction_package, is_group)
        _set_hab(reduction_alg, reduction_package, is_group)
    elif reduction_mode is ISISReductionMode.LAB:
        _set_lab(reduction_alg, reduction_package, is_group)
    elif reduction_mode is ISISReductionMode.HAB:
        _set_hab(reduction_alg, reduction_package, is_group)
    elif reduction_mode is ISISReductionMode.All:
        _set_lab(reduction_alg, reduction_package, is_group)
        _set_hab(reduction_alg, reduction_package, is_group)
    else:
        raise RuntimeError("The reduction mode {0} is not known".format(reduction_mode))

    # ------------------------------------------------------------------------------------------------------------------
    # Set the output workspaces for the calculated and unfitted transmission
    # ------------------------------------------------------------------------------------------------------------------
    sample_calculated_transmission, \
    sample_calculated_transmission_base = get_transmission_output_name(reduction_package.state, DataType.Sample,
                                                                       multi_reduction_type, True)
    can_calculated_transmission, \
    can_calculated_transmission_base = get_transmission_output_name(reduction_package.state, DataType.Can,
                                                                    multi_reduction_type, True)
    sample_unfitted_transmission, \
    sample_unfitted_transmission_base = get_transmission_output_name(reduction_package.state, DataType.Sample,
                                                                     multi_reduction_type, False)
    can_unfitted_transmission, \
    can_unfitted_transmission_base = get_transmission_output_name(reduction_package.state, DataType.Can,
                                                                  multi_reduction_type, False)

    _set_output_name_from_string(reduction_alg, reduction_package, "OutputWorkspaceCalculatedTransmission",
                                 sample_calculated_transmission, sample_calculated_transmission_base
                                 , "calculated_transmission_name", "calculated_transmission_base_name")

    _set_output_name_from_string(reduction_alg, reduction_package, "OutputWorkspaceUnfittedTransmission",
                                 sample_unfitted_transmission, sample_unfitted_transmission_base
                                 , "unfitted_transmission_name", "unfitted_transmission_base_name")

    _set_output_name_from_string(reduction_alg, reduction_package, "OutputWorkspaceCalculatedTransmissionCan",
                                 can_calculated_transmission, can_calculated_transmission_base
                                 , "calculated_transmission_can_name", "calculated_transmission_can_base_name")

    _set_output_name_from_string(reduction_alg, reduction_package, "OutputWorkspaceUnfittedTransmissionCan",
                                 can_unfitted_transmission, can_unfitted_transmission_base
                                 , "unfitted_transmission_can_name", "unfitted_transmission_can_base_name")


def get_workspace_from_algorithm(alg, output_property_name):
    """
    Gets the output workspace from an algorithm. Since we don't run this as a child we need to get it from the
    ADS.

    :param alg: a handle to the algorithm from which we want to take the output workspace property.
    :param output_property_name: the name of the output property.
    :return the workspace or None
    """
    output_workspace_name = alg.getProperty(output_property_name).valueAsStr

    if not output_workspace_name:
        return None

    if AnalysisDataService.doesExist(output_workspace_name):
        return AnalysisDataService.retrieve(output_workspace_name)
    else:
        return None


# ----------------------------------------------------------------------------------------------------------------------
# Functions for outputs to the ADS and saving the file
# ----------------------------------------------------------------------------------------------------------------------
def group_workspaces_if_required(reduction_package, output_mode, save_can):
    """
    The output workspaces have already been published to the ADS by the algorithm. Now we might have to
    bundle them into a group if:
    * They are part of a multi-period workspace or a sliced reduction
    * They are reduced LAB and HAB workspaces of a Merged reduction
    * They are can workspaces - they are all grouped into a single group
    :param reduction_package: a list of reduction packages
    :param output_mode: one of OutputMode. SaveToFile, PublishToADS, Both.
    :param save_can: a bool. If true save out can and sample workspaces.
    """
    is_part_of_multi_period_reduction = reduction_package.is_part_of_multi_period_reduction
    is_part_of_event_slice_reduction = reduction_package.is_part_of_event_slice_reduction
    is_part_of_wavelength_range_reduction = reduction_package.is_part_of_wavelength_range_reduction
    requires_grouping = is_part_of_multi_period_reduction or is_part_of_event_slice_reduction\
        or is_part_of_wavelength_range_reduction

    reduced_lab = reduction_package.reduced_lab
    reduced_hab = reduction_package.reduced_hab
    reduced_merged = reduction_package.reduced_merged

    is_merged_reduction = reduced_merged is not None

    # Add the reduced workspaces to groups if they require this
    if is_merged_reduction:
        if requires_grouping:
            add_to_group(reduced_merged, reduction_package.reduced_merged_base_name)
        add_to_group(reduced_lab, REDUCED_HAB_AND_LAB_WORKSPACE_FOR_MERGED_REDUCTION)
        add_to_group(reduced_hab, REDUCED_HAB_AND_LAB_WORKSPACE_FOR_MERGED_REDUCTION)
    else:
        if requires_grouping:
            add_to_group(reduced_lab, reduction_package.reduced_lab_base_name)
            add_to_group(reduced_hab, reduction_package.reduced_hab_base_name)

    # Can group workspace depends on if save_can is checked and output_mode
    # Logic table for which group to save CAN into
    # CAN | FILE | In OPTIMIZATION group
    # ----------------------------------
    #  Y  |   Y  | YES
    #  N  |   Y  | YES
    #  Y  |   N  | NO
    #  N  |   N  | YES

    if save_can and output_mode is not OutputMode.SaveToFile:
        CAN_WORKSPACE_GROUP = CAN_AND_SAMPLE_WORKSPACE
    else:
        CAN_WORKSPACE_GROUP = CAN_COUNT_AND_NORM_FOR_OPTIMIZATION

    # Add the can workspaces (used for optimizations) to a Workspace Group (if they exist)
    add_to_group(reduction_package.reduced_lab_can, CAN_WORKSPACE_GROUP)
    add_to_group(reduction_package.reduced_lab_can_count, CAN_COUNT_AND_NORM_FOR_OPTIMIZATION)
    add_to_group(reduction_package.reduced_lab_can_norm, CAN_COUNT_AND_NORM_FOR_OPTIMIZATION)

    add_to_group(reduction_package.reduced_hab_can, CAN_WORKSPACE_GROUP)
    add_to_group(reduction_package.reduced_hab_can_count, CAN_COUNT_AND_NORM_FOR_OPTIMIZATION)
    add_to_group(reduction_package.reduced_hab_can_norm, CAN_COUNT_AND_NORM_FOR_OPTIMIZATION)

    add_to_group(reduction_package.reduced_lab_sample, CAN_AND_SAMPLE_WORKSPACE)
    add_to_group(reduction_package.reduced_hab_sample, CAN_AND_SAMPLE_WORKSPACE)

    add_to_group(reduction_package.calculated_transmission, reduction_package.calculated_transmission_base_name)
    add_to_group(reduction_package.calculated_transmission_can,
                 reduction_package.calculated_transmission_can_base_name)
    add_to_group(reduction_package.unfitted_transmission, reduction_package.unfitted_transmission_base_name)
    add_to_group(reduction_package.unfitted_transmission_can, reduction_package.unfitted_transmission_can_base_name)


def add_to_group(workspace, name_of_group_workspace):
    """
    Creates a group workspace with the base name for the workspace

    :param workspace: the workspace to add to the WorkspaceGroup
    :param name_of_group_workspace: the name of the WorkspaceGroup
    """
    if workspace is None:
        return
    name_of_workspace = workspace.name()
    if AnalysisDataService.doesExist(name_of_group_workspace):
        group_workspace = AnalysisDataService.retrieve(name_of_group_workspace)
        if type(group_workspace) is WorkspaceGroup:
            if not group_workspace.contains(name_of_workspace):
                group_workspace.add(name_of_workspace)
        else:
            group_name = "GroupWorkspaces"
            group_options = {"InputWorkspaces": [name_of_workspace],
                             "OutputWorkspace": name_of_group_workspace}
            group_alg = create_unmanaged_algorithm(group_name, **group_options)

            group_alg.setAlwaysStoreInADS(True)
            group_alg.execute()
    else:
        group_name = "GroupWorkspaces"
        group_options = {"InputWorkspaces": [name_of_workspace],
                         "OutputWorkspace": name_of_group_workspace}
        group_alg = create_unmanaged_algorithm(group_name, **group_options)

        group_alg.setAlwaysStoreInADS(True)
        group_alg.execute()


def save_to_file(reduction_packages, save_can):
    """
    Extracts all workspace names which need to be saved and saves them into a file.

    @param reduction_packages: a list of reduction packages which contain all the relevant information for saving
    @param save_can: a bool. When true save the unsubtracted can and sample workspaces
    """
    workspaces_names_to_save = get_all_names_to_save(reduction_packages, save_can=save_can)

    state = reduction_packages[0].state
    save_info = state.save
    file_formats = save_info.file_format
    for name_to_save in workspaces_names_to_save:
        if isinstance(name_to_save, tuple):
            transmission = name_to_save[1]
            transmission_can = name_to_save[2]
            name_to_save = name_to_save[0]
            save_workspace_to_file(name_to_save, file_formats, name_to_save, transmission, transmission_can)
        else:
            save_workspace_to_file(name_to_save, file_formats, name_to_save)


def delete_reduced_workspaces(reduction_packages, include_non_transmission=True):
    """
    Deletes all workspaces which would have been generated from a list of reduction packages.

    @param reduction_packages: a list of reduction package
    """
    def _delete_workspaces(_delete_alg, _workspaces):
        for _workspace in _workspaces:
            if _workspace is not None:
                _delete_alg.setProperty("Workspace", _workspace.name())
                _delete_alg.execute()
    # Get all names which were saved out to workspaces
    # Delete each workspace
    delete_name = "DeleteWorkspace"
    delete_options = {}
    delete_alg = create_unmanaged_algorithm(delete_name, **delete_options)

    for reduction_package in reduction_packages:
        # Remove transmissions
        calculated_transmission = reduction_package.calculated_transmission
        unfitted_transmission = reduction_package.unfitted_transmission
        calculated_transmission_can = reduction_package.calculated_transmission_can
        unfitted_transmission_can = reduction_package.unfitted_transmission_can

        workspaces_to_delete = [calculated_transmission, unfitted_transmission,
                                calculated_transmission_can, unfitted_transmission_can]

        if include_non_transmission:
            reduced_lab = reduction_package.reduced_lab
            reduced_hab = reduction_package.reduced_hab
            reduced_merged = reduction_package.reduced_merged

            # Remove samples
            reduced_lab_sample = reduction_package.reduced_lab_sample
            reduced_hab_sample = reduction_package.reduced_hab_sample

            workspaces_to_delete.extend([reduced_lab, reduced_hab, reduced_merged,
                                         reduced_lab_sample, reduced_hab_sample])

        _delete_workspaces(delete_alg, workspaces_to_delete)


def delete_optimization_workspaces(reduction_packages, workspaces, monitors, save_can):
    """
    Deletes all workspaces which are used for optimizations. This can be loaded workspaces or can optimizations

    :param reduction_packages: a list of reductioin packages.
    """
    def _delete_workspaces(_delete_alg, _workspaces):
        _workspace_names_to_delete = set([_workspace.name() for _workspace in _workspaces if _workspace is not None])
        for _workspace_name_to_delete in _workspace_names_to_delete:
            if _workspace_name_to_delete and AnalysisDataService.doesExist(_workspace_name_to_delete):
                _delete_alg.setProperty("Workspace", _workspace_name_to_delete)
                _delete_alg.execute()

    def _delete_workspaces_from_dict(_delete_alg, workspaces):
        _workspace_names_to_delete = []
        for key, workspace_list in workspaces.items():
            for workspace in workspace_list:
                if workspace and workspace.name():
                    _workspace_names_to_delete.append(workspace.name())

        for _workspace_name_to_delete in _workspace_names_to_delete:
            if _workspace_name_to_delete and AnalysisDataService.doesExist(_workspace_name_to_delete):
                _delete_alg.setProperty("Workspace", _workspace_name_to_delete)
                _delete_alg.execute()

    delete_name = "DeleteWorkspace"
    delete_options = {}
    delete_alg = create_unmanaged_algorithm(delete_name, **delete_options)

    _delete_workspaces_from_dict(delete_alg, workspaces)

    _delete_workspaces_from_dict(delete_alg, monitors)

    for reduction_package in reduction_packages:
        # Delete can optimizations
        optimizations_to_delete = [reduction_package.reduced_lab_can_count,
                                   reduction_package.reduced_lab_can_norm,
                                   reduction_package.reduced_hab_can_count,
                                   reduction_package.reduced_hab_can_norm]
        if not save_can:
            optimizations_to_delete.extend([reduction_package.reduced_lab_can,
                                            reduction_package.reduced_hab_can])
        _delete_workspaces(delete_alg, optimizations_to_delete)

def get_transmission_names_to_save(reduction_package, can):
    """
    For transmission workspaces, we use the names for them attached to the
    reduction package, rather than the name attached to the workspace variable.
    This is to avoid a bug where python variables containing workspaces which are also
    on the ADS appear to be deleted.
    This affects transmission workspaces for event slice data.
    :param reduction_package: an object containing workspace information
    :param can: a bool. If true then we are retrieving the transmission can.
                Else retrieve the transmission
    :return: Workspace name
    """
    if can:
        base_name = reduction_package.unfitted_transmission_can_base_name
        ws_name = reduction_package.unfitted_transmission_can_name
    else:
        base_name = reduction_package.unfitted_transmission_base_name
        ws_name = reduction_package.unfitted_transmission_name
    name_to_return = ''
    if AnalysisDataService.doesExist(base_name):
        group = AnalysisDataService.retrieve(base_name)
        if group.contains(ws_name):
            name_to_return = ws_name
    return name_to_return


def get_all_names_to_save(reduction_packages, save_can):
    """
    Extracts all the output names from a list of reduction packages. The main

    @param reduction_packages: a list of reduction packages
    @param save_can: a bool, whether or not to save unsubtracted can workspace
    @return: a list of workspace names to save.
    """
    names_to_save = []
    for reduction_package in reduction_packages:
        reduced_lab = reduction_package.reduced_lab
        reduced_hab = reduction_package.reduced_hab
        reduced_merged = reduction_package.reduced_merged
        reduced_lab_can = reduction_package.reduced_lab_can
        reduced_hab_can = reduction_package.reduced_hab_can
        reduced_lab_sample = reduction_package.reduced_lab_sample
        reduced_hab_sample = reduction_package.reduced_hab_sample

        trans_name = get_transmission_names_to_save(reduction_package, False)
        transcan_name = get_transmission_names_to_save(reduction_package, True)

        if save_can:
            if reduced_merged:
                names_to_save.append((reduced_merged.name(), trans_name, transcan_name))
            if reduced_lab:
                names_to_save.append((reduced_lab.name(), trans_name, transcan_name))
            if reduced_hab:
                names_to_save.append((reduced_hab.name(), trans_name, transcan_name))
            if reduced_lab_can:
                names_to_save.append((reduced_lab_can.name(), '', transcan_name))
            if reduced_hab_can:
                names_to_save.append((reduced_hab_can.name(), '', transcan_name))
            if reduced_lab_sample:
                names_to_save.append((reduced_lab_sample.name(), trans_name, ''))
            if reduced_hab_sample:
                names_to_save.append((reduced_hab_sample.name(), trans_name, ''))

        # If we have merged reduction then store the
        elif reduced_merged:
            names_to_save.append((reduced_merged.name(), trans_name, transcan_name))
        else:
            if reduced_lab:
                names_to_save.append((reduced_lab.name(), trans_name, transcan_name))
            if reduced_hab:
                names_to_save.append((reduced_hab.name(), trans_name, transcan_name))

    # We might have some workspaces as duplicates (the group workspaces), so make them unique
    return set(names_to_save)


def save_workspace_to_file(workspace_name, file_formats, file_name,
                           transmission_name='', transmission_can_name=''):
    """
    Saves the workspace to the different file formats specified in the state object.

    :param workspace_name: the name of the output workspace and also the name of the file
    :param file_formats: a list of file formats to save
    :param transmission_name: name of sample transmission workspace to save to file
            for CanSAS algorithm. Only some workspaces have a corresponding transmission workspace.
    :param transmission_can_name: name of can transmission workspace. As above.
    """
    save_name = "SANSSave"
    save_options = {"InputWorkspace": workspace_name}
    save_options.update({"Filename": file_name,
                         "Transmission": transmission_name,
                         "TransmissionCan": transmission_can_name})

    if SaveType.Nexus in file_formats:
        save_options.update({"Nexus": True})
    if SaveType.CanSAS in file_formats:
        save_options.update({"CanSAS": True})
    if SaveType.NXcanSAS in file_formats:
        save_options.update({"NXcanSAS": True})
    if SaveType.NistQxy in file_formats:
        save_options.update({"NistQxy": True})
    if SaveType.RKH in file_formats:
        save_options.update({"RKH": True})
    if SaveType.CSV in file_formats:
        save_options.update({"CSV": True})

    save_alg = create_unmanaged_algorithm(save_name, **save_options)
    save_alg.execute()


# ----------------------------------------------------------------------------------------------------------------------
# Container classes
# ----------------------------------------------------------------------------------------------------------------------
class ReducedDataType(object):
    class Merged(object):
        pass

    class LAB(object):
        pass

    class HAB(object):
        pass


class ReductionPackage(object):
    """
    The reduction package is a mutable store for
    1. The state object which defines our reductions.
    2. A dictionary with input_workspace_type vs input_workspace
    3. A dictionary with input_monitor_workspace_type vs input_monitor_workspace
    4. A flag which indicates if the reduction is part of a multi-period reduction
    5. A flag which indicates if the reduction is part of a sliced reduction
    6. The reduced workspaces (not all need to exist)
    7. The reduced can and the reduced partial can workspaces (non have to exist, this is only for optimizations)
    8. The unfitted transmission workspaces
    """
    def __init__(self, state, workspaces, monitors, is_part_of_multi_period_reduction=False,
                 is_part_of_event_slice_reduction=False, is_part_of_wavelength_range_reduction=False):
        super(ReductionPackage, self).__init__()
        # -------------------------------------------------------
        # General Settings
        # -------------------------------------------------------
        self.state = state
        self.workspaces = workspaces
        self.monitors = monitors
        self.is_part_of_multi_period_reduction = is_part_of_multi_period_reduction
        self.is_part_of_event_slice_reduction = is_part_of_event_slice_reduction
        self.is_part_of_wavelength_range_reduction = is_part_of_wavelength_range_reduction
        self.reduction_mode = state.reduction.reduction_mode

        # -------------------------------------------------------
        # Reduced workspaces
        # -------------------------------------------------------
        self.reduced_lab = None
        self.reduced_hab = None
        self.reduced_merged = None

        # -------------------------------------------------------
        # Reduced partial can workspaces (and partial workspaces)
        # -------------------------------------------------------
        self.reduced_lab_can = None
        self.reduced_lab_can_count = None
        self.reduced_lab_can_norm = None

        self.reduced_hab_can = None
        self.reduced_hab_can_count = None
        self.reduced_hab_can_norm = None

        # -------------------------------------------------------
        # Output names and base names
        # -------------------------------------------------------
        self.reduced_lab_name = None
        self.reduced_lab_base_name = None
        self.reduced_hab_name = None
        self.reduced_hab_base_name = None
        self.reduced_merged_name = None
        self.reduced_merged_base_name = None

        # Partial reduced can workspace names
        self.reduced_lab_can_name = None
        self.reduced_lab_can_base_name = None
        self.reduced_lab_can_count_name = None
        self.reduced_lab_can_count_base_name = None
        self.reduced_lab_can_norm_name = None
        self.reduced_lab_can_norm_base_name = None

        self.reduced_hab_can_name = None
        self.reduced_hab_can_base_name = None
        self.reduced_hab_can_count_name = None
        self.reduced_hab_can_count_base_name = None
        self.reduced_hab_can_norm_name = None
        self.reduced_hab_can_norm_base_name = None

        self.out_scale_factor = None
        self.out_shift_factor = None

        # Unfitted transmission names
        self.unfitted_transmission_name = None
        self.unfitted_transmission_base_name = None
        self.unfitted_transmission_can_name = None
        self.unfitted_transmission_can_base_name = None
