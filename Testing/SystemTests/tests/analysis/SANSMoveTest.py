# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=too-many-public-methods, invalid-name, too-many-arguments

from __future__ import (absolute_import, division, print_function)
import unittest
import systemtesting


from mantid.api import AlgorithmManager
from mantid.kernel import (Quat, V3D)
from sans.algorithm_detail.move_workspaces import (create_mover, SANSMoveLOQ, SANSMoveSANS2D, SANSMoveLARMORNewStyle,
                                                   SANSMoveLARMOROldStyle)
from sans.common.enums import (SANSFacility, DetectorType)
# Not clear why the names in the module are not found by Pylint, but it seems to get confused. Hence this check
# needs to be disabled here.
# pylint: disable=no-name-in-module
from sans.test_helper.test_director import TestDirector
from sans.state.move import get_move_builder
from sans.state.data import get_data_builder
from sans.common.file_information import SANSFileInformationFactory


def load_workspace(file_name):
    alg = AlgorithmManager.createUnmanaged("Load")
    alg.initialize()
    alg.setChild(True)
    alg.setProperty("Filename", file_name)
    alg.setProperty("OutputWorkspace", "dummy")
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


class SANSMoveFactoryTest(unittest.TestCase):
    def _do_test(self, file_name, mover_type):
        # Arrange
        workspace = load_workspace(file_name)
        # Act
        mover = create_mover(workspace)
        # Assert
        self.assertTrue(isinstance(mover, mover_type))

    def test_that_LOQ_strategy_is_selected(self):
        file_name = "LOQ74044"
        mover_type = SANSMoveLOQ
        self._do_test(file_name, mover_type)

    def test_that_SANS2D_strategy_is_selected(self):
        file_name = "SANS2D00028784"
        mover_type = SANSMoveSANS2D
        self._do_test(file_name, mover_type)

    def test_that_LARMOR_new_style_strategy_is_selected(self):
        file_name = "LARMOR00002260"
        mover_type = SANSMoveLARMORNewStyle
        self._do_test(file_name, mover_type)

    def test_that_LARMOR_8Tubes_strategy_is_selected(self):
        file_name = "LARMOR00000063"
        mover_type = SANSMoveLARMOROldStyle
        self._do_test(file_name, mover_type)

    def test_that_ZOOM_strategy_is_selected(self):
        # TODO when test data becomes available
        pass


