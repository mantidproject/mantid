from __future__ import absolute_import, print_function

import unittest
import matplotlib

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest

from Muon.GUI.ElementalAnalysis.elemental_analysis import ElementalAnalysisGui
from Muon.GUI.ElementalAnalysis.elemental_analysis import gen_name

from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_presenter import PeriodicTablePresenter
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_view import PeriodicTableView
from Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_model import PeriodicTableModel

from MultiPlotting.multi_plotting_widget import MultiPlotWindow
from MultiPlotting.multi_plotting_widget import MultiPlotWidget
from MultiPlotting.label import Label

from Muon.GUI.ElementalAnalysis.LoadWidget.load_model import LoadModel, CoLoadModel
from Muon.GUI.Common.load_widget.load_view import LoadView
from Muon.GUI.Common.load_widget.load_presenter import LoadPresenter

from Muon.GUI.ElementalAnalysis.Detectors.detectors_presenter import DetectorsPresenter
from Muon.GUI.ElementalAnalysis.Detectors.detectors_view import DetectorsView
from Muon.GUI.ElementalAnalysis.Peaks.peaks_presenter import PeaksPresenter
from Muon.GUI.ElementalAnalysis.Peaks.peaks_view import PeaksView

from Muon.GUI.ElementalAnalysis.PeriodicTable.PeakSelector.peak_selector_presenter import PeakSelectorPresenter
from Muon.GUI.ElementalAnalysis.PeriodicTable.PeakSelector.peak_selector_view import PeakSelectorView

from Muon.GUI.Common import message_box


class ElementalAnalysisTest(GuiTest):
    def setUp(self):
        self.gui = ElementalAnalysisGui()

    def tearDown(self):
        self.gui = None

    def test_that_default_colour_cycle_is_used(self):
        cycle = len(matplotlib.rcParams['axes.prop_cycle'])
        self.assertEqual(cycle, self.gui.num_colors)

        expected = ['C%d' % i for i in range(cycle)]
        returned = [self.gui.get_color() for _ in range(cycle)]
        self.assertEqual(expected, returned)

    def test_color_is_increased_every_time_get_color_is_called(self):
        self.assertEqual(self.gui.color_index, 0)
        self.gui.get_color()
        self.assertEqual(self.gui.color_index, 1)

    def test_that_color_index_wraps_around_when_end_reached(self):
        self.gui.color_index = self.gui.num_colors - 1
        self.gui.get_color()

        self.assertEqual(self.gui.color_index, 0)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.get_color')
    def test_that_plotting_lines_will_call_get_color(self, mock_get_color):
        self.gui._add_element_lines('Cu')
        self.assertEqual(mock_get_color.call_count, 1)

    def test_plot_line_once_calls_correct_multiplot_function(self):
        self.gui.plotting = mock.create_autospec(MultiPlotWidget)
        self.gui._plot_line_once('GE1', 1.0, 'label', 'C0')

        self.assertEqual(self.gui.plotting.add_vline_and_annotate.call_count, 1)


if __name__ == '__main__':
    unittest.main()