from __future__ import (absolute_import, division, print_function)
import unittest
from paraview.simple import *

class PVPythonTest(unittest.TestCase):

    def test_PVPython(self):
        self.assertEquals(str(GetParaViewVersion()),'5.1')

if __name__ == '__main__':
    unittest.main()

