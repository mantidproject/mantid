# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.py3compat.enum import Enum

from sans.algorithm_detail.move_workspaces import create_mover
from sans.common.enums import DetectorType


class MoveTypes(Enum):
    # Get which move operation the user wants to perform on the workspace. This can be:
    # Initial move: Suitable when a workspace has been freshly loaded.
    INITIAL_MOVE = 1
    # Elementary displacement: Takes the degrees of freedom of the detector into account.
    # This is normally used for beam center finding
    ELEMENTARY_DISPLACEMENT = 2
    # Set to zero: Set the component to its IDF position
    RESET_POSITION = 3


def move_component(component_name, move_info, move_type,
                   workspace, is_transmission_workspace=False, beam_coordinates=None):
    mover = create_mover(workspace)

    # Ensure we handle a None type
    parsed_component_name = _get_full_component_name(user_comp_name=component_name, move_info=move_info)
    # Get the selected component and the beam coordinates
    if not beam_coordinates:
        beam_coordinates = _get_coordinates(component_name=parsed_component_name, move_info=move_info)

    if move_type is MoveTypes.ELEMENTARY_DISPLACEMENT:
        if not beam_coordinates or len(beam_coordinates) == 0:
            raise ValueError("Beam coordinates were not specified. An elementary displacement "
                             "requires beam coordinates.")
        mover.move_with_elementary_displacement(move_info, workspace, beam_coordinates, parsed_component_name)

    elif move_type is MoveTypes.INITIAL_MOVE:
        mover.move_initial(move_info, workspace, beam_coordinates, parsed_component_name, is_transmission_workspace)

    elif move_type is MoveTypes.RESET_POSITION:
        mover.set_to_zero(move_info, workspace, parsed_component_name)

    else:
        raise ValueError("move_sans_instrument_component: The selection {0} for the  move type "
                         "is unknown".format(str(move_type)))


def _get_coordinates(move_info, component_name):
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
    detectors = move_info.detectors
    selected_detector = _get_detector_for_component(move_info, component_name)

    # If the detector is unknown take the position from the LAB
    if selected_detector is None:
        selected_detector = detectors[DetectorType.to_string(DetectorType.LAB)]
    pos1 = selected_detector.sample_centre_pos1
    pos2 = selected_detector.sample_centre_pos2
    coordinates = [pos1, pos2]
    return coordinates


def _get_full_component_name(user_comp_name, move_info):
    """
    Select the detector name for the input component.
    The component can be either:
    1. An actual component name for LAB or HAB
    2. Or the word HAB, LAB which will then select the actual component name, e.g. main-detector-bank
    :param move_info: a SANSStateMove object
    :param user_comp_name: the name of the component as a string
    :return: the full name of the component or an empty string if it is not found.
    """
    selected_detector = _get_detector_for_component(move_info, user_comp_name)
    return selected_detector.detector_name if selected_detector is not None else ""


def _get_detector_for_component(move_info, component):
    """
    Get the detector for the selected component.

    The detector can be either an actual component name or a HAB, LAB abbreviation
    :param move_info: a SANSStateMove object
    :param component: the selected component
    :return: an equivalent detector to teh selected component or None
    """
    detectors = move_info.detectors
    selected_detector = None
    if component == "HAB":
        selected_detector = detectors[DetectorType.to_string(DetectorType.HAB)]
    elif component == "LAB":
        selected_detector = detectors[DetectorType.to_string(DetectorType.LAB)]
    else:
        # Check if the component is part of the detector names
        for _, detector in list(detectors.items()):
            if detector.detector_name == component or detector.detector_name_short == component:
                selected_detector = detector
    return selected_detector
