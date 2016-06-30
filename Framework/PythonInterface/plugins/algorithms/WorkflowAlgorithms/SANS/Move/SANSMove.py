""" SANSMove algorithm to move a workspace according to the instrument settings."""

from mantid.kernel import (Direction, PropertyManagerProperty, Property, StringListValidator)
from mantid.api import (DataProcessorAlgorithm, MatrixWorkspaceProperty, AlgorithmFactory, PropertyMode)

from SANS.Move.SANSMoveWorkspaces import (SANSMoveFactory)
from SANS2.State.SANSStateSerializer import create_deserialized_sans_state_from_property_manager
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

    @staticmethod
    def _make_move_type_map():
        return {'InitialMove': MoveType.InitialMove, 'ElementaryDisplacement': MoveType.ElementaryDisplacement,
                'SetToZero': MoveType.SetToZero}

    def PyInit(self):
        # ----------
        # INPUT
        # ----------
        self.declareProperty(PropertyManagerProperty('SANSState'),
                             doc='A property manager which fulfills the SANSState contract.')

        self.declareProperty(MatrixWorkspaceProperty(SANSConstants.input_workspace, '',
                                                     optional=PropertyMode.Mandatory, direction=Direction.InputOutput),
                             doc='The sample scatter workspace. This workspace does not contain monitors.')

        move_types = StringListValidator(SANSMove._make_move_type_map().keys())

        self.declareProperty('MoveType', 'ElementaryDisplacement', validator=move_types, direction=Direction.Input,
                             doc='The type of movement. This can be: '
                                 '1) InitialMove for freshly workspaces, '
                                 '2) ElementaryDisplacement of the instrument component, '
                                 '3) SetToZero resets a component to the initial position.')

        # ------------
        #  OUTPUT
        # ------------
        self.declareProperty(MatrixWorkspaceProperty(SANSConstants.output_workspace, '',
                                                     optional=PropertyMode.Optional, direction=Direction.Output),
                             doc='The sample scatter workspace. This workspace does not contain monitors.')

    def PyExec(self):
        # Read the state
        state_property_manager = self.getProperty("SANSState").value
        state = create_deserialized_sans_state_from_property_manager(state_property_manager)

        # Get the correct SANS move object from the SANSMoveFactory
        move_factory = SANSMoveFactory()
        mover = move_factory.create_mover(state)

        # Get the workspace and the beam coordinates
        workspace = self.getProperty(SANSConstants.input_workspace).value
        coordinates = self._get_beam_coordinates()
        component = self.getProperty("Component").value
        move_info = state.move

        # Get which move operation the user wants to perform on the workspace. This can be:
        # 1. Initial move: Suitable when a workspace has been freshly loaded.
        # 2. Elementary displacement: Takes the degrees of freedom of the detector into account. This is normally used
        #    for beam center finding
        # 3. Set to zero: Set the component to its zero position
        move_type_input = self.getProperty("MoveType").value
        move_type_map = SANSMove._make_move_type_map()
        selected_move_type = move_type_map[move_type_input]
        if selected_move_type is MoveType.ElementaryDisplacement:
            mover.move_with_elementary_displacement(move_info, workspace, coordinates, component)
        elif selected_move_type is MoveType.InitialMove:
            pass
        elif selected_move_type is MoveType.SetToZero:
            pass
        else:
            raise ValueError("SANSMove: The selection {} for the  move type is unknown".format(str(selected_move_type)))

        # Perform the desired operation

        # Set the output workspace and new position

    def validateInputs(self):
        errors = dict()
        # Check that the input can be converted into the right state object
        state_property_manager = self.getProperty("SANSState").value
        try:
            state = create_deserialized_sans_state_from_property_manager(state_property_manager)
            state.property_manager = state_property_manager
            state.validate()
        except ValueError as e:
            errors.update({"SANSSMove": str(e)})
        return errors


# Register algorithm with Mantid
AlgorithmFactory.subscribe(SANSMove)
