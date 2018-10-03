# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
"""
Test one can import paraview.simple in MantidPlot
"""
import mantidplottests
from mantidplottests import *
from paraview.simple import *


class MantidPlotPVPythonTest(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_PVPython(self):
        self.assertEqual(GetParaViewVersion().major, 5)

# Run the unit tests
mantidplottests.runTests(MantidPlotPVPythonTest)
