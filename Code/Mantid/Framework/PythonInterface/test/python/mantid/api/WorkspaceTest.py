import unittest
from testhelpers import run_algorithm
from mantid.api import Workspace

class WorkspaceTest(unittest.TestCase):

    def test_that_one_cannot_be_instantiated(self):
        try:
            Workspace()
            error = False
        except RuntimeError: # For some reason self.assertRaises doesn't catch this
            error = True
        self.assertTrue(error, True)

if __name__ == '__main__':
    unittest.main()
