# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-few-public-methods

""" SANSMove algorithm to move a workspace according to the instrument settings."""

from __future__ import (absolute_import, division, print_function)

from mantid.api import (DistributedDataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode, Progress)
from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator,
                           FloatArrayProperty)

from sans.algorithm_detail.move_workspaces import create_mover
from sans.common.enums import DetectorType
from sans.state.state_base import create_deserialized_sans_state_from_property_manager


class MoveType(object):
    class InitialMove(object):
        pass

    class ElementaryDisplacement(object):
        pass

    class SetToZero(object):
        pass


def get_detector_for_component(move_info, component):
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


class SANSMove(DistributedDataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Move'

    def summary(self):
        return 'Moves SANS workspaces.'

    def _make_move_type_map(self):
        return {'InitialMove': MoveType.InitialMove,
                'ElementaryDisplacement': MoveType.ElementaryDisplacement,
                'SetToZero': MoveType.SetToZero}

    def PyInit(self):
        # State
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        # Workspace which is to be moved
        self.declareProperty(MatrixWorkspaceProperty("Workspace", '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.InOut),
                             doc='The sample scatter workspace. This workspace does not contain monitors.')

        # Move Type
        move_types = StringListValidator(list(self._make_move_type_map().keys()))
        self.declareProperty('MoveType', 'ElementaryDisplacement', validator=move_types, direction=Direction.Input,
                             doc='The type of movement. This can be: '
                                 '1) InitialMove for freshly workspaces, '
                                 '2) ElementaryDisplacement of the instrument component, '
                                 '3) SetToZero resets a component to the initial position.')

        # Coordinates of the beam
        self.declareProperty(FloatArrayProperty(name='BeamCoordinates', values=[]),
                             doc='The coordinates which are used to position the instrument component(s). If nothing '
                                 'is specified, then the coordinates from SANSState are used')

        # Components which are to be moved
        self.declareProperty('Component', '', direction=Direction.Input, doc='Component that should be moved.')

        # If is a transmission workspace
        self.declareProperty('IsTransmissionWorkspace', False, direction=Direction.Input,
                             doc='If the input workspace is a transmission or direct workspace')

    def PyExec(self):
        # Read the state
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)

        # Get the correct SANS move strategy from create_mover
        workspace = self.getProperty("Workspace").value
        mover = create_mover(workspace)

        # Get the selected component and the beam coordinates
        move_info = state.move
        full_component_name = self._get_full_component_name(move_info)
        coordinates = self._get_coordinates(move_info, full_component_name)

        # Get which move operation the user wants to perform on the workspace. This can be:
        # 1. Initial move: Suitable when a workspace has been freshly loaded.
        # 2. Elementary displacement: Takes the degrees of freedom of the detector into account. This is normally used
        #    for beam center finding
        # 3. Set to zero: Set the component to its zero position
        progress = Progress(self, start=0.0, end=1.0, nreports=2)
        selected_move_type = self._get_move_type()

        if selected_move_type is MoveType.ElementaryDisplacement:
            progress.report("Starting elementary displacement")
            mover.move_with_elementary_displacement(move_info, workspace, coordinates, full_component_name)
        elif selected_move_type is MoveType.InitialMove:
            is_transmission_workspace = self.getProperty("IsTransmissionWorkspace").value
            progress.report("Starting initial move.")
            mover.move_initial(move_info, workspace, coordinates, full_component_name, is_transmission_workspace)
        elif selected_move_type is MoveType.SetToZero:
            progress.report("Starting set to zero.")
            mover.set_to_zero(move_info, workspace, full_component_name)
        else:
            raise ValueError("SANSMove: The selection {0} for the  move type "
                             "is unknown".format(str(selected_move_type)))
        progress.report("Completed move.")

    def _get_full_component_name(self, move_info):
        """
        Select the detector name for the input component.

        The component can be either:
        1. An actual component name for LAB or HAB
        2. Or the word HAB, LAB which will then select the actual component name, e.g. main-detector-bank
        :param move_info: a SANSStateMove object
        :return: the full name of the component or an empty string if it is not found.
        """
        component = self.getProperty("Component").value
        selected_detector = get_detector_for_component(move_info, component)
        return selected_detector.detector_name if selected_detector is not None else ""

    def _get_move_type(self):
        move_type_input = self.getProperty("MoveType").value
        move_type_map = self._make_move_type_map()
        return move_type_map[move_type_input]

    def _get_coordinates(self, move_info, full_component_name):
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
        coordinates = self.getProperty("BeamCoordinates").value.tolist()
        if not coordinates:
            # Get the selected detector
            detectors = move_info.detectors
            selected_detector = get_detector_for_component(move_info, full_component_name)

            # If the detector is unknown take the position from the LAB
            if selected_detector is None:
                selected_detector = detectors[DetectorType.to_string(DetectorType.LAB)]
            pos1 = selected_detector.sample_centre_pos1
            pos2 = selected_detector.sample_centre_pos2
            coordinates = [pos1, pos2]
        return coordinates

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        state_property_manager = self.getProperty("SANSState").value
        try:
            state = create_deserialized_sans_state_from_property_manager(state_property_manager)
            state.property_manager = state_property_manager
            state.validate()
        except ValueError as err:
            errors.update({"SANSSMove": str(err)})

        # Check that if the MoveType is either InitialMove or ElementaryDisplacement, then there are beam coordinates
        # supplied. In the case of SetToZero these coordinates are ignored if they are supplied
        coordinates = self.getProperty("BeamCoordinates").value
        selected_move_type = self._get_move_type()
        if len(coordinates) == 0 and (selected_move_type is MoveType.ElementaryDisplacement):
            errors.update({"BeamCoordinates": "Beam coordinates were not specified. An elementary displacement "
                                              "requires beam coordinates."})
        return errors


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSMove)
