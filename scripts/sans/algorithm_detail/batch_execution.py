# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from typing import Dict, Any
from copy import deepcopy

from mantid.api import AnalysisDataService, WorkspaceGroup
from mantid.dataobjects import Workspace2D
from sans.common.general_functions import (
    create_managed_non_child_algorithm,
    create_unmanaged_algorithm,
    get_output_name,
    get_base_name_from_multi_period_name,
    get_transmission_output_name,
    get_wav_range_from_ws,
)
from sans.common.enums import SANSDataType, SaveType, OutputMode, ReductionMode, DataType
from sans.common.constants import (
    TRANS_SUFFIX,
    SANS_SUFFIX,
    ALL_PERIODS,
    LAB_CAN_SUFFIX,
    LAB_CAN_COUNT_SUFFIX,
    LAB_CAN_NORM_SUFFIX,
    HAB_CAN_SUFFIX,
    HAB_CAN_COUNT_SUFFIX,
    HAB_CAN_NORM_SUFFIX,
    LAB_SAMPLE_SUFFIX,
    HAB_SAMPLE_SUFFIX,
    REDUCED_HAB_AND_LAB_WORKSPACE_FOR_MERGED_REDUCTION,
    CAN_COUNT_AND_NORM_FOR_OPTIMIZATION,
    CAN_AND_SAMPLE_WORKSPACE,
    SCALED_BGSUB_SUFFIX,
)
from sans.common.file_information import get_extension_for_file_type, SANSFileInformationFactory
from sans.common.plotting import get_plotting_module
from sans.state.Serializer import Serializer
from sans.state.StateObjects.StateData import StateData


