# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from MultiPlotting.multiPlotting_context import PlottingContext
from MultiPlotting.subplot.subPlot_context import subPlotContext


try:
    from unittest import mock
except ImportError:
    import mock

class gen_ws(object):
    def __init__(self,mock):
       self._input = "in"
       self._OutputWorkspace = mock

    def __len__(self):
        return 2

    @property
    def OutputWorkspace(self):
        return self._OutputWorkspace

class MultiPlottingContextTest(unittest.TestCase):
    def setUp(self):
        self.context = PlottingContext()
 
    def test_add_line_1(self):
        specNum = 4
        ws = mock.MagicMock()
        # add mock subplot
        subplot = mock.MagicMock()
        self.subplot = mock.create_autospec(subPlotContext)
        with mock.patch("MultiPlotting.subplot.subPlot_context.subPlotContext.addLine") as patch:
            self.context.addSubplot("one",subplot) 
            self.context.addLine("one",ws,specNum)
            self.assertEquals(patch.call_count,1)
            patch.assert_called_with(ws,specNum)

    def test_add_line_2(self):
        specNum = 4
        mockWS = mock.MagicMock()
        ws = gen_ws(mockWS)
        # add mock subplot
        subplot = mock.MagicMock()
        self.subplot = mock.create_autospec(subPlotContext)
        with mock.patch("MultiPlotting.subplot.subPlot_context.subPlotContext.addLine") as patch:
            self.context.addSubplot("one",subplot) 
            self.context.addLine("one",ws,specNum)
            self.assertEquals(patch.call_count,1)
            patch.assert_called_with(mockWS,specNum)


if __name__ == "__main__":
    unittest.main()