class SANSMoveTest(unittest.TestCase):
    @staticmethod
    def _get_simple_state(sample_scatter, lab_x_translation_correction=None, lab_z_translation_correction=None):
        # Set the data
        file_information_factory = SANSFileInformationFactory()
        file_information = file_information_factory.create_sans_file_information(sample_scatter)
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter(sample_scatter)
        data_info = data_builder.build()

        # Set the move parameters
        builder = get_move_builder(data_info)
        if lab_x_translation_correction is not None:
            builder.set_LAB_x_translation_correction(lab_x_translation_correction)
        if lab_z_translation_correction is not None:
            builder.set_LAB_z_translation_correction(lab_z_translation_correction)
        move_info = builder.build()

        # Get the sample state
        test_director = TestDirector()
        test_director.set_states(data_state=data_info, move_state=move_info)
        return test_director.construct()

    @staticmethod
    def _get_position_and_rotation(workspace, move_info, component):
        instrument = workspace.getInstrument()
        component_name = move_info.detectors[component].detector_name
        detector = instrument.getComponentByName(component_name)
        position = detector.getPos()
        rotation = detector.getRotation()
        return position, rotation

    @staticmethod
    def _provide_mover(workspace):
        return create_mover(workspace)

    def compare_expected_position(self, expected_position, expected_rotation, component, move_info, workspace):
        position, rotation = SANSMoveTest._get_position_and_rotation(workspace, move_info, component)
        for index in range(0, 3):
            self.assertAlmostEqual(position[index], expected_position[index], delta=1e-4)
        for index in range(0, 4):
            self.assertAlmostEqual(rotation[index], expected_rotation[index], delta=1e-4)

    def check_that_elementary_displacement_with_only_translation_is_correct(self, workspace, move_alg, move_info,
                                                                            coordinates, component, component_key):
        # Get position and rotation before the move
        position_before_move, rotation_before_move = SANSMoveTest._get_position_and_rotation(workspace, move_info,
                                                                                             component_key)
        expected_position_elementary_move = V3D(position_before_move[0] - coordinates[0],
                                                position_before_move[1] - coordinates[1],
                                                position_before_move[2])
        expected_rotation = rotation_before_move
        move_alg.setProperty("BeamCoordinates", coordinates)
        move_alg.setProperty("MoveType", "ElementaryDisplacement")
        move_alg.setProperty("Component", component)
        move_alg.execute()
        self.assertTrue(move_alg.isExecuted())

        self.compare_expected_position(expected_position_elementary_move, expected_rotation,
                                       component_key, move_info, workspace)

    def check_that_sets_to_zero(self, workspace, move_alg, move_info, comp_name=None):
        def _get_components_to_compare(_key, _move_info, _component_names):
            if _key in _move_info.detectors:
                _name = _move_info.detectors[_key].detector_name
                _component_names.append(_name)

        # Reset the position to zero
        move_alg.setProperty("Workspace", workspace)
        move_alg.setProperty("MoveType", "SetToZero")
        if comp_name is not None:
            move_alg.setProperty("Component", comp_name)
        else:
            move_alg.setProperty("Component", "")
        move_alg.execute()
        self.assertTrue(move_alg.isExecuted())

        # Get the components to compare
        if comp_name is None:
            component_names = list(move_info.monitor_names.values())
            hab_name = DetectorType.HAB.name
            lab_name = DetectorType.LAB.name,
            _get_components_to_compare(hab_name, move_info, component_names)
            _get_components_to_compare(lab_name, move_info, component_names)
            component_names.append("some-sample-holder")
        else:
            component_names = [comp_name]

        # Ensure that the positions on the base instrument and the instrument are the same
        instrument = workspace.getInstrument()
        base_instrument = instrument.getBaseInstrument()
        for component_name in component_names:
            # Confirm that the positions are the same
            component = instrument.getComponentByName(component_name)
            base_component = base_instrument.getComponentByName(component_name)

            # If we are dealing with a monitor which has not been implemented we need to continue
            if component is None or base_component is None:
                continue

            position = component.getPos()
            position_base = base_component.getPos()
            for index in range(0, 3):
                self.assertAlmostEquals(position[index], position_base[index], delta=1e-4)
            rotation = component.getRotation()
            rotation_base = base_component.getRotation()
            for index in range(0, 4):
                self.assertAlmostEquals(rotation[index], rotation_base[index], delta=1e-4)

    def _run_move(self, state, workspace, move_type, beam_coordinates=None, component=None):
        move_alg = AlgorithmManager.createUnmanaged("SANSMove")
        move_alg.setChild(True)
        move_alg.initialize()
        state_dict = state.property_manager

        move_alg.setProperty("SANSState", state_dict)
        move_alg.setProperty("Workspace", workspace)
        move_alg.setProperty("MoveType", move_type)

        if beam_coordinates is not None:
            move_alg.setProperty("BeamCoordinates", beam_coordinates)

        if component is not None:
            move_alg.setProperty("Component", component)

        # Act
        move_alg.execute()
        self.assertTrue(move_alg.isExecuted())
        return move_alg

    def test_that_LOQ_can_perform_move(self):
        # Arrange
        # Setup data info
        file_name = "LOQ74044"
        lab_x_translation_correction = 123.
        beam_coordinates = [45, 25]
        component = "main-detector-bank"
        component_key = DetectorType.LAB.name

        workspace = load_workspace(file_name)
        state = SANSMoveTest._get_simple_state(sample_scatter=file_name,
                                               lab_x_translation_correction=lab_x_translation_correction)

        # Act
        move_alg = self._run_move(state, workspace=workspace, move_type="InitialMove",
                                  beam_coordinates=beam_coordinates, component=component)

        # Act + Assert for initial move
        move_info = state.move
        center_position = move_info.center_position
        initial_z_position = 15.15
        expected_position = V3D(center_position - beam_coordinates[0] + lab_x_translation_correction,
                                center_position - beam_coordinates[1],
                                initial_z_position)
        expected_rotation = Quat(1., 0., 0., 0.)
        self.compare_expected_position(expected_position, expected_rotation, component_key, move_info, workspace)

        # # Act + Assert for elementary move on high-angle bank
        component_elementary_move = "HAB"
        component_elementary_move_key = DetectorType.HAB.name

        beam_coordinates_elementary_move = [120, 135]
        self.check_that_elementary_displacement_with_only_translation_is_correct(workspace, move_alg, move_info,
                                                                                 beam_coordinates_elementary_move,
                                                                                 component_elementary_move,
                                                                                 component_elementary_move_key)

        # Act + Assert for setting to zero position for all
        self.check_that_sets_to_zero(workspace, move_alg, state.move, comp_name="main-detector-bank")

    def test_that_SANS2D_can_move(self):
        # Arrange
        file_name = "SANS2D00028784"
        lab_z_translation_correction = 123.

        workspace = load_workspace(file_name)
        state = SANSMoveTest._get_simple_state(sample_scatter=file_name,
                                               lab_z_translation_correction=lab_z_translation_correction)
        beam_coordinates = [0.1076, -0.0835]

        # Act
        # The component input is not relevant for SANS2D's initial move. All detectors are moved
        component = None
        move_alg = self._run_move(state, workspace=workspace, move_type="InitialMove",
                                  beam_coordinates=beam_coordinates, component=component)

        # Assert for initial move for low angle bank
        # These values are on the workspace and in the sample logs,
        component_to_investigate = DetectorType.LAB.name
        initial_z_position = 23.281
        rear_det_z = 11.9989755859
        offset = 4.
        total_x = 0.
        total_y = 0.
        total_z = initial_z_position + rear_det_z - offset + lab_z_translation_correction
        expected_position = V3D(total_x - beam_coordinates[0], total_y - beam_coordinates[1], total_z)
        expected_rotation = Quat(1., 0., 0., 0.)
        self.compare_expected_position(expected_position, expected_rotation,
                                       component_to_investigate, state.move, workspace)

        # Assert for initial move for high angle bank
        # These values are on the workspace and in the sample logs
        component_to_investigate = DetectorType.HAB.name
        initial_x_position = 1.1
        x_correction = -0.187987540973
        initial_z_position = 23.281
        z_correction = 1.00575649188
        total_x = initial_x_position + x_correction
        total_y = 0.
        total_z = initial_z_position + z_correction
        expected_position = V3D(total_x - beam_coordinates[0], total_y - beam_coordinates[1], total_z)
        expected_rotation = Quat(0.9968998362876025, 0., 0.07868110579898738, 0.)
        self.compare_expected_position(expected_position, expected_rotation,
                                       component_to_investigate, state.move, workspace)

        # Act + Assert for elementary move
        component_elementary_move = "rear-detector"
        component_elementary_move_key = DetectorType.LAB.name
        beam_coordinates_elementary_move = [120, 135]
        self.check_that_elementary_displacement_with_only_translation_is_correct(workspace, move_alg, state.move,
                                                                                 beam_coordinates_elementary_move,
                                                                                 component_elementary_move,
                                                                                 component_elementary_move_key)

        # # Act + Assert for setting to zero position for all
        self.check_that_sets_to_zero(workspace, move_alg, state.move, comp_name=None)

    def test_that_LARMOR_new_style_can_move(self):
        # Arrange
        file_name = "LARMOR00002260"
        lab_x_translation_correction = 123.

        workspace = load_workspace(file_name)
        state = SANSMoveTest._get_simple_state(sample_scatter=file_name,
                                               lab_x_translation_correction=lab_x_translation_correction)

        # Note that the first entry is an angle while the second is a translation (in meter)
        beam_coordinates = [24., 38.]

        # Act for initial move
        component = None
        move_alg = self._run_move(state, workspace=workspace, move_type="InitialMove",
                                  beam_coordinates=beam_coordinates, component=component)

        # Assert low angle bank for initial move
        # These values are on the workspace and in the sample logs
        component_to_investigate = DetectorType.LAB.name
        # The rotation couples the movements, hence we just insert absoltute value, to have a type of regression test.
        expected_position = V3D(0, -38, 25.3)
        expected_rotation = Quat(0.978146, 0, -0.20792, 0)
        self.compare_expected_position(expected_position, expected_rotation,
                                       component_to_investigate, state.move, workspace)

        # Act + Assert for setting to zero position for all
        self.check_that_sets_to_zero(workspace, move_alg, state.move, comp_name=None)

    def test_that_LARMOR_old_Style_can_be_moved(self):
        # Arrange
        file_name = "LARMOR00000063"
        workspace = load_workspace(file_name)
        state = SANSMoveTest._get_simple_state(sample_scatter=file_name)

        # Note that both entries are translations
        beam_coordinates = [24., 38.]

        # Act
        component = None
        move_alg = self._run_move(state, workspace=workspace, move_type="InitialMove",
                                  beam_coordinates=beam_coordinates, component=component)

        # Assert low angle bank for initial move
        # These values are on the workspace and in the sample logs
        component_to_investigate = DetectorType.LAB.name
        # The rotation couples the movements, hence we just insert absolute value, to have a type of regression test
        # solely.
        expected_position = V3D(-beam_coordinates[0], -beam_coordinates[1], 25.3)
        expected_rotation = Quat(1., 0., 0., 0.)
        self.compare_expected_position(expected_position, expected_rotation,
                                       component_to_investigate, state.move, workspace)

        # Act + Assert for setting to zero position for all
        self.check_that_sets_to_zero(workspace, move_alg, state.move, comp_name=None)

    def test_that_ZOOM_can_be_moved(self):
        # TODO when test data becomes available
        pass

    def test_that_missing_beam_centre_is_taken_from_move_state(self):
        # Arrange
        file_name = "SANS2D00028784"
        lab_z_translation_correction = 123.

        workspace = load_workspace(file_name)
        state = SANSMoveTest._get_simple_state(sample_scatter=file_name,
                                               lab_z_translation_correction=lab_z_translation_correction)
        # These values should be used instead of an explicitly specified beam centre
        state.move.detectors[DetectorType.HAB.name].sample_centre_pos1 = 26.
        state.move.detectors[DetectorType.HAB.name].sample_centre_pos2 = 98.

        # Act
        # The component input is not relevant for SANS2D's initial move. All detectors are moved
        component = "front-detector"
        self._run_move(state, workspace=workspace, move_type="InitialMove", component=component)

        # Assert for initial move for high angle bank
        # These values are on the workspace and in the sample logs
        component_to_investigate = DetectorType.HAB.name
        initial_x_position = 1.1
        x_correction = -0.187987540973
        initial_z_position = 23.281
        z_correction = 1.00575649188
        total_x = initial_x_position + x_correction
        total_y = 0.
        total_z = initial_z_position + z_correction
        expected_position = V3D(total_x - 26., total_y - 98., total_z)
        expected_rotation = Quat(0.9968998362876025, 0., 0.07868110579898738, 0.)
        self.compare_expected_position(expected_position, expected_rotation,
                                       component_to_investigate, state.move, workspace)

    def test_that_missing_beam_centre_is_taken_from_lab_move_state_when_no_component_is_specified(self):
        # Arrange
        file_name = "SANS2D00028784"
        lab_z_translation_correction = 123.

        workspace = load_workspace(file_name)
        state = SANSMoveTest._get_simple_state(sample_scatter=file_name,
                                               lab_z_translation_correction=lab_z_translation_correction)
        # These values should be used instead of an explicitly specified beam centre
        state.move.detectors[DetectorType.LAB.name].sample_centre_pos1 = 26.
        state.move.detectors[DetectorType.LAB.name].sample_centre_pos2 = 98.

        # Act
        # The component input is not relevant for SANS2D's initial move. All detectors are moved
        component = None
        self._run_move(state, workspace=workspace, move_type="InitialMove", component=component)

        # Assert for initial move for low angle bank
        # These values are on the workspace and in the sample logs,
        component_to_investigate = DetectorType.LAB.name
        initial_z_position = 23.281
        rear_det_z = 11.9989755859
        offset = 4.
        total_x = 0.
        total_y = 0.
        total_z = initial_z_position + rear_det_z - offset + lab_z_translation_correction
        expected_position = V3D(total_x - 26., total_y - 98., total_z)
        expected_rotation = Quat(1., 0., 0., 0.)
        self.compare_expected_position(expected_position, expected_rotation,
                                       component_to_investigate, state.move, workspace)


class SANSMoveRunnerTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self._success = False

    def runTest(self):
        suite = unittest.TestSuite()
        suite.addTest(unittest.makeSuite(SANSMoveFactoryTest, 'test'))
        suite.addTest(unittest.makeSuite(SANSMoveTest, 'test'))
        runner = unittest.TextTestRunner()
        res = runner.run(suite)
        if res.wasSuccessful():
            self._success = True

    def requiredMemoryMB(self):
        return 1000

    def validate(self):
        return self._success


if __name__ == '__main__':
    unittest.main()
