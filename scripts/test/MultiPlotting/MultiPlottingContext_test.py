# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.py3compat import mock
from MultiPlotting.multi_plotting_context import PlottingContext
from MultiPlotting.subplot.subplot_context import subplotContext
from line_helper import line

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
        self.subplot = mock.create_autospec(subplotContext)
        with mock.patch("MultiPlotting.subplot.subplot_context.subplotContext.addLine") as patch:
            self.context.addSubplot("one",subplot) 
            self.context.addLine("one",ws,specNum)
            self.assertEqual(patch.call_count,1)
            patch.assert_called_with(ws,specNum)

    def test_add_line_2(self):
        specNum = 4
        mockWS = mock.MagicMock()
        ws = gen_ws(mockWS)
        # add mock subplot
        subplot = mock.MagicMock()
        self.subplot = mock.create_autospec(subplotContext)
        with mock.patch("MultiPlotting.subplot.subplot_context.subplotContext.addLine") as patch:
            self.context.addSubplot("one",subplot) 
            self.context.addLine("one",ws,specNum)
            self.assertEqual(patch.call_count,1)
            patch.assert_called_with(mockWS,specNum)

    def test_updateLayout(self):
        # add mocks
        figure = mock.Mock()
        self.subplot = mock.create_autospec(subplotContext)
        names = ["one","two","three"]
        for name in names:
            self.context.addSubplot(name, mock.Mock())

        gridspec = mock.Mock()
        self.context._gridspec = gridspec
        with mock.patch("MultiPlotting.subplot.subplot_context.subplotContext.update_gridspec") as patch:
            self.context.update_layout(figure)
            self.assertEqual(patch.call_count,3)
            # only last iteration survives
            patch.assert_called_with(gridspec,figure,2)


    def test_subplotEmptyTrue(self):
        names = ["one","two","three"]
        for name in names:
            self.context.addSubplot(name, mock.Mock())

        for name in names:
             self.assertEqual(self.context.is_subplot_empty(name),True)
 
    def test_subplotEmptyFalse(self):
        names = ["one","two","three"]

        no_lines = 1

        ws = mock.MagicMock()
        with mock.patch("mantid.plots.plotfunctions.plot") as patch:
            patch.return_value = tuple([line()])
 
            for name in names:
                self.context.addSubplot(name, mock.Mock())
                for k in range(0,no_lines):
                    self.context.addLine(name, ws,1)
                no_lines +=1

        for name in names:
             self.assertEqual(self.context.is_subplot_empty(name),False)
        
if __name__ == "__main__":
    unittest.main()
