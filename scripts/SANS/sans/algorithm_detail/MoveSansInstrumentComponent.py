# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.py3compat.enum import Enum

from sans.algorithm_detail.move_workspaces import create_mover
from sans.common.enums import DetectorType


class MoveTypes(Enum):
    INITIAL_MOVE = 1
    ELEMENTARY_DISPLACEMENT = 2
    RESET_POSITION = 3


class MoveSansInstrumentComponent(object):
    def __init__(self, move_type,
                 beam_coordinates=None):

        if not move_type:
            raise ValueError("A MoveTypes is required, none was provided")

        self.beam_coordinates = beam_coordinates
        self.move_type = move_type

    def move_component(self, component_name, move_info,
                       workspace, is_transmission_workspace=False):
        mover = create_mover(workspace)

        # Ensure we handle a None type
        parsed_component_name = self._get_full_component_name(user_comp_name=component_name, move_info=move_info)
        # Get the selected component and the beam coordinates
        coordinates = self._get_coordinates(component_name=parsed_component_name, move_info=move_info)

        # Get which move operation the user wants to perform on the workspace. This can be:
        # 1. Initial move: Suitable when a workspace has been freshly loaded.
        # 2. Elementary displacement: Takes the degrees of freedom of the detector into account. This is normally used
        #    for beam center finding
        # 3. Set to zero: Set the component to its zero position

        selected_move_type = self.move_type

        if selected_move_type is MoveTypes.ELEMENTARY_DISPLACEMENT:
            if not coordinates or len(coordinates) == 0:
                raise ValueError("Beam coordinates were not specified. An elementary displacement "
                                 "requires beam coordinates.")
            mover.move_with_elementary_displacement(move_info, workspace, coordinates, parsed_component_name)

        elif selected_move_type is MoveTypes.INITIAL_MOVE:
            mover.move_initial(move_info, workspace, coordinates, parsed_component_name, is_transmission_workspace)

        elif selected_move_type is MoveTypes.RESET_POSITION:
            mover.set_to_zero(move_info, workspace, parsed_component_name)

        else:
            raise ValueError("MoveSansInstrumentComponent: The selection {0} for the  move type "
                             "is unknown".format(str(selected_move_type)))

    def _get_coordinates(self, move_info, component_name):
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
        coordinates = self.beam_coordinates
        if not coordinates:
            # Get the selected detector
            detectors = move_info.detectors
            selected_detector = self._get_detector_for_component(move_info, component_name)

            # If the detector is unknown take the position from the LAB
            if selected_detector is None:
                selected_detector = detectors[DetectorType.to_string(DetectorType.LAB)]
            pos1 = selected_detector.sample_centre_pos1
            pos2 = selected_detector.sample_centre_pos2
            coordinates = [pos1, pos2]
        return coordinates

    @staticmethod
    def _get_full_component_name(user_comp_name, move_info):
        """
        Select the detector name for the input component.
        The component can be either:
        1. An actual component name for LAB or HAB
        2. Or the word HAB, LAB which will then select the actual component name, e.g. main-detector-bank
        :param move_info: a SANSStateMove object
        :return: the full name of the component or an empty string if it is not found.
        """
        selected_detector = MoveSansInstrumentComponent._get_detector_for_component(move_info, user_comp_name)
        return selected_detector.detector_name if selected_detector is not None else ""

    @staticmethod
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
