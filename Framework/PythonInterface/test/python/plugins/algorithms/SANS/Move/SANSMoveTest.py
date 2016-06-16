import unittest
import mantid

from mantid.api import AlgorithmManger

from Move.SANSMove import SANSMoveFactory
from State.SANSState import SANSStateISIS
from State.SANSStateData import SANSStateDataISIS
from State.SANSStateMoveWorkspace import (SANSStateMoveWorkspaceLOQ, SANSStateMoveWorkspaceSANS2D,
                                          SANSMoveLARMORNewStyle, SANSMoveLARMOROldStyle)
from Common.SANSConstants import SANSConstants


def create_valid_state(scatter_file_name):
    state = SANSStateISIS()

    # Add the different descriptors of the SANSState here:
    data = SANSStateDataISIS()
    data.sample_scatter = "sample_scat"
    state.data = data

    # Add the move
    move = SANSStateMoveWorkspaceLOQ()
    move.detectors[SANSConstants.high_angle_bank].detector_name = "high-angle"
    move.detectors[SANSConstants.high_angle_bank].detector_name_short = "test"
    move.detectors[SANSConstants.low_angle_bank].detector_name = "test"
    move.detectors[SANSConstants.low_angle_bank].detector_name_short = "test"
    state.move = move
    return state


def load_file(file_name):
    alg = AlgorithmManger.createUnmanaged("Load")
    alg.initialize()
    alg.setChild(True)
    alg.setProperty("Filename", file_name)
    alg.setProperty("OutputWorkspace", "dummy")
    alg.execute()
    return alg.getProperty("OutputWorkspace").value


class SANSMoveFactoryTest(unittest.TestCase):
    def _do_test(self, file_name, mover_type):
        # Arrange
        workspace = load_file(file_name)
        move_factory = SANSMoveFactory()
        # Act
        mover = move_factory.create_mover(workspace)
        # Assert
        self.assertTrue(isinstance(mover, mover_type))

    def test_that_LOQ_strategy_is_selected(self):
        file_name = None
        mover_type = SANSStateMoveWorkspaceLOQ
        self._do_test(file_name, mover_type)

    def test_that_SANS2D_strategy_is_selected(self):
        file_name = None
        mover_type = SANSStateMoveWorkspaceSANS2D
        self._do_test(file_name, mover_type)

    def test_that_LARMOR_strategy_is_selected(self):
        file_name = None
        mover_type = SANSMoveLARMORNewStyle
        self._do_test(file_name, mover_type)

    def test_that_LARMOR_8Tubes_strategy_is_selected(self):
        file_name = None
        mover_type = SANSMoveLARMOROldStyle
        self._do_test(file_name, mover_type)


class SANSMoveTest(unittest.TestCase):
    def test_that_LOQ_can_be_moved(self):
        pass

    def test_that_SANS2D_can_be_moved(self):
        pass

    def test_that_SANS2D_TUBES_can_be_moved(self):
        pass

    def test_that_LARMOR_can_be_moved(self):
        pass

    def test_that_LARMOR_8Tubes_can_be_moved(self):
        pass


if __name__ == '__main__':
    unittest.main()
