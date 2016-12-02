"""
Test one can import paraview.simple in MantidPlot
"""
import mantidplottests
from mantidplottests import *


class MantidPlotPVPythonTest(unittest.TestCase):

    def setUp(self):
        pass

    def tearDown(self):
        pass

    def test_PVPython(self):
        from paraview.simple import *
        self.assertEqual(GetParaViewVersion().major, 5)

# Run the unit tests
mantidplottests.runTests(MantidPlotPVPythonTest)
