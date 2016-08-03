import unittest
import mantid

from SANS2.State.StateDirector.TestDirector import TestDirector


class SANSMaskWorkspaceTest(unittest.TestCase):
    def test_that_SANS2D_workspace_is_masked_correctly(self):


        # Get the sample state
        test_director = TestDirector()
        test_director.set_states(data_state=data_info)

    def test_that_LOQ_workspace_is_masked_correctly(self):
        pass


if __name__ == '__main__':
    unittest.main()
