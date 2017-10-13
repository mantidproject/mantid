from __future__ import (absolute_import, division, print_function)

from sans.common.general_functions import (create_managed_non_child_algorithm, create_unmanaged_algorithm,
                                           get_output_name, get_base_name_from_multi_period_name)
from sans.common.enums import (SANSDataType, SaveType, OutputMode, ISISReductionMode, FindDirectionEnum)

from sans.algorithm_detail.batch_execution import (provide_loaded_data, get_reduction_packages,
                                                   set_properties_for_reduction_algorithm, get_workspace_from_algorithm,
                                                   group_workspaces_if_required, save_to_file,
                                                   delete_reduced_workspaces, delete_optimization_workspaces)
from mantid.simpleapi import CreateEmptyTableWorkspace

# ----------------------------------------------------------------------------------------------------------------------
# Functions for the execution of a single batch iteration
# ----------------------------------------------------------------------------------------------------------------------
def centre_finder_new(state, r_min = 0.06, r_max = 0.26, iterations = 10, position_1_start = 0.0, position_2_start = 0.0, tolerance = 0.0001251, find_direction = FindDirectionEnum.All):
    """
    Runs a single reduction.

    This function creates reduction packages which essentially contain information for a single valid reduction, run it
    and store the results according to the user specified setting (output_mode). Although this is considered a single
    reduction it can contain still several reductions since the SANSState object can at this point contain slice
    settings which require on reduction per time slice.
    :param state: a SANSState object
    :param use_optimizations: if true then the optimizations of child algorithms are enabled.
    :param output_mode: the output mode
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

    workspace_to_monitor = {SANSDataType.SampleScatter: "SampleScatterMonitorWorkSpace",
                            SANSDataType.CanScatter: "CanScatterMonitorWorkspace"}

    workspaces, monitors = provide_loaded_data(state, False, workspace_to_name, workspace_to_monitor)

    # ------------------------------------------------------------------------------------------------------------------
    # Get reduction settings
    # Split into individual bundles which can be reduced individually. We split here if we have multiple periods or
    # sliced times for example.
    # ------------------------------------------------------------------------------------------------------------------
    reduction_packages = get_reduction_packages(state, workspaces, monitors)
    #pydevd.settrace('localhost', port=5230, stdoutToServer=True, stderrToServer=True)
    # ------------------------------------------------------------------------------------------------------------------
    # Run reductions (one at a time)
    # ------------------------------------------------------------------------------------------------------------------
    beam_centre_finder = "SANSBeamCentreFinder"
    beam_centre_finder_options = {"Iterations": iterations, "RMin": r_min/1000, "RMax": r_max/1000, "Position1Start": position_1_start, "Position2Start": position_2_start, "Tolerance": tolerance, "Direction" : FindDirectionEnum.to_string(find_direction)}
    beam_centre_alg = create_managed_non_child_algorithm(beam_centre_finder, **beam_centre_finder_options)
    beam_centre_alg.setChild(False)
    # Perform the data reduction
    reduction_package = reduction_packages[0]
    # -----------------------------------
    # Set the properties on the algorithm
    # -----------------------------------
    set_properties_for_beam_centre_algorithm(beam_centre_alg, reduction_package,
                                           workspace_to_name, workspace_to_monitor)
    # -----------------------------------
    #  Run the reduction
    # -----------------------------------
    beam_centre_alg.execute()

    # -----------------------------------
    #  Get the outputs
    # -----------------------------------

    centre1 = beam_centre_alg.getProperty("Centre1").value
    centre2 = beam_centre_alg.getProperty("Centre2").value
    create_output_table(centre1, centre2)

    return {"pos1": centre1, "pos2": centre2}

def centre_finder_mass(state, r_min = 0.06, iterations = 10, position_1_start = 0.0, position_2_start = 0.0, tolerance = 0.0001251):
    """
    Runs a single reduction.

    This function creates reduction packages which essentially contain information for a single valid reduction, run it
    and store the results according to the user specified setting (output_mode). Although this is considered a single
    reduction it can contain still several reductions since the SANSState object can at this point contain slice
    settings which require on reduction per time slice.
    :param state: a SANSState object
    :param use_optimizations: if true then the optimizations of child algorithms are enabled.
    :param output_mode: the output mode
    """
    # ------------------------------------------------------------------------------------------------------------------
    # Load the data
    # ------------------------------------------------------------------------------------------------------------------
    workspace_to_name = {SANSDataType.SampleScatter: "SampleScatterWorkspace"}

    workspace_to_monitor = {SANSDataType.SampleScatter: "SampleScatterMonitorWorkSpace"}

    workspaces, monitors = provide_loaded_data(state, False, workspace_to_name, workspace_to_monitor)

    # ------------------------------------------------------------------------------------------------------------------
    # Get reduction settings
    # Split into individual bundles which can be reduced individually. We split here if we have multiple periods or
    # sliced times for example.
    # ------------------------------------------------------------------------------------------------------------------
    reduction_packages = get_reduction_packages(state, workspaces, monitors)
    #pydevd.settrace('localhost', port=5230, stdoutToServer=True, stderrToServer=True)
    # ------------------------------------------------------------------------------------------------------------------
    # Run reductions (one at a time)
    # ------------------------------------------------------------------------------------------------------------------
    beam_centre_finder = "SANSBeamCentreFinderMassMethod"
    beam_centre_finder_options = {"Iterations": iterations, "RMin": r_min/1000, "Centre1": position_1_start, "Centre2": position_2_start, "Tolerance": tolerance}
    beam_centre_alg = create_managed_non_child_algorithm(beam_centre_finder, **beam_centre_finder_options)
    beam_centre_alg.setChild(False)
    # Perform the data reduction
    reduction_package = reduction_packages[0]
    # -----------------------------------
    # Set the properties on the algorithm
    # -----------------------------------
    set_properties_for_beam_centre_algorithm(beam_centre_alg, reduction_package,
                                           workspace_to_name, workspace_to_monitor)
    # -----------------------------------
    #  Run the reduction
    # -----------------------------------
    beam_centre_alg.execute()

    # -----------------------------------
    #  Get the outputs
    # -----------------------------------

    centre1 = beam_centre_alg.getProperty("Centre1").value
    centre2 = beam_centre_alg.getProperty("Centre2").value
    create_output_table(centre1, centre2)

    return {"pos1": centre1, "pos2": centre2}

def set_properties_for_beam_centre_algorithm(beam_centre_alg, reduction_package, workspace_to_name, workspace_to_monitor):
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
    beam_centre_alg.setProperty("SANSState", state_dict)

    # Set the input workspaces
    workspaces = reduction_package.workspaces
    for workspace_type, workspace in list(workspaces.items()):
        if workspace is not None:
            beam_centre_alg.setProperty(workspace_to_name[workspace_type], workspace)

    # Set the monitors
    monitors = reduction_package.monitors
    for workspace_type, monitor in list(monitors.items()):
        if monitor is not None:
            beam_centre_alg.setProperty(workspace_to_monitor[workspace_type], monitor)

def create_output_table(centre1, centre2):
    Centre_position = CreateEmptyTableWorkspace()

    Centre_position.addColumn(type="double", name="X Centre Position")
    Centre_position.addColumn(type="double", name="Y Centre Position")
    Centre_position.addRow({"X Centre Position": centre1, "Y Centre Position": centre2})

