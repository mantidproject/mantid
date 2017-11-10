from __future__ import (absolute_import, division, print_function)
import unittest
from paraview.simple import *


class PVPythonTest(unittest.TestCase):

    def test_PVPython(self):
        self.assertEqual(GetParaViewVersion().major, 5)

if __name__ == '__main__':
    unittest.main()