# ----------------------------------------------------------------------------------------------------------------------
# Functions for the execution of a single batch iteration
# ----------------------------------------------------------------------------------------------------------------------
def select_reduction_alg(split_for_event_slices, use_compatibility_mode, event_slice_optimisation_selected, reduction_packages):
    """
    Select whether the data should be reduced via version 1 or 2 of SANSSingleReduction.
    To use version 2, the reduction must be carried out with event slices, compatibility mode
    must not have been switched on (via the sans_data_processor_gui), and event slice mode must have been switched on
    (via the sans_data_processor_gui)
    :param split_for_event_slices: bool. If true, event slicing will be carried out in the reduction
    :param use_compatibility_mode: bool. If true, compatibility mode has been turned on
    :param event_slice_optimisation_selected: bool. If true, event slice mode has been turned on
    :param reduction_packages: a list of reduction package objects
    :return:whether or not we're event slicing (bool); reduction packages
    """
    if split_for_event_slices and not use_compatibility_mode and event_slice_optimisation_selected:
        # If using compatibility mode we convert to histogram immediately after taking event slices,
        # so would not be able to perform operations on event workspaces pre-slicing.
        event_slice_optimisation = True
    else:
        event_slice_optimisation = False
        if split_for_event_slices:
            # Split into separate event slice workspaces here.
            # For event_slice mode, this is done in SANSSingleReductionEventSlice
            reduction_packages = split_reduction_packages_for_event_slice_packages(reduction_packages)
    return event_slice_optimisation, reduction_packages


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
    :param plot_results: bool. Whether or not workspaces should be plotted as they are reduced. Currently only works
                         with event slice compatibility
    :param output_graph: The graph object for plotting workspaces.
    :param save_can: bool. whether or not to save out can workspaces
    """
    # ------------------------------------------------------------------------------------------------------------------
    # Load the data
    # ------------------------------------------------------------------------------------------------------------------
    workspace_to_name = {
        SANSDataType.SAMPLE_SCATTER: "SampleScatterWorkspace",
        SANSDataType.SAMPLE_TRANSMISSION: "SampleTransmissionWorkspace",
        SANSDataType.SAMPLE_DIRECT: "SampleDirectWorkspace",
        SANSDataType.CAN_SCATTER: "CanScatterWorkspace",
        SANSDataType.CAN_TRANSMISSION: "CanTransmissionWorkspace",
        SANSDataType.CAN_DIRECT: "CanDirectWorkspace",
    }

    workspace_to_monitor = {
        SANSDataType.SAMPLE_SCATTER: "SampleScatterMonitorWorkspace",
        SANSDataType.CAN_SCATTER: "CanScatterMonitorWorkspace",
    }

    workspaces, monitors = provide_loaded_data(state, use_optimizations, workspace_to_name, workspace_to_monitor)

    # ------------------------------------------------------------------------------------------------------------------
    # Get reduction settings
    # Split into individual bundles which can be reduced individually. We split here if we have multiple periods.
    # ------------------------------------------------------------------------------------------------------------------
    reduction_packages = get_reduction_packages(state, workspaces, monitors)
    split_for_event_slices = reduction_packages_require_splitting_for_event_slices(reduction_packages)

    event_slice_optimisation, reduction_packages = select_reduction_alg(
        split_for_event_slices,
        state.compatibility.use_compatibility_mode,
        state.compatibility.use_event_slice_optimisation,
        reduction_packages,
    )

    scaled_background_ws = None
    # ------------------------------------------------------------------------------------------------------------------
    # Run reductions (one at a time)
    # ------------------------------------------------------------------------------------------------------------------
    single_reduction_name = "SANSSingleReduction"
    single_reduction_options = {"UseOptimizations": use_optimizations, "SaveCan": save_can}
    alg_version = 2 if event_slice_optimisation else 1
    reduction_alg = create_managed_non_child_algorithm(single_reduction_name, version=alg_version, **single_reduction_options)
    reduction_alg.setChild(False)
    reduction_alg.setAlwaysStoreInADS(False)
    # Perform the data reduction
    for reduction_package in reduction_packages:
        # -----------------------------------
        # Set the properties on the algorithm
        # -----------------------------------
        set_properties_for_reduction_algorithm(
            reduction_alg, reduction_package, workspace_to_name, workspace_to_monitor, event_slice_optimisation=event_slice_optimisation
        )

        # -----------------------------------
        #  Run the reduction
        # -----------------------------------
        reduction_alg.execute()

        # -----------------------------------
        # Get the output of the algorithm
        # -----------------------------------
        _get_ws_from_alg(reduction_alg, reduction_package)

        set_alg_output_names(reduction_package, event_slice_optimisation)
        out_scale_factor, out_shift_factor = get_shift_and_scale_factors_from_algorithm(reduction_alg, event_slice_optimisation)
        reduction_package.out_scale_factor = out_scale_factor
        reduction_package.out_shift_factor = out_shift_factor

        # ---------------------------------------
        # Subtract the background from the slice.
        # ---------------------------------------
        if state.background_subtraction.workspace or state.background_subtraction.scale_factor:
            if not scaled_background_ws:
                scaled_background_ws = create_scaled_background_workspace(state, reduction_package)
            reduction_package.reduced_bgsub, reduction_package.reduced_bgsub_name = subtract_scaled_background(
                reduction_package, scaled_background_ws
            )

        if not event_slice_optimisation and plot_results:
            # Plot results is intended to show the result of each workspace/slice as it is reduced
            # as we reduce in bulk, it is not possible to plot live results while in event_slice mode
            plot_workspace(reduction_package, output_graph)
        # -----------------------------------
        # The workspaces are already on the ADS, but should potentially be grouped
        # -----------------------------------
        group_workspaces_if_required(reduction_package, output_mode, save_can, event_slice_optimisation=event_slice_optimisation)

    data = state.data
    additional_run_numbers = {
        "SampleTransmissionRunNumber": "" if data.sample_transmission is None else str(data.sample_transmission),
        "SampleDirectRunNumber": "" if data.sample_direct is None else str(data.sample_direct),
        "CanScatterRunNumber": "" if data.can_scatter is None else str(data.can_scatter),
        "CanDirectRunNumber": "" if data.can_direct is None else str(data.can_direct),
    }
    additional_metadata = {}

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
    if output_mode is OutputMode.SAVE_TO_FILE:
        save_to_file(
            reduction_packages, save_can, additional_run_numbers, additional_metadata, event_slice_optimisation=event_slice_optimisation
        )
        delete_reduced_workspaces(reduction_packages)
    elif output_mode is OutputMode.BOTH:
        save_to_file(
            reduction_packages, save_can, additional_run_numbers, additional_metadata, event_slice_optimisation=event_slice_optimisation
        )

    # -----------------------------------------------------------------------
    # Clean up other workspaces if the optimizations have not been turned on.
    # -----------------------------------------------------------------------
    if not use_optimizations:
        delete_optimization_workspaces(reduction_packages, workspaces, monitors, save_can)

    if scaled_background_ws:
        delete_workspace_by_name(scaled_background_ws)

    out_scale_factors = []
    out_shift_factors = []
    for reduction_package in reduction_packages:
        out_scale_factors.extend(reduction_package.out_scale_factor)
        out_shift_factors.extend(reduction_package.out_shift_factor)

    return out_scale_factors, out_shift_factors


def _get_ws_from_alg(reduction_alg, reduction_package):
    reduction_package.reduced_lab = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceLAB")
    reduction_package.reduced_hab = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceHAB")
    reduction_package.reduced_merged = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceMerged")
    reduction_package.reduced_lab_can = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceLABCan")
    reduction_package.reduced_lab_can_count = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceLABCanCount")
    reduction_package.reduced_lab_can_norm = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceLABCanNorm")
    reduction_package.reduced_hab_can = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceHABCan")
    reduction_package.reduced_hab_can_count = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceHABCanCount")
    reduction_package.reduced_hab_can_norm = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceHABCanNorm")
    reduction_package.calculated_transmission = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceCalculatedTransmission")
    reduction_package.unfitted_transmission = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceUnfittedTransmission")
    reduction_package.calculated_transmission_can = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceCalculatedTransmissionCan")
    reduction_package.unfitted_transmission_can = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceUnfittedTransmissionCan")
    reduction_package.reduced_lab_sample = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceLABSample")
    reduction_package.reduced_hab_sample = get_workspace_from_algorithm(reduction_alg, "OutputWorkspaceHABSample")


def load_workspaces_from_states(state):
    workspace_to_name = {
        SANSDataType.SAMPLE_SCATTER: "SampleScatterWorkspace",
        SANSDataType.SAMPLE_TRANSMISSION: "SampleTransmissionWorkspace",
        SANSDataType.SAMPLE_DIRECT: "SampleDirectWorkspace",
        SANSDataType.CAN_SCATTER: "CanScatterWorkspace",
        SANSDataType.CAN_TRANSMISSION: "CanTransmissionWorkspace",
        SANSDataType.CAN_DIRECT: "CanDirectWorkspace",
    }

    workspace_to_monitor = {
        SANSDataType.SAMPLE_SCATTER: "SampleScatterMonitorWorkspace",
        SANSDataType.CAN_SCATTER: "CanScatterMonitorWorkspace",
    }

    workspaces, monitors = provide_loaded_data(state, True, workspace_to_name, workspace_to_monitor)


# ----------------------------------------------------------------------------------------------------------------------
# Function for plotting
# ----------------------------------------------------------------------------------------------------------------------
def plot_workspace(reduction_package, output_graph):
    """
    Plotting continuous output. Decides on the backend to use based on availability

    :param reduction_package: An object containing the reduced workspaces
    :param output_graph: Name to the plot window
    """
    plotting_module = get_plotting_module()
    plot_workspace_mantidqt(reduction_package, output_graph, plotting_module)


def plot_workspace_mantidqt(reduction_package, output_graph, plotting_module):
    """
    Continuous plotting using a matplotlib backend.

    :param reduction_package: An object containing the reduced workspaces
    :param output_graph: A matplotlib fig
    :param plotting_module: The mantidqt plotting module
    """
    plot = plotting_module.plot

    plot_kwargs = {"scalex": True, "scaley": True}
    ax_options = {"xscale": "linear", "yscale": "linear"}

    workspaces_to_plot = []

    if reduction_package.reduction_mode == ReductionMode.ALL:
        workspaces_to_plot = [reduction_package.reduced_hab, reduction_package.reduced_lab]
    elif reduction_package.reduction_mode == ReductionMode.HAB:
        workspaces_to_plot = [reduction_package.reduced_hab]
    elif reduction_package.reduction_mode == ReductionMode.LAB:
        workspaces_to_plot = [reduction_package.reduced_lab]
    elif reduction_package.reduction_mode == ReductionMode.MERGED:
        workspaces_to_plot = [reduction_package.reduced_merged, reduction_package.reduced_hab, reduction_package.reduced_lab]
    if reduction_package.reduced_bgsub:
        workspaces_to_plot.extend(reduction_package.reduced_bgsub)

    plot(
        workspaces_to_plot,
        wksp_indices=[0],
        overplot=True,
        fig=output_graph,
        plot_kwargs=plot_kwargs,
        ax_properties=ax_options,
    )


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


def set_output_workspace_on_load_algorithm_for_one_workspace_type(
    load_options, load_workspace_name, file_name, period, is_transmission, file_info_factory, load_monitor_name=None
):
    file_info = file_info_factory.create_sans_file_information(file_name)
    workspace_names = get_expected_workspace_names(file_info, is_transmission=is_transmission, period=period, get_base_name_only=True)
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
    set_output_workspace_on_load_algorithm_for_one_workspace_type(
        load_options=load_options,
        load_workspace_name="SampleScatterWorkspace",
        file_name=data.sample_scatter,
        period=data.sample_scatter_period,
        is_transmission=False,
        file_info_factory=file_information_factory,
        load_monitor_name="SampleScatterMonitorWorkspace",
    )

    # SampleTransmission
    sample_transmission = data.sample_transmission
    if sample_transmission:
        set_output_workspace_on_load_algorithm_for_one_workspace_type(
            load_options=load_options,
            load_workspace_name="SampleTransmissionWorkspace",
            file_name=sample_transmission,
            period=data.sample_transmission_period,
            is_transmission=True,
            file_info_factory=file_information_factory,
        )
    # SampleDirect
    sample_direct = data.sample_direct
    if sample_direct:
        set_output_workspace_on_load_algorithm_for_one_workspace_type(
            load_options=load_options,
            load_workspace_name="SampleDirectWorkspace",
            file_name=sample_direct,
            period=data.sample_direct_period,
            is_transmission=True,
            file_info_factory=file_information_factory,
        )

    # CanScatter + CanMonitor
    can_scatter = data.can_scatter
    if can_scatter:
        set_output_workspace_on_load_algorithm_for_one_workspace_type(
            load_options=load_options,
            load_workspace_name="CanScatterWorkspace",
            file_name=can_scatter,
            period=data.can_scatter_period,
            is_transmission=False,
            file_info_factory=file_information_factory,
            load_monitor_name="CanScatterMonitorWorkspace",
        )

    # CanTransmission
    can_transmission = data.can_transmission
    if can_transmission:
        set_output_workspace_on_load_algorithm_for_one_workspace_type(
            load_options=load_options,
            load_workspace_name="CanTransmissionWorkspace",
            file_name=can_transmission,
            period=data.can_transmission_period,
            is_transmission=True,
            file_info_factory=file_information_factory,
        )
    # CanDirect
    can_direct = data.can_direct
    if can_direct:
        set_output_workspace_on_load_algorithm_for_one_workspace_type(
            load_options=load_options,
            load_workspace_name="CanDirectWorkspace",
            file_name=can_direct,
            period=data.can_direct_period,
            is_transmission=True,
            file_info_factory=file_information_factory,
        )


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
    state_serialized = Serializer.to_json(state)
    load_name = "SANSLoad"
    load_options = {"SANSState": state_serialized, "PublishToCache": use_optimizations, "UseCached": use_optimizations}

    # Set the output workspaces

    set_output_workspaces_on_load_algorithm(load_options, state)

    load_alg = create_managed_non_child_algorithm(load_name, **load_options)
    load_alg.execute()

    # Retrieve the data
    workspace_to_count = {
        SANSDataType.SAMPLE_SCATTER: "NumberOfSampleScatterWorkspaces",
        SANSDataType.SAMPLE_TRANSMISSION: "NumberOfSampleTransmissionWorkspaces",
        SANSDataType.SAMPLE_DIRECT: "NumberOfSampleDirectWorkspaces",
        SANSDataType.CAN_SCATTER: "NumberOfCanScatterWorkspaces",
        SANSDataType.CAN_TRANSMISSION: "NumberOfCanTransmissionWorkspaces",
        SANSDataType.CAN_DIRECT: "NumberOfCanDirectWorkspaces",
    }

    workspaces = get_workspaces_from_load_algorithm(load_alg, workspace_to_count, workspace_to_name)
    monitors = get_workspaces_from_load_algorithm(load_alg, workspace_to_count, workspace_to_monitor)

    for key, workspace_type in workspaces.items():
        for workspace in workspace_type:
            add_to_group(workspace, "sans_interface_raw_data")
    for key, monitor_workspace_type in monitors.items():
        for monitor_workspace in monitor_workspace_type:
            add_to_group(monitor_workspace, "sans_interface_raw_data")
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
            workspaces = get_multi_period_workspaces(load_alg, workspace_name_dict[workspace_type], number_of_workspaces)
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
    group_options = {"InputWorkspaces": workspace_names, "OutputWorkspace": base_name}
    group_alg = create_unmanaged_algorithm(group_name, **group_options)
    group_alg.setChild(False)
    group_alg.execute()
    return workspaces


def check_for_background_workspace_in_ads(state, reduction_package):
    background_ws_name = state.background_subtraction.workspace
    if AnalysisDataService.doesExist(background_ws_name):
        return background_ws_name
    # Look for a workspace that has a reduction suffix appended (such as _merged_1D_2.2_10.0)
    reduced_name = ""
    if reduction_package.reduction_mode == ReductionMode.MERGED:
        reduced_name = reduction_package.reduced_merged_name[0]
    elif reduction_package.reduction_mode == ReductionMode.HAB:
        reduced_name = reduction_package.reduced_hab_name[0]
    elif reduction_package.reduction_mode == ReductionMode.LAB:
        reduced_name = reduction_package.reduced_lab_name[0]
    full_name = (
        background_ws_name
        + reduced_name.split(
            state.save.user_specified_output_name if state.save.user_specified_output_name else str(state.data.sample_scatter_run_number),
            1,
        )[-1]
    )

    if AnalysisDataService.doesExist(full_name):
        reduction_package.state.background_subtraction.workspace = full_name
        return full_name
    else:
        raise ValueError(f"BackgroundWorkspace: The workspace '{background_ws_name}' or '{full_name}' could not be found in the ADS.")


def create_scaled_background_workspace(state, reduction_package) -> str:
    state.background_subtraction.validate()
    if reduction_package.reduction_mode == ReductionMode.ALL:
        raise ValueError(
            f"Reduction Mode '{ReductionMode.ALL}' is incompatible with scaled background reduction. The "
            f"ReductionMode must be set to '{ReductionMode.MERGED}', '{ReductionMode.HAB}', or '{ReductionMode.LAB}'."
        )

    background_ws_name = check_for_background_workspace_in_ads(state, reduction_package)

    scaled_bg_ws_name = "__" + state.background_subtraction.workspace + "_scaled"  # __ makes the ws invisible

    scale_name = "Scale"
    scale_options = {
        "InputWorkspace": background_ws_name,
        "Factor": state.background_subtraction.scale_factor,
        "OutputWorkspace": scaled_bg_ws_name,
    }
    scale_alg = create_unmanaged_algorithm(scale_name, **scale_options)
    scale_alg.setAlwaysStoreInADS(True)
    scale_alg.execute()
    return scaled_bg_ws_name


# ----------------------------------------------------------------------------------------------------------------------
# Functions for reduction packages
# ----------------------------------------------------------------------------------------------------------------------
def get_reduction_packages(state, workspaces, monitors):
    """
    This function creates a set of reduction packages which contain the necessary state for a single reduction
    as well as the required workspaces.

    There is one workflow where a state can (and should) split up:
    Event slices were specified. We event slice after initial reduction has taken place, as some operations can
            be performed before event slicing. We do this for more efficient reduction, as we are not performing the
            same operations multiple times needlessly.

    :param state: A single state which potentially needs to be split up into several states
    :param workspaces: The workspaces contributing to the reduction
    :param monitors: The monitors contributing to the reduction
    :return: A set of "Reduction packages" where each reduction package defines a single reduction.
    """
    reduction_packages = create_initial_reduction_packages(state, workspaces, monitors)
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
        is_wav_range = reduction_package.is_part_of_wavelength_range_reduction

        for state in states:
            new_state = deepcopy(state)
            new_reduction_package = ReductionPackage(
                state=new_state,
                workspaces=workspaces,
                monitors=monitors,
                is_part_of_wavelength_range_reduction=is_wav_range,
                is_part_of_multi_period_reduction=is_part_of_multi_period_reduction,
                is_part_of_event_slice_reduction=True,
            )
            reduction_packages_split.append(new_reduction_package)
    return reduction_packages_split


def create_initial_reduction_packages(state, workspaces, monitors):
    """
    This provides the initial split of the workspaces.

    If the data stems from multi-period data, then we need to split up the workspaces. The state object is valid
    for each one of these workspaces. Hence, we need to create a deep copy of them for each reduction package.

    The way multi-period files are handled over the different workspaces input types is:
    1. The sample scatter period determines all other periods, i.e. if the sample scatter workspace is has only
       one period, but the sample transmission has two, then only the first period is used.
    2. If the sample scatter period is not available on another workspace type, then the last period on that
       workspace type is used.

    For the cases where the periods between the different workspaces types does not match, an information is logged.

    :param state: A single state which potentially needs to be split up into several states
    :param workspaces: The workspaces contributing to the reduction
    :param monitors: The monitors contributing to the reduction
    :return: A set of "Reduction packages" where each reduction package defines a single reduction.
    """
    # For loaded period we create a package
    packages = []

    data_info = state.data
    sample_scatter_period = data_info.sample_scatter_period
    requires_new_period_selection = len(workspaces[SANSDataType.SAMPLE_SCATTER]) > 1 and sample_scatter_period == ALL_PERIODS

    is_multi_period = len(workspaces[SANSDataType.SAMPLE_SCATTER]) > 1
    is_multi_wavelength = len(state.wavelength.wavelength_interval.selected_ranges) > 1

    for index in range(0, len(workspaces[SANSDataType.SAMPLE_SCATTER])):
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
        packages.append(
            ReductionPackage(
                state=state_copy,
                workspaces=workspaces_for_package,
                monitors=monitors_for_package,
                is_part_of_wavelength_range_reduction=is_multi_wavelength,
                is_part_of_multi_period_reduction=is_multi_period,
            )
        )
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


def set_properties_for_reduction_algorithm(
    reduction_alg, reduction_package, workspace_to_name, workspace_to_monitor, event_slice_optimisation=False
):
    """
    Sets up everything necessary on the reduction algorithm.

    :param reduction_alg: a handle to the reduction algorithm
    :param reduction_package: a reduction package object
    :param workspace_to_name: the workspace to name map
    :param workspace_to_monitor: a workspace to monitor map
    :param event_slice_optimisation: optional bool. If true then using SANSSingleReductionEventSlice algorithm.
                        In this base, names and base names should not include time slice information.
    """
    # Go through the elements of the reduction package and set them on the reduction algorithm
    # Set the SANSState
    state = reduction_package.state
    state_dict = Serializer.to_json(state)
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
    is_part_of_wavelength_range_reduction = reduction_package.is_part_of_wavelength_range_reduction

    # SANSSingleReduction version 2 only properties
    if event_slice_optimisation:
        # In event slice mode, we can have multiple shift and scale factors for one reduction package
        # there we output these as a workspace containing shifts as X data and scales as Y data.
        reduction_alg.setProperty("OutShiftAndScaleFactor", "ShiftAndScaleFactors")
        # Set properties used to generated names for workspaces within the output workspace groups
        reduction_alg.setProperty("Period", is_part_of_multi_period_reduction)
        reduction_alg.setProperty("WavelengthRange", is_part_of_wavelength_range_reduction)


def set_alg_output_names(reduction_package, event_slice_optimisation):
    ads_instance = AnalysisDataService.Instance()

    is_part_of_multi_period_reduction = reduction_package.is_part_of_multi_period_reduction
    is_part_of_event_slice_reduction = reduction_package.is_part_of_event_slice_reduction
    is_part_of_wavelength_range_reduction = reduction_package.is_part_of_wavelength_range_reduction
    is_group = (
        is_part_of_multi_period_reduction
        or is_part_of_event_slice_reduction
        or is_part_of_wavelength_range_reduction
        or event_slice_optimisation
    )
    multi_reduction_type = {
        "period": is_part_of_multi_period_reduction,
        "event_slice": is_part_of_event_slice_reduction,
        "wavelength_range": is_part_of_wavelength_range_reduction,
    }

    def _set_output_name(
        ws_group,
        _reduction_package,
        _is_group,
        _reduction_mode,
        _attr_out_name,
        _atrr_out_name_base,
        multi_reduction_type,
        _suffix=None,
        transmission=False,
    ):
        setattr(_reduction_package, _attr_out_name, [])
        setattr(_reduction_package, _atrr_out_name_base, [])

        if not ws_group:
            return

        for ws in ws_group:
            if not transmission:
                # Use event_slice from set_properties_for_reduction_algorithm scope
                wav_range = get_wav_range_from_ws(ws)
                _out_name, _out_name_base = get_output_name(
                    _reduction_package.state,
                    _reduction_mode,
                    _is_group,
                    multi_reduction_type=multi_reduction_type,
                    wav_range=wav_range,
                    event_slice_optimisation=event_slice_optimisation,
                )
            else:
                _out_name, _out_name_base = get_transmission_output_name(
                    _reduction_package.state, _reduction_mode, multi_reduction_type=multi_reduction_type
                )

            if _suffix is not None:
                _out_name += _suffix
                _out_name_base += _suffix

            ads_instance.addOrReplace(_out_name, ws)
            getattr(_reduction_package, _attr_out_name).append(_out_name)
            getattr(_reduction_package, _atrr_out_name_base).append(_out_name_base)

    def _set_custom_output_name(
        ws, reduction_package, workspace_names, workspace_name_base, package_attribute_name, package_attribute_name_base
    ):
        if not ws:
            setattr(reduction_package, package_attribute_name, None)
            setattr(reduction_package, package_attribute_name_base, None)
            return

        if isinstance(ws, Workspace2D):
            setattr(reduction_package, package_attribute_name, workspace_names)
            setattr(reduction_package, package_attribute_name_base, workspace_name_base)
            ads_instance.addOrReplace(workspace_names, ws)
        else:
            setattr(reduction_package, package_attribute_name, [])
            setattr(reduction_package, package_attribute_name_base, workspace_name_base)
            assert isinstance(ws, WorkspaceGroup), f"Expected Ws2D or WsGroup, got {repr(ws)} instead"
            for i in range(ws.size()):
                ws_i = ws.getItem(i)
                ads_instance.addOrReplace(workspace_names[i], ws_i)
                getattr(reduction_package, package_attribute_name).append(workspace_names[i])

    def _set_lab(_reduction_package, _is_group):
        _set_output_name(
            _reduction_package.reduced_lab_can,
            _reduction_package,
            _is_group,
            ReductionMode.LAB,
            "reduced_lab_can_name",
            "reduced_lab_can_base_name",
            multi_reduction_type,
            LAB_CAN_SUFFIX,
        )

        # Lab Can Count workspace - this is a partial workspace
        _set_output_name(
            _reduction_package.reduced_lab_can_count,
            _reduction_package,
            _is_group,
            ReductionMode.LAB,
            "reduced_lab_can_count_name",
            "reduced_lab_can_count_base_name",
            multi_reduction_type,
            LAB_CAN_COUNT_SUFFIX,
        )

        # Lab Can Norm workspace - this is a partial workspace
        _set_output_name(
            _reduction_package.reduced_lab_can_norm,
            _reduction_package,
            _is_group,
            ReductionMode.LAB,
            "reduced_lab_can_norm_name",
            "reduced_lab_can_norm_base_name",
            multi_reduction_type,
            LAB_CAN_NORM_SUFFIX,
        )

        _set_output_name(
            _reduction_package.reduced_lab_sample,
            _reduction_package,
            _is_group,
            ReductionMode.LAB,
            "reduced_lab_sample_name",
            "reduced_lab_sample_base_name",
            multi_reduction_type,
            LAB_SAMPLE_SUFFIX,
        )

    def _set_hab(_reduction_package, _is_group):
        # Hab Can Workspace
        _set_output_name(
            _reduction_package.reduced_hab_can,
            _reduction_package,
            _is_group,
            ReductionMode.HAB,
            "reduced_hab_can_name",
            "reduced_hab_can_base_name",
            multi_reduction_type,
            HAB_CAN_SUFFIX,
        )

        # Hab Can Count workspace - this is a partial workspace
        _set_output_name(
            _reduction_package.reduced_hab_can_count,
            _reduction_package,
            _is_group,
            ReductionMode.HAB,
            "reduced_hab_can_count_name",
            "reduced_hab_can_count_base_name",
            multi_reduction_type,
            HAB_CAN_COUNT_SUFFIX,
        )

        # Hab Can Norm workspace - this is a partial workspace
        _set_output_name(
            _reduction_package.reduced_hab_can_norm,
            _reduction_package,
            _is_group,
            ReductionMode.HAB,
            "reduced_hab_can_norm_name",
            "reduced_hab_can_norm_base_name",
            multi_reduction_type,
            HAB_CAN_NORM_SUFFIX,
        )

        _set_output_name(
            _reduction_package.reduced_hab_sample,
            _reduction_package,
            _is_group,
            ReductionMode.HAB,
            "reduced_hab_sample_name",
            "reduced_hab_sample_base_name",
            multi_reduction_type,
            HAB_SAMPLE_SUFFIX,
        )

    # Set the output workspaces for the can reductions too -- note that these will only be set if optimizations
    # are enabled
    reduction_mode = reduction_package.reduction_mode
    if reduction_mode in (ReductionMode.MERGED, ReductionMode.LAB, ReductionMode.ALL):
        _set_output_name(
            reduction_package.reduced_lab,
            reduction_package,
            is_group,
            ReductionMode.LAB,
            "reduced_lab_name",
            "reduced_lab_base_name",
            multi_reduction_type,
        )
        _set_lab(reduction_package, is_group)

    if reduction_mode in (ReductionMode.MERGED, ReductionMode.HAB, ReductionMode.ALL):
        _set_output_name(
            reduction_package.reduced_hab,
            reduction_package,
            is_group,
            ReductionMode.HAB,
            "reduced_hab_name",
            "reduced_hab_base_name",
            multi_reduction_type,
        )
        _set_hab(reduction_package, is_group)

    if reduction_mode is ReductionMode.MERGED:
        _set_output_name(
            reduction_package.reduced_merged,
            reduction_package,
            is_group,
            ReductionMode.MERGED,
            "reduced_merged_name",
            "reduced_merged_base_name",
            multi_reduction_type,
        )

    # ------------------------------------------------------------------------------------------------------------------
    # Set the output workspaces for the calculated and unfitted transmission
    # ------------------------------------------------------------------------------------------------------------------
    def get_transmission_names(_trans_ws_group, data_type, is_fitted):
        output_names = []
        base_name = None

        if not _trans_ws_group:
            return output_names, base_name

        for ws in _trans_ws_group:
            range_str = ws.getRun().getProperty("Wavelength Range").valueAsStr
            wav_range = range_str.split("-")
            trans_name, base_name = get_transmission_output_name(
                reduction_package.state, wav_range, data_type, multi_reduction_type, is_fitted
            )
            output_names.append(trans_name)
        return output_names, base_name

    sample_calculated_transmission, sample_calculated_transmission_base = get_transmission_names(
        reduction_package.calculated_transmission, DataType.SAMPLE, True
    )
    can_calculated_transmission, can_calculated_transmission_base = get_transmission_names(
        reduction_package.calculated_transmission_can, DataType.CAN, True
    )
    sample_unfitted_transmission, sample_unfitted_transmission_base = get_transmission_names(
        reduction_package.unfitted_transmission, DataType.SAMPLE, False
    )
    can_unfitted_transmission, can_unfitted_transmission_base = get_transmission_names(
        reduction_package.unfitted_transmission_can, DataType.CAN, False
    )

    _set_custom_output_name(
        reduction_package.calculated_transmission,
        reduction_package,
        sample_calculated_transmission,
        sample_calculated_transmission_base,
        "calculated_transmission_name",
        "calculated_transmission_base_name",
    )

    _set_custom_output_name(
        reduction_package.unfitted_transmission,
        reduction_package,
        sample_unfitted_transmission,
        sample_unfitted_transmission_base,
        "unfitted_transmission_name",
        "unfitted_transmission_base_name",
    )

    _set_custom_output_name(
        reduction_package.calculated_transmission_can,
        reduction_package,
        can_calculated_transmission,
        can_calculated_transmission_base,
        "calculated_transmission_can_name",
        "calculated_transmission_can_base_name",
    )

    _set_custom_output_name(
        reduction_package.unfitted_transmission_can,
        reduction_package,
        can_unfitted_transmission,
        can_unfitted_transmission_base,
        "unfitted_transmission_can_name",
        "unfitted_transmission_can_base_name",
    )


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
        # Try to return by pointer
        return alg.getProperty(output_property_name).value

    if AnalysisDataService.doesExist(output_workspace_name):
        ws = AnalysisDataService.retrieve(output_workspace_name)
        return ws
    else:
        return None


def get_shift_and_scale_factors_from_algorithm(alg, event_slice_optimisation):
    """
    Retrieve the shift and scale factors from the algorithm. In event slice mode there can be multiple shift
    and scale factors. These are output as a workspace containing scale and shift as X, Y data, respectively.
    :param alg: The SingleReduction algorithm
    :param event_slice_optimisation: bool. If true, then version 2 has been run, otherwise v1.
    :return: a list of shift factors, a list of scale factors
    """
    if event_slice_optimisation:
        factors_workspace = get_workspace_from_algorithm(alg, "OutShiftAndScaleFactor")
        if factors_workspace is None:
            return [], []
        else:
            scales = factors_workspace.readX(0)
            shifts = factors_workspace.readY(0)
            delete_alg = create_unmanaged_algorithm("DeleteWorkspace", **{"Workspace": "ShiftAndScaleFactors"})
            delete_alg.execute()
            return scales, shifts
    else:
        return [alg.getProperty("OutScaleFactor").value], [alg.getProperty("OutShiftFactor").value]


# ----------------------------------------------------------------------------------------------------------------------
# Functions for outputs to the ADS and saving the file
# ----------------------------------------------------------------------------------------------------------------------
def group_workspaces_if_required(reduction_package, output_mode, save_can, event_slice_optimisation=False):
    """
    The output workspaces have already been published to the ADS by the algorithm. Now we might have to
    bundle them into a group if:
    * They are part of a multi-period workspace or a sliced reduction
    * They are reduced LAB and HAB workspaces of a Merged reduction
    * They are can workspaces - they are all grouped into a single group
    :param reduction_package: a list of reduction packages
    :param output_mode: one of OutputMode. SaveToFile, PublishToADS, Both.
    :param save_can: a bool. If true save out can and sample workspaces.
    :param event_slice_optimisation: an optional bool. If true group_workspaces is being called on event sliced data, so the
                        reduction_package contains grouped workspaces.
    """
    is_part_of_multi_period_reduction = reduction_package.is_part_of_multi_period_reduction
    is_part_of_event_slice_reduction = reduction_package.is_part_of_event_slice_reduction or event_slice_optimisation
    is_part_of_wavelength_range_reduction = reduction_package.is_part_of_wavelength_range_reduction
    requires_grouping = is_part_of_multi_period_reduction or is_part_of_event_slice_reduction or is_part_of_wavelength_range_reduction

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

    if requires_grouping:
        group_bgsub_if_required(reduction_package)

    # Can group workspace depends on if save_can is checked and output_mode
    # Logic table for which group to save CAN into
    # CAN | FILE | In OPTIMIZATION group
    # ----------------------------------
    #  Y  |   Y  | YES
    #  N  |   Y  | YES
    #  Y  |   N  | NO
    #  N  |   N  | YES

    if save_can and output_mode is not OutputMode.SAVE_TO_FILE:
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
    add_to_group(reduction_package.calculated_transmission_can, reduction_package.calculated_transmission_can_base_name)
    add_to_group(reduction_package.unfitted_transmission, reduction_package.unfitted_transmission_base_name)
    add_to_group(reduction_package.unfitted_transmission_can, reduction_package.unfitted_transmission_can_base_name)


def group_bgsub_if_required(reduction_package):
    reduced_bgsub = reduction_package.reduced_bgsub
    is_bgsub_reduction = reduced_bgsub is not None

    if is_bgsub_reduction:
        base_names = []
        if reduction_package.reduction_mode == ReductionMode.HAB:
            base_names = reduction_package.reduced_lab_base_name
        if reduction_package.reduction_mode == ReductionMode.LAB:
            base_names = reduction_package.reduced_lab_base_name
        if reduction_package.reduction_mode == ReductionMode.MERGED:
            base_names = reduction_package.reduced_merged_base_name
        for ws, base_name in zip(reduced_bgsub, base_names):
            add_to_group(ws, base_name)


def add_to_group(workspace, name_of_group_workspace):
    """
    Creates a group workspace with the base name for the workspace

    :param workspace: the workspace to add to the WorkspaceGroup. This can be a group workspace
    :param name_of_group_workspace: the name of the WorkspaceGroup, or a list relating to a ws group
    """
    if workspace is None:
        return

    ads_inst = AnalysisDataService.Instance()

    def _add_single_ws_to_group(_workspace, _group_ws_name):
        name_of_workspace = _workspace.name()
        if ads_inst.doesExist(_group_ws_name):
            if name_of_workspace not in ads_inst.retrieve(_group_ws_name).getNames():
                ads_inst.addToGroup(_group_ws_name, name_of_workspace)
        else:
            make_group_from_workspace(name_of_workspace, _group_ws_name)

    if isinstance(name_of_group_workspace, list):
        for i, ws_name in enumerate(name_of_group_workspace):
            _add_single_ws_to_group(workspace.getItem(i), ws_name)
    elif isinstance(workspace, WorkspaceGroup):
        for i in range(workspace.size()):
            _add_single_ws_to_group(workspace.getItem(i), name_of_group_workspace)
    else:
        _add_single_ws_to_group(workspace, name_of_group_workspace)


def add_group_to_group(group_workspace, name_of_target_group_workspace):
    """
    Adds a group workspace to an existing group workspace.
    This is used when using SANSSingleEventSlice algorithm, which returns group workspaces
    containing the workspaces from each time slice.
    :param group_workspace: str. The group workspace to merge in
    :param name_of_target_group_workspace: str. The name of the group workspace we want as output
    :return:
    """
    group_name = "GroupWorkspaces"
    AnalysisDataService.Instance().add("__tmp_grp", group_workspace)

    group_options = {"InputWorkspaces": [name_of_target_group_workspace, "__tmp_grp"], "OutputWorkspace": name_of_target_group_workspace}
    group_alg = create_unmanaged_algorithm(group_name, **group_options)
    group_alg.setAlwaysStoreInADS(True)
    group_alg.execute()


def make_group_from_workspace(name_of_workspace, name_of_group_workspace):
    """
    Group a workspace into a group workspace which does not yet exist on the ADS
    :param name_of_workspace: name of workspace to put into a group
    :param name_of_group_workspace: name of group workspace to create
    """
    group_name = "GroupWorkspaces"
    group_options = {"InputWorkspaces": [name_of_workspace], "OutputWorkspace": name_of_group_workspace}
    group_alg = create_unmanaged_algorithm(group_name, **group_options)

    group_alg.setAlwaysStoreInADS(True)
    group_alg.execute()


def rename_group_workspace(name_of_workspace, name_of_group_workspace):
    """
    Rename a group workspace
    :param name_of_workspace: current name of group workspace
    :param name_of_group_workspace: target name of group workspace
    """
    rename_name = "RenameWorkspace"
    rename_options = {"InputWorkspace": name_of_workspace, "OutputWorkspace": name_of_group_workspace}
    rename_alg = create_unmanaged_algorithm(rename_name, **rename_options)
    rename_alg.setAlwaysStoreInADS(True)
    rename_alg.execute()


def save_to_file(
    reduction_packages: list,
    save_can: bool,
    additional_run_numbers: Dict[str, str],
    additional_metadata: Dict[str, Any],
    event_slice_optimisation: bool = False,
):
    """
    Extracts all workspace names which need to be saved and saves them into a file.

    :param reduction_packages: a list of reduction packages which contain all the relevant information for saving
    :param save_can: When true save the unsubtracted can and sample workspaces
    :param additional_run_numbers: Workspace type to run number
    :param additional_metadata: Dict of reduction metadata to be included in saved output.
    :param event_slice_optimisation: optional. If true then reduction packages contain event slice data
    """
    if not event_slice_optimisation:
        workspaces_names_to_save = get_all_names_to_save(reduction_packages, save_can=save_can)
    else:
        workspaces_names_to_save = get_event_slice_names_to_save(reduction_packages, save_can=save_can)

    state = reduction_packages[0].state
    save_info = state.save
    scaled_bg_info = state.background_subtraction
    file_formats = save_info.file_format
    for to_save in workspaces_names_to_save:
        if isinstance(to_save, tuple):
            for i, ws_name_to_save in enumerate(to_save[0]):
                transmission = to_save[1][i] if to_save[1] else ""
                transmission_can = to_save[2][i] if to_save[2] else ""
                _add_scaled_background_metadata_if_relevant(scaled_bg_info, ws_name_to_save, additional_metadata)
                save_workspace_to_file(
                    ws_name_to_save,
                    file_formats,
                    ws_name_to_save,
                    additional_run_numbers,
                    additional_metadata,
                    transmission,
                    transmission_can,
                )
        else:
            _add_scaled_background_metadata_if_relevant(scaled_bg_info, str(to_save), additional_metadata)
            save_workspace_to_file(to_save, file_formats, to_save, additional_run_numbers, additional_metadata)


def _add_scaled_background_metadata_if_relevant(bg_state, ws_name: str, metadata: dict[str, Any]):
    if not ws_name.endswith(SCALED_BGSUB_SUFFIX):
        return
    metadata["BackgroundSubtractionWorkspace"] = str(bg_state.workspace)
    metadata["BackgroundSubtractionScaleFactor"] = float(bg_state.scale_factor)


def delete_reduced_workspaces(reduction_packages, include_non_transmission=True):
    """
    Deletes all workspaces which would have been generated from a list of reduction packages.

    :param reduction_packages: a list of reduction package
    :param include_non_transmission: an optional bool. If true then also delete reduced hab, lab, merged
    """

    def _delete_workspaces(_delete_alg, _workspaces):
        for _workspace in _workspaces:
            if _workspace is not None:
                _delete_alg.setProperty("Workspace", _workspace)
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

        workspaces_to_delete = [calculated_transmission, unfitted_transmission, calculated_transmission_can, unfitted_transmission_can]

        if include_non_transmission:
            reduced_lab = reduction_package.reduced_lab
            reduced_hab = reduction_package.reduced_hab
            reduced_merged = reduction_package.reduced_merged
            reduced_bgsub = reduction_package.reduced_bgsub_name

            # Remove samples
            reduced_lab_sample = reduction_package.reduced_lab_sample
            reduced_hab_sample = reduction_package.reduced_hab_sample

            workspaces_to_delete.extend([reduced_lab, reduced_hab, reduced_merged, reduced_lab_sample, reduced_hab_sample])
            if reduced_bgsub is not None:
                workspaces_to_delete.extend(reduced_bgsub)

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
        optimizations_to_delete = [
            reduction_package.reduced_lab_can_count,
            reduction_package.reduced_lab_can_norm,
            reduction_package.reduced_hab_can_count,
            reduction_package.reduced_hab_can_norm,
        ]
        if not save_can:
            optimizations_to_delete.extend([reduction_package.reduced_lab_can, reduction_package.reduced_hab_can])
        _delete_workspaces(delete_alg, optimizations_to_delete)


def delete_workspace_by_name(ws_name):
    delete_name = "DeleteWorkspace"
    delete_options = {}
    delete_alg = create_unmanaged_algorithm(delete_name, **delete_options)
    if ws_name and AnalysisDataService.doesExist(ws_name):
        delete_alg.setProperty("Workspace", ws_name)
        delete_alg.execute()


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
        ws_names = reduction_package.unfitted_transmission_can_name
    else:
        base_name = reduction_package.unfitted_transmission_base_name
        ws_names = reduction_package.unfitted_transmission_name
    if base_name in (None, "") or ws_names in (None, "", []):
        return []

    ws_in_ads = []

    if AnalysisDataService.doesExist(base_name):
        group = AnalysisDataService.retrieve(base_name)
        for ws_name in ws_names:
            if group.contains(ws_name):
                ws_in_ads.append(ws_name)

    return ws_in_ads


def get_all_names_to_save(reduction_packages, save_can):
    """
    Extracts all the output names from a list of reduction packages.

    :param reduction_packages: a list of reduction packages
    :param save_can: a bool, whether or not to save unsubtracted can workspace
    :return: a list of workspace names to save.
    """

    def get_ws_names_from_group(ws_group):
        return [ws.name() for ws in ws_group]

    names_to_save = []
    for reduction_package in reduction_packages:
        reduced_lab = reduction_package.reduced_lab
        reduced_hab = reduction_package.reduced_hab
        reduced_merged = reduction_package.reduced_merged
        reduced_lab_can = reduction_package.reduced_lab_can
        reduced_hab_can = reduction_package.reduced_hab_can
        reduced_lab_sample = reduction_package.reduced_lab_sample
        reduced_hab_sample = reduction_package.reduced_hab_sample

        reduced_bgsub = reduction_package.reduced_bgsub_name

        trans_name = get_transmission_names_to_save(reduction_package, False)
        trans_can_name = get_transmission_names_to_save(reduction_package, True)

        if save_can:
            if reduced_merged:
                names_to_save.append((get_ws_names_from_group(reduced_merged), trans_name, trans_can_name))
            if reduced_lab:
                names_to_save.append((get_ws_names_from_group(reduced_lab), trans_name, trans_can_name))
            if reduced_hab:
                names_to_save.append((get_ws_names_from_group(reduced_hab), trans_name, trans_can_name))
            if reduced_bgsub:
                names_to_save.append((reduced_bgsub, trans_name, trans_can_name))
            if reduced_lab_can:
                names_to_save.append((get_ws_names_from_group(reduced_lab_can), [], trans_can_name))
            if reduced_hab_can:
                names_to_save.append((get_ws_names_from_group(reduced_hab_can), [], trans_can_name))
            if reduced_lab_sample:
                names_to_save.append((get_ws_names_from_group(reduced_lab_sample), trans_name, []))
            if reduced_hab_sample:
                names_to_save.append((get_ws_names_from_group(reduced_hab_sample), trans_name, []))

        # If we have merged reduction then store the
        elif reduced_merged:
            names_to_save.append((get_ws_names_from_group(reduced_merged), trans_name, trans_can_name))
            if reduced_bgsub:
                names_to_save.append((reduced_bgsub, trans_name, trans_can_name))
        else:
            if reduced_lab:
                names_to_save.append((get_ws_names_from_group(reduced_lab), trans_name, trans_can_name))
            if reduced_hab:
                names_to_save.append((get_ws_names_from_group(reduced_hab), trans_name, trans_can_name))
            if reduced_bgsub:
                names_to_save.append((reduced_bgsub, trans_name, trans_can_name))

    return names_to_save


def get_event_slice_names_to_save(reduction_packages, save_can):
    """
    Extracts all the output names from a list of reduction packages which contain event sliced data.
    The workspaces in these reduction packages are gruop workspaces, except for transmissions.

    :param reduction_packages: a list of reduction packages
    :param save_can: a bool, whether or not to save unsubtracted can workspace
    :return: a list of workspace names to save.
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

        reduced_bgsub_names = reduction_package.reduced_bgsub_name

        reduced_lab_names = [] if reduced_lab is None else reduced_lab.getNames()
        reduced_hab_names = [] if reduced_hab is None else reduced_hab.getNames()
        reduced_merged_names = [] if reduced_merged is None else reduced_merged.getNames()
        reduced_lab_can_names = [] if reduced_lab_can is None else reduced_lab_can.getNames()
        reduced_hab_can_names = [] if reduced_hab_can is None else reduced_hab_can.getNames()
        reduced_lab_sample_names = [] if reduced_lab_sample is None else reduced_lab_sample.getNames()
        reduced_hab_sample_names = [] if reduced_hab_sample is None else reduced_hab_sample.getNames()

        trans_name = get_transmission_names_to_save(reduction_package, False)
        trans_can_name = get_transmission_names_to_save(reduction_package, True)

        def _get_names_in_list(_list, _trans_name, _trans_can_name):
            return ((name, _trans_name, _trans_can_name) for name in _list if name not in (None, ""))

        if save_can:
            names_to_save.extend(_get_names_in_list(reduced_merged_names, trans_name, trans_can_name))
            names_to_save.extend(_get_names_in_list(reduced_lab_names, trans_name, trans_can_name))
            names_to_save.extend(_get_names_in_list(reduced_hab_names, trans_name, trans_can_name))
            names_to_save.extend(_get_names_in_list(reduced_lab_can_names, "", trans_can_name))
            names_to_save.extend(_get_names_in_list(reduced_hab_can_names, "", trans_can_name))
            names_to_save.extend(_get_names_in_list(reduced_lab_sample_names, trans_name, ""))
            names_to_save.extend(_get_names_in_list(reduced_hab_sample_names, trans_name, ""))
        elif reduced_merged:
            names_to_save.extend(_get_names_in_list(reduced_merged_names, trans_name, trans_can_name))
        else:
            names_to_save.extend(_get_names_in_list(reduced_lab_names, trans_name, trans_can_name))
            names_to_save.extend(_get_names_in_list(reduced_hab_names, trans_name, trans_can_name))
        names_to_save.extend(_get_names_in_list(reduced_bgsub_names, trans_name, trans_can_name))

    # We might have some workspaces as duplicates (the group workspaces), so make them unique
    return set(names_to_save)


