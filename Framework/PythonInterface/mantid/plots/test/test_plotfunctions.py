import unittest

from mock import Mock

from mantid.plots.plotfunctions import MantidAxType, _get_data_for_plot
from mantid.simpleapi import CreateSampleWorkspace


class MockMantidAxes:
    def __init__(self):
        self.set_xlabel = Mock()


class PlotfunctionsTest(unittest.TestCase):
    def test_get_data_for_plot(self):
        mock_axes = MockMantidAxes()
        ws = CreateSampleWorkspace(NumBanks=1, BankPixelWidth=4, NumEvents=10)
        kwargs = {"axis": MantidAxType.SPECTRUM,
                  "wkspIndex": 1}
        x, y, dy, dx, kwargs = _get_data_for_plot(mock_axes, kwargs, ws)
