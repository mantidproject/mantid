import unittest
import mantid

from Move.SANSMove import SANSMoveFactory
from State.SANSState import SANSStateISIS
from State.SANSStateData import SANSStateDataISIS
from State.SANSStateMoveWorkspace import (SANSStateMoveWorkspaceLOQ)


def create_valid_LOQ_state():
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



class SANSMoveFactoryTest(unittest.TestCase):
    def test_that_LOQ_strategy_is_selected(self):
        pass

    def test_that_SANS2D_strategy_is_selected(self):
        pass

    def test_that_SANS2D_TUBES_strategy_is_selected(self):
        pass

    def test_that_LARMOR_strategy_is_selected(self):
        pass

    def test_that_LARMOR_8Tubes_strategy_is_selected(self):
        pass


class SANSMoveTest(unittest.TestCase):
    def test_that_LOQ_can_be_moved(self):
        # Arrange
        move_factory = SANSMoveFactory()

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