def save_workspace_to_file(
    workspace_name,
    file_formats: list,
    file_name,
    additional_run_numbers: Dict[str, str],
    additional_metadata: Dict[str, str],
    transmission_name: str = "",
    transmission_can_name: str = "",
):
    """
    Saves the workspace to the different file formats specified in the state object.

    :param workspace_name: the name of the output workspace and also the name of the file
    :param file_formats: a list of file formats to save
    :param file_name: name of file to save
    :param additional_run_numbers: a dict of workspace type to run number
    :param additional_metadata: metadata names and values to include in saved output files.
    :param transmission_name: name of sample transmission workspace to save to file
            for CanSAS algorithm. Only some workspaces have a corresponding transmission workspace.
    :param transmission_can_name: name of can transmission workspace. As above.
    """
    save_name = "SANSSave"
    save_options = {"InputWorkspace": workspace_name}
    save_options.update({"Filename": file_name, "Transmission": transmission_name, "TransmissionCan": transmission_can_name})
    save_options.update(additional_run_numbers)
    save_options.update(additional_metadata)

    if SaveType.NEXUS in file_formats:
        save_options.update({"Nexus": True})
    if SaveType.CAN_SAS in file_formats:
        save_options.update({"CanSAS": True})
    if SaveType.NX_CAN_SAS in file_formats:
        save_options.update({"NXcanSAS": True})
    if SaveType.NIST_QXY in file_formats:
        save_options.update({"NistQxy": True})
    if SaveType.RKH in file_formats:
        save_options.update({"RKH": True})
    if SaveType.CSV in file_formats:
        save_options.update({"CSV": True})

    save_alg = create_unmanaged_algorithm(save_name, **save_options)
    save_alg.execute()


