import unittest
import mantid

from mantid.api import AlgorithmManager
from mantid.kernel import (Quat, V3D)
from Move.SANSMove import (SANSMoveFactory, SANSMoveLOQ, SANSMoveSANS2D)
from State.SANSStateData import SANSStateDataISIS
from State.StateBuilder.SANSStateMoveBuilder import get_state_move_builder
from Common.SANSConstants import SANSConstants


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
        move_factory = SANSMoveFactory()
        # Act
        mover = move_factory.create_mover(workspace)
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
    #
    # def test_that_LARMOR_strategy_is_selected(self):
    #     file_name = None
    #     mover_type = SANSMoveLARMORNewStyle
    #     self._do_test(file_name, mover_type)
    #
    # def test_that_LARMOR_8Tubes_strategy_is_selected(self):
    #     file_name = None
    #     mover_type = SANSMoveLARMOROldStyle
    #     self._do_test(file_name, mover_type)


class SANSMoveTest(unittest.TestCase):
    @staticmethod
    def _provide_builder(file_name):
        data_info = SANSStateDataISIS()
        data_info.sample_scatter = file_name
        return get_state_move_builder(data_info)

    @staticmethod
    def _provide_mover(workspace):
        move_factory = SANSMoveFactory()
        return move_factory.create_mover(workspace)

    @staticmethod
    def _get_position_and_rotation(workspace, move_info, component):
        instrument = workspace.getInstrument()
        component_name = move_info.detectors[component].detector_name
        detector = instrument.getComponentByName(component_name)
        position = detector.getPos()
        rotation = detector.getRotation()
        return position, rotation

    def check_that_elementary_displacement_with_only_translation_is_correct(self, workspace, mover, move_info,
                                                                            coordinates, component):
        position_before_move, rotation_before_move = SANSMoveTest._get_position_and_rotation(workspace, move_info,
                                                                                             component)
        expected_position_elementary_move = V3D(position_before_move[0] - coordinates[0],
                                                position_before_move[1] - coordinates[1],
                                                position_before_move[2])
        mover.do_move_with_elementary_displacement(move_info, workspace, coordinates,
                                                   component)
        expected_rotation = rotation_before_move
        self.compare_expected_position(expected_position_elementary_move, expected_rotation,
                                       component, move_info, workspace)

    def compare_expected_position(self, expected_position, expected_rotation, component, move_info, workspace):
        position, rotation = SANSMoveTest._get_position_and_rotation(workspace, move_info, component)
        for index in range(0, 3):
            self.assertAlmostEqual(position[index], expected_position[index], delta=1e-4)
            self.assertAlmostEqual(rotation[index], expected_rotation[index], delta=1e-4)

    def test_that_LOQ_can_perform_a_correct_initial_move_and_subsequent_elementary_move(self):
        # Arrange
        # Setup data info
        file_name = "LOQ74044"
        builder = SANSMoveTest._provide_builder(file_name)
        builder.set_LAB_x_translation_correction = 123
        move_info = builder.build()

        # Setup mover
        workspace = load_workspace(file_name)
        mover = SANSMoveTest._provide_mover(workspace)

        coordinates = [45, 25]
        component = SANSConstants.low_angle_bank

        # Act + Assert for initial move
        center_position = move_info.center_position
        initial_z_position = 15.15
        expected_position = V3D(center_position - coordinates[0], center_position - coordinates[1], initial_z_position)
        expected_rotation = Quat(1., 0., 0., 0.)
        mover.move_initial(move_info, workspace, coordinates, component)
        self.compare_expected_position(expected_position, expected_rotation, component, move_info, workspace)

        # # Act + Assert for elementary move
        component_elementary_move = SANSConstants.high_angle_bank
        coordinates_elementary_move = [120, 135]
        self.check_that_elementary_displacement_with_only_translation_is_correct(workspace, mover, move_info,
                                                                                 coordinates_elementary_move,
                                                                                 component_elementary_move)

    def test_that_SANS2D_can_perform_a_correct_initial_move_and_subsequent_elementary_move(self):
        # Arrange
        # Setup data info
        file_name = "SANS2D00028784"
        builder = SANSMoveTest._provide_builder(file_name)
        additional_offset = 0
        builder.set_LAB_x_translation_correction = additional_offset
        move_info = builder.build()

        # Setup mover
        workspace = load_workspace(file_name)
        mover = SANSMoveTest._provide_mover(workspace)

        coordinates = [0, 0]
        # The component input is not relevant for SANS2D's initial move. All detectors are moved

        # Act for initial move
        mover.move_initial(move_info, workspace, coordinates, component=None)

        # Assert low angle bank for initial move
        # These values are on the workspace and in the sample logs
        component_to_investigate = SANSConstants.low_angle_bank
        initial_z_position = 23.281
        rear_det_z = 11.9989755859
        offset = 4.
        total_x = 0.
        total_y = 0.
        total_z = initial_z_position + rear_det_z - offset
        expected_position = V3D(total_x - coordinates[0], total_y - coordinates[1], total_z)
        expected_rotation = Quat(1., 0., 0., 0.)
        self.compare_expected_position(expected_position, expected_rotation,
                                       component_to_investigate, move_info, workspace)

        # Act + Assert for initial move for high angle bank
        # These values are on the workspace and in the sample logs
        component_to_investigate = SANSConstants.high_angle_bank
        initial_x_position = 1.1
        x_correction = -0.188187
        initial_z_position = 23.281
        z_correction = 1.002
        total_x = initial_x_position + x_correction
        total_y = 0.
        total_z = initial_z_position + z_correction
        expected_position = V3D(total_x - coordinates[0], total_y-coordinates[1], total_z)
        expected_rotation = Quat(1., 0., 7.87625e-05, 0.)
        self.compare_expected_position(expected_position, expected_rotation,
                                       component_to_investigate, move_info, workspace)

        # Act + Assert for elementary move
        component_elementary_move = SANSConstants.low_angle_bank
        coordinates_elementary_move = [120, 135]
        self.check_that_elementary_displacement_with_only_translation_is_correct(workspace, mover, move_info,
                                                                                 coordinates_elementary_move,
                                                                                 component_elementary_move)

    # def test_that_LARMOR_old_Style_can_be_moved(self):
    #     pass

    # def test_that_LARMOR_new_Style_can_be_moved(self):
    #     pass


if __name__ == '__main__':
    unittest.main()
