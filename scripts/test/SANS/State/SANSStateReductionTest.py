import unittest
import mantid

from SANS2.State.SANSStateReduction import (SANSStateReductionISIS, SANSReductionType)


class SANSStateDataTest(unittest.TestCase):
    def test_that_reduction_state_gets_and_sets(self):
        # Arrange
        state = SANSStateReductionISIS()
        # Act
        state.reduction_type = SANSReductionType.Front
        # Assert
        self.assertTrue(state.reduction_type is SANSReductionType.Front)


if __name__ == '__main__':
    unittest.main()
