# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from paraview.simple import *


class PVPythonTest(unittest.TestCase):

    def test_PVPython(self):
        self.assertEqual(GetParaViewVersion().major, 5)

if __name__ == '__main__':
    unittest.main()
