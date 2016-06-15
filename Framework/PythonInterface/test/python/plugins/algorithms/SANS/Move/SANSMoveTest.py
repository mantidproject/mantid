import unittest
import mantid

from Move.SANSMove import SANSMoveFactory
from State.SANSStateMoveWorkspace import SANSStateMoveWorkspace


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