def subtract_scaled_background(reduction_package, scaled_ws_name: str):
    def run_minus_alg():
        output_name = ws_name + SCALED_BGSUB_SUFFIX
        minus_options["LHSWorkspace"] = ws_name
        minus_options["OutputWorkspace"] = output_name
        minus_alg = create_unmanaged_algorithm(minus_name, **minus_options)
        minus_alg.setAlwaysStoreInADS(True)
        minus_alg.execute()
        output_workspaces_names.append(output_name)
        output_workspaces.append(get_workspace_from_algorithm(minus_alg, "OutputWorkspace"))

    minus_name = "Minus"
    minus_options = {"RHSWorkspace": scaled_ws_name}
    output_workspaces_names = []
    output_workspaces = []

    if reduction_package.reduction_mode == ReductionMode.MERGED:
        for ws_name in reduction_package.reduced_merged_name:
            run_minus_alg()
    elif reduction_package.reduction_mode == ReductionMode.HAB:
        for ws_name in reduction_package.reduced_hab_name:
            run_minus_alg()
    elif reduction_package.reduction_mode == ReductionMode.LAB:
        for ws_name in reduction_package.reduced_lab_name:
            run_minus_alg()
    return output_workspaces, output_workspaces_names


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

    def __init__(
        self,
        state,
        workspaces,
        monitors,
        is_part_of_multi_period_reduction=False,
        is_part_of_event_slice_reduction=False,
        is_part_of_wavelength_range_reduction=False,
    ):
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
        self.reduced_hab_scaled = None
        self.reduced_merged = None
        self.reduced_bgsub = None

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
        self.reduced_bgsub_name = None

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

        self.calculated_transmission = None
        self.calculated_transmission_can = None

        # Unfitted transmission names
        self.unfitted_transmission_name = None
        self.unfitted_transmission_base_name = None
        self.unfitted_transmission_can_name = None
        self.unfitted_transmission_can_base_name = None
