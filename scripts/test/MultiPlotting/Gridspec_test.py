# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from matplotlib.gridspec import GridSpec
import unittest

from MultiPlotting.gridspec_engine import gridspecEngine


class GridSpecTest(unittest.TestCase):
    """ We do not need to test 
        the layout from the gridspec 
        as it is tested elsewhere    """
    def test_noMax(self):
        self.engine = gridspecEngine()
        result = self.engine.getGridSpec(100)
        self.assertNotEquals(result, None)
 
    def test_underMax(self):
        self.engine = gridspecEngine(max_plot=10)
        result = self.engine.getGridSpec(8)
        self.assertNotEquals(result, None)

    def test_overMax(self):
        self.engine = gridspecEngine(max_plot=10)
        result = self.engine.getGridSpec(100)
        self.assertEquals(result, None)

    def test_returnType(self):
        gridspec = GridSpec(1,1)
        self.engine = gridspecEngine()
        result = self.engine.getGridSpec(1)
        self.assertEquals(type(result),type(gridspec))


if __name__ == "__main__":
    unittest.main()
