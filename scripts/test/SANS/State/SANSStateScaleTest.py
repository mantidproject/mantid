import unittest
import mantid

from SANS2.State.SANSStateScale import (SANSStateScale, SANSStateScaleISIS)


class SANSStateScaleTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateScaleISIS()
        self.assertTrue(isinstance(state, SANSStateScale))


if __name__ == '__main__':
    unittest.main()
