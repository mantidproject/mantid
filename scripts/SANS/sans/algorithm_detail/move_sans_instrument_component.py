# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from enum import Enum

from sans.algorithm_detail.move_workspaces import create_mover
from sans.common.enums import DetectorType
from sans.state.AllStates import AllStates
from sans.state.StateObjects.state_instrument_info import StateInstrumentInfo


class MoveTypes(Enum):
    # Get which move operation the user wants to perform on the workspace. This can be:
    # Initial move: Suitable when a workspace has been freshly loaded.
    INITIAL_MOVE = 1
    # Elementary displacement: Takes the degrees of freedom of the detector into account.
    # This is normally used for beam center finding
    ELEMENTARY_DISPLACEMENT = 2
    # Set to zero: Set the component to its IDF position
    RESET_POSITION = 3


def move_component(component_name, state: AllStates, move_type, workspace, is_transmission_workspace=False, beam_coordinates=None):
    mover = create_mover(workspace, state)
    inst_info = state.instrument_info

    # Ensure we handle a None type
    parsed_component_name = _get_full_component_name(user_comp_name=component_name, inst_info=inst_info)
    # Get the selected component and the beam coordinates
    if not beam_coordinates:
        beam_coordinates = _get_coordinates(component_name=parsed_component_name, state=state)

    if move_type is MoveTypes.ELEMENTARY_DISPLACEMENT:
        if not beam_coordinates or len(beam_coordinates) == 0:
            raise ValueError("Beam coordinates were not specified. An elementary displacement requires beam coordinates.")
        mover.move_with_elementary_displacement(workspace, beam_coordinates, parsed_component_name)

    elif move_type is MoveTypes.INITIAL_MOVE:
        mover.move_initial(workspace, beam_coordinates, parsed_component_name, is_transmission_workspace)

    elif move_type is MoveTypes.RESET_POSITION:
        mover.set_to_zero(workspace, parsed_component_name)

    else:
        raise ValueError("move_sans_instrument_component: The selection {0} for the  move type is unknown".format(str(move_type)))


def _get_coordinates(component_name, state):
    """
    Gets the coordinates for a particular component.

    If the coordinates were not specified by the user then the coordinates are taken from the move state.
    There are several possible scenarios
    1. component is specified => take the beam centre from the appropriate detector
    2. component is not specified => take the beam centre from the LAB
    :param move_info: a SANSStateMove object
    :param full_component_name: The full component name as it is known to the Mantid instrument
    :return:
    """
    # Get the selected detector
    detectors = state.move.detectors
    detector_type = _get_detector_for_component(inst_info=state.instrument_info, component=component_name)

    # If the detector is unknown take the position from the LAB
    if detector_type:
        selected_detector = state.move.detectors[detector_type.value]
    else:
        selected_detector = detectors[DetectorType.LAB.value]

    pos1 = selected_detector.sample_centre_pos1
    pos2 = selected_detector.sample_centre_pos2
    coordinates = [pos1, pos2]
    return coordinates


def _get_full_component_name(user_comp_name, inst_info):
    """
    Select the detector name for the input component.
    The component can be either:
    1. An actual component name for LAB or HAB
    2. Or the word HAB, LAB which will then select the actual component name, e.g. main-detector-bank
    :param move_info: a SANSInstInfo object
    :param user_comp_name: the name of the component as a string
    :return: the full name of the component or an empty string if it is not found.
    """
    selected_detector = _get_detector_for_component(inst_info=inst_info, component=user_comp_name)
    if selected_detector:
        return inst_info.detector_names[selected_detector.value].detector_name
    else:
        return ""


def _get_detector_for_component(inst_info: StateInstrumentInfo, component):
    """
    Get the detector for the selected component.

    The detector can be either an actual component name or a HAB, LAB abbreviation
    :param inst_info: a stateInstrumentInfo
    :param component: the selected component
    :return: Enum of the detector type
    """
    detectors = inst_info.detector_names
    hab_values = detectors[DetectorType.HAB.value]
    lab_values = detectors[DetectorType.LAB.value]

    has_hab = True
    if not hab_values.detector_name and not hab_values.detector_name_short:
        has_hab = False

    if component in ("LAB", lab_values.detector_name, lab_values.detector_name_short):
        return DetectorType.LAB
    elif has_hab and component in ("HAB", hab_values.detector_name, hab_values.detector_name_short):
        return DetectorType.HAB
    else:
        return None
