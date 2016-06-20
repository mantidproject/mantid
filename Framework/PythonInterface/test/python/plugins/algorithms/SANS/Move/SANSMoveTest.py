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

    def compare_expected_position(self, expected_position, expected_rotation, component, move_info, workspace):
        position, rotation = SANSMoveTest._get_position_and_rotation(workspace, move_info, component)
        print "================"
        print position
        print expected_position
        print rotation
        print expected_rotation
        self.assertTrue(position == expected_position)
        self.assertTrue(rotation == expected_rotation)

    def test_that_LOQ_can_perform_a_correct_initial_move_and_subsequent_elementary_move(self):
        # Arrange
        # Setup data info
        file_name = "LOQ74044"
        # builder = SANSMoveTest._provide_builder(file_name)
        # builder.set_LAB_x_translation_correction = 123
        # move_info = builder.build()
        #
        # # Setup mover
        # workspace = load_workspace(file_name)
        # mover = SANSMoveTest._provide_mover(workspace)
        #
        # coordinates = [45, 25]
        # component = SANSConstants.low_angle_bank
        #
        # # Act + Assert for initial move
        # center_position = move_info.center_position
        # initial_z_position = 15.15
        # expected_position = V3D(center_position - coordinates[0], center_position - coordinates[1], initial_z_position)
        # expected_rotation = Quat(1., 0., 0., 0.)
        # mover.move_initial(move_info, workspace, coordinates, component)
        # self.compare_expected_position(expected_position, expected_rotation, component, move_info, workspace)
        #
        # # Act + Assert for elementary move
        # component_elementary_move = SANSConstants.high_angle_bank
        # coordinates_elementary_move = [120, 135]
        # position_before_move, _ = SANSMoveTest._get_position_and_rotation(workspace, move_info,
        #                                                                   component_elementary_move)
        # expected_position_elementary_move = V3D(position_before_move[0] - coordinates_elementary_move[0],
        #                                         position_before_move[1] - coordinates_elementary_move[1],
        #                                         position_before_move[2])
        # mover.do_move_with_elementary_displacement(move_info, workspace, coordinates_elementary_move,
        #                                            component_elementary_move)
        # self.compare_expected_position(expected_position_elementary_move, expected_rotation,
        #                                component_elementary_move, move_info, workspace)

    def test_that_LOQ_can_perform_a_correct_initial_move_and_subsequent_elementary_move(self):
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

        # Act + Assert for initial move for low angle bank
        component = SANSConstants.low_angle_bank
        # These values are on the workspace and in the sample logs
        initial_z_position = 23.281
        rear_det_z = 11.9989755859
        offset = 4.
        total_z = initial_z_position + rear_det_z - offset
        expected_position = V3D(-coordinates[0], - coordinates[1], total_z)
        expected_rotation = Quat(1., 0., 0., 0.)
        mover.move_initial(move_info, workspace, coordinates, component)
        self.compare_expected_position(expected_position, expected_rotation, component, move_info, workspace)

        # Act + Assert for initial move for low angle bank
        component = SANSConstants.high_angle_bank
        # These values are on the workspace and in the sample logs
        initial_z_position = 23.281
        total_z = initial_z_position
        expected_position = V3D(-coordinates[0], - coordinates[1], total_z)
        expected_rotation = Quat(1., 0., 0., 0.)
        mover.move_initial(move_info, workspace, coordinates, component)
        self.compare_expected_position(expected_position, expected_rotation, component, move_info, workspace)

        # Act + Assert for elementary move
        # component_elementary_move = SANSConstants.high_angle_bank
        # coordinates_elementary_move = [120, 135]
        # position_before_move, _ = SANSMoveTest._get_position_and_rotation(workspace, move_info,
        #                                                                   component_elementary_move)
        # expected_position_elementary_move = V3D(position_before_move[0] - coordinates_elementary_move[0],
        #                                         position_before_move[1] - coordinates_elementary_move[1],
        #                                         position_before_move[2])
        # mover.do_move_with_elementary_displacement(move_info, workspace, coordinates_elementary_move,
        #                                            component_elementary_move)
        # self.compare_expected_position(expected_position_elementary_move, expected_rotation,
        #                                component_elementary_move, move_info, workspace)

    # def test_that_LARMOR_old_Style_can_be_moved(self):
    #     pass

    # def test_that_LARMOR_new_Style_can_be_moved(self):
    #     pass


if __name__ == '__main__':
    unittest.main()
