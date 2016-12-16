import unittest
import mantid

from SANS2.State.SANSStateReduction import SANSStateReductionISIS
from SANS2.Common.SANSEnumerations import SANSReductionType


class SANSStateDataTest(unittest.TestCase):
    def test_that_reduction_state_gets_and_sets(self):
        # Arrange
        state = SANSStateReductionISIS()
        # Act
        state.reduction_type = SANSReductionType.Hab
        # Assert
        self.assertTrue(state.reduction_type is SANSReductionType.Hab)


if __name__ == '__main__':
    unittest.main()
