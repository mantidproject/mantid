import unittest
from mantid.dataobjects import Workspace2D

class Workspace2DTest(unittest.TestCase):

    def test_class_cannot_be_instantiated_in_python(self):
        self.assertRaises(RuntimeError, Workspace2D)

if __name__ == '__main__':
    unittest.main()
