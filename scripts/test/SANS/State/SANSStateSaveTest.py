import unittest
import mantid

from SANS2.State.SANSStateSave import (SANSStateSave, SANSStateSaveISIS)


class SANSStateReductionTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateSaveISIS()
        self.assertTrue(isinstance(state, SANSStateSave))


if __name__ == '__main__':
    unittest.main()
