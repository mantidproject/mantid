# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import CreateEmptyTableWorkspace
from sans_core.algorithm_detail.batch_execution import provide_loaded_data, get_reduction_packages
from sans_core.common.enums import SANSDataType, DetectorType
from sans_core.common.general_functions import create_managed_non_child_algorithm

# ----------------------------------------------------------------------------------------------------------------------
# Functions for the execution of a single batch iteration
# ----------------------------------------------------------------------------------------------------------------------
from sans_core.state.Serializer import Serializer


def centre_finder_new(state, r_min, r_max, iterations, position_1_start, position_2_start, tolerance, find_direction, verbose, component):
    """
    Finds the beam centre from a good initial guess.

    This function finds the centre of the beam by splitting the workspace up into 4 quadrants and running a reduction on
    each. The (left, right) and (up, down) reductions are then compared producing residuals which are minimised through
    repeated iteration.
    :param state: This is a sans state, to find the beam centre for.
    :param r_min: This is the inner radius of the quartile mask.
    :param r_max: This is the outer radius of the quartile mask.
    :param max_iter: This is the maximum number of iterations.
    :param position_1_start: This is the starting position of the search on the x axis.
    :param position_2_start: This is the starting position of the search on the y axis.
    :param tolerance: This is the tolerance for the search.
    :param find_direction: This is an enumerator controlling which axis or both should be searched.
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
        SANSDataType.SAMPLE_SCATTER: "SampleScatterMonitorWorkSpace",
        SANSDataType.CAN_SCATTER: "CanScatterMonitorWorkspace",
    }

    workspaces, monitors = provide_loaded_data(state, False, workspace_to_name, workspace_to_monitor)

    # ------------------------------------------------------------------------------------------------------------------
    # Get reduction settings
    # Split into individual bundles which can be reduced individually. We split here if we have multiple periods or
    # sliced times for example. For the beam centre finder we only use the first period.
    # ------------------------------------------------------------------------------------------------------------------
    reduction_packages = get_reduction_packages(state, workspaces, monitors)
    reduction_package = reduction_packages[0]

    # ------------------------------------------------------------------------------------------------------------------
    # Setup the beam centre finder algorithm.
    # ------------------------------------------------------------------------------------------------------------------
    beam_centre_finder = "SANSBeamCentreFinder"
    beam_centre_finder_options = {
        "Iterations": iterations,
        "RMin": r_min / 1000,
        "RMax": r_max / 1000,
        "Position1Start": position_1_start,
        "Position2Start": position_2_start,
        "Tolerance": tolerance,
        "Direction": find_direction.value,
        "Verbose": verbose,
        "Component": component.value,
    }
    beam_centre_alg = create_managed_non_child_algorithm(beam_centre_finder, **beam_centre_finder_options)
    beam_centre_alg.setChild(False)
    set_properties_for_beam_centre_algorithm(beam_centre_alg, reduction_package, workspace_to_name, workspace_to_monitor)
    # -----------------------------------
    #  Run the beam centre finder algorithm.
    # -----------------------------------
    beam_centre_alg.execute()

    # -----------------------------------
    #  Get the outputs
    # -----------------------------------
    centre1 = beam_centre_alg.getProperty("Centre1").value
    centre2 = beam_centre_alg.getProperty("Centre2").value

    return {"pos1": centre1, "pos2": centre2}


def centre_finder_mass(
    state, r_min=0.06, max_iter=10, position_1_start=0.0, position_2_start=0.0, tolerance=0.0001251, component=DetectorType.LAB
):
    """
    Finds the beam centre from an initial guess.

    This function finds the centre of the beam by splitting the workspace up into 4 quadrants and running a reduction on
    each. The (left, right) and (up, down) reductions are then compared producing residuals which are minimised through
    repeated iteration.
    :param state: This is a sans state, to find the beam centre for.
    :param r_min: This is the radius of the central beam to mask out.
    :param position_1_start: This is the starting position of the search on the x axis.
    :param position_2_start: This is the starting position of the search on the y axis.
    :param tolerance: This is the tolerance for the search.
    """
    # ------------------------------------------------------------------------------------------------------------------
    # Load the data
    # ------------------------------------------------------------------------------------------------------------------
    workspace_to_name = {SANSDataType.SAMPLE_SCATTER: "SampleScatterWorkspace"}

    workspace_to_monitor = {SANSDataType.SAMPLE_SCATTER: "SampleScatterMonitorWorkSpace"}

    workspaces, monitors = provide_loaded_data(state, False, workspace_to_name, workspace_to_monitor)

    # ------------------------------------------------------------------------------------------------------------------
    # Get reduction settings
    # Split into individual bundles which can be reduced individually. We split here if we have multiple periods or
    # sliced times for example. For the beam centre finder we only use the first period.
    # ------------------------------------------------------------------------------------------------------------------
    reduction_packages = get_reduction_packages(state, workspaces, monitors)
    reduction_package = reduction_packages[0]
    # ------------------------------------------------------------------------------------------------------------------
    # Run reductions (one at a time)
    # ------------------------------------------------------------------------------------------------------------------
    beam_centre_finder = "SANSBeamCentreFinderMassMethod"
    beam_centre_finder_options = {
        "RMin": r_min / 1000,
        "Centre1": position_1_start,
        "Centre2": position_2_start,
        "Tolerance": tolerance,
        "Component": component.value,
    }
    beam_centre_alg = create_managed_non_child_algorithm(beam_centre_finder, **beam_centre_finder_options)
    beam_centre_alg.setChild(False)

    # -----------------------------------
    # Set the properties on the algorithm.
    # -----------------------------------
    set_properties_for_beam_centre_algorithm(beam_centre_alg, reduction_package, workspace_to_name, workspace_to_monitor)
    # -----------------------------------
    #  Run the algorithm.
    # -----------------------------------
    beam_centre_alg.execute()

    # -----------------------------------
    #  Get the outputs
    # -----------------------------------

    centre1 = beam_centre_alg.getProperty("Centre1").value
    centre2 = beam_centre_alg.getProperty("Centre2").value

    return {"pos1": centre1, "pos2": centre2}


def set_properties_for_beam_centre_algorithm(beam_centre_alg, reduction_package, workspace_to_name, workspace_to_monitor):
    """
    Sets up everything necessary on the beam centre algorithm.

    :param beam_centre_alg: a handle to the beam centre algorithm
    :param reduction_package: a reduction package object
    :param workspace_to_name: the workspace to name map
    :param workspace_to_monitor: a workspace to monitor map
    """
    # Go through the elements of the reduction package and set them on the beam centre algorithm
    # Set the SANSState
    state = reduction_package.state
    state_json = Serializer.to_json(state)
    beam_centre_alg.setProperty("SANSState", state_json)

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
