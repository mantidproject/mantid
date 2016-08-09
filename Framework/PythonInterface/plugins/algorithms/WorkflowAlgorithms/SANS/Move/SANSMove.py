# pylint: disable=too-few-public-methods

""" SANSMove algorithm to move a workspace according to the instrument settings."""

from mantid.kernel import (Direction, PropertyManagerProperty, StringListValidator,
                           FloatArrayProperty)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode)

from SANS.Move.SANSMoveWorkspaces import SANSMoveFactory
from SANS2.State.SANSStateBase import create_deserialized_sans_state_from_property_manager
from SANS2.Common.SANSConstants import SANSConstants


class MoveType(object):
    class InitialMove(object):
        pass

    class ElementaryDisplacement(object):
        pass

    class SetToZero(object):
        pass


class SANSMove(DataProcessorAlgorithm):
    def category(self):
        return 'SANS\\Move'

    def summary(self):
        return 'Moves SANS workspaces.'

    def _make_move_type_map(self):
        return {'InitialMove': MoveType.InitialMove, 'ElementaryDisplacement': MoveType.ElementaryDisplacement,
                'SetToZero': MoveType.SetToZero}

    def PyInit(self):
        # State
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        # Workspace which is to be moved
        self.declareProperty(MatrixWorkspaceProperty(SANSConstants.workspace, '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.InOut),
                             doc='The sample scatter workspace. This workspace does not contain monitors.')

        # Move Type
        move_types = StringListValidator(self._make_move_type_map().keys())
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

    def PyExec(self):
        # Read the state
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)

        # Get the correct SANS move strategy from the SANSMoveFactory
        workspace = self.getProperty(SANSConstants.workspace).value
        move_factory = SANSMoveFactory()
        mover = move_factory.create_mover(workspace)

        # Get the selected component and the beam coordinates
        move_info = state.move
        component = self.getProperty("Component").value
        coordinates = self._get_coordinates(move_info, component)
        full_component_name = self._get_full_comonent_name(move_info, component)

        # Get which move operation the user wants to perform on the workspace. This can be:
        # 1. Initial move: Suitable when a workspace has been freshly loaded.
        # 2. Elementary displacement: Takes the degrees of freedom of the detector into account. This is normally used
        #    for beam center finding
        # 3. Set to zero: Set the component to its zero position
        selected_move_type = self._get_move_type()
        if selected_move_type is MoveType.ElementaryDisplacement:
            mover.move_with_elementary_displacement(move_info, workspace, coordinates, full_component_name)
        elif selected_move_type is MoveType.InitialMove:
            mover.move_initial(move_info, workspace, coordinates, full_component_name)
        elif selected_move_type is MoveType.SetToZero:
            mover.set_to_zero(move_info, workspace, full_component_name)
        else:
            raise ValueError("SANSMove: The selection {0} for the  move type "
                             "is unknown".format(str(selected_move_type)))

    def _get_full_comonent_name(self, move_info, component):
        detectors = move_info.detectors
        if component == "HAB":
            selected_detector = detectors[SANSConstants.high_angle_bank]
        else:
            selected_detector = detectors[SANSConstants.low_angle_bank]
        return selected_detector.detector_name

    def _get_move_type(self):
        move_type_input = self.getProperty("MoveType").value
        move_type_map = self._make_move_type_map()
        return move_type_map[move_type_input]

    def _get_coordinates(self, move_info, component):
        coordinates = self.getProperty("BeamCoordinates").value.tolist()
        # If the coordinates were not specified by the user then the coordinates are taken from the move state.
        # There are several possible scenarios
        # 1. component is specified => take the beam centre from the appropriate detector
        # 2. component is not specified => take the beam centre from the LAB
        if not coordinates:
            detectors = move_info.detectors
            if component is not None and component == "HAB":
                selected_detector = detectors[SANSConstants.high_angle_bank]
            else:
                selected_detector = detectors[SANSConstants.low_angle_bank]
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
