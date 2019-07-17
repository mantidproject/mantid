from __future__ import absolute_import, print_function

import unittest
import matplotlib
from qtpy.QtGui import QCloseEvent

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

DEBUG = True


class ElementalAnalysisTest(GuiTest):
    def setUp(self):
        self.gui = ElementalAnalysisGui()

    def tearDown(self):
        self.gui = None

    @unittest.skipIf(DEBUG, 'Only for debug')
    def test_that_default_colour_cycle_is_used(self):
        cycle = len(matplotlib.rcParams['axes.prop_cycle'])
        self.assertEqual(cycle, self.gui.num_colors)

        expected = ['C%d' % i for i in range(cycle)]
        returned = [self.gui.get_color() for _ in range(cycle)]
        self.assertEqual(expected, returned)

    @unittest.skipIf(DEBUG, 'Only for debug')
    def test_color_is_increased_every_time_get_color_is_called(self):
        self.assertEqual(self.gui.color_index, 0)
        self.gui.get_color()
        self.assertEqual(self.gui.color_index, 1)

    @unittest.skipIf(DEBUG, 'Only for debug')
    def test_that_color_index_wraps_around_when_end_reached(self):
        self.gui.color_index = self.gui.num_colors - 1
        self.gui.get_color()

        self.assertEqual(self.gui.color_index, 0)

    @unittest.skipIf(DEBUG, 'Only for debug')
    def test_that_closing_with_no_plot_will_not_throw(self):
        self.gui.plot_window = None
        self.gui.closeEvent(QCloseEvent())

    @unittest.skipIf(DEBUG, 'Only for debug')
    def test_that_closing_with_a_plot_will_close_the_window(self):
        self.gui.plot_window = mock.create_autospec(MultiPlotWindow)
        self.gui.closeEvent(QCloseEvent())

        self.assertEqual(self.gui.plot_window.closeEvent.call_count, 1)

    @unittest.skipIf(DEBUG, 'Only for debug')
    def test_that_gen_label_does_not_throw_with_non_float_x_values(self):
        self.gui._gen_label('name', 'not_a_float', 'Cu')
        self.gui._gen_label('name', u'not_a_float', 'Cu')
        self.gui._gen_label('name', None, 'Cu')
        self.gui._gen_label('name', ('not', 'a', 'float'), 'Cu')
        self.gui._gen_label('name', ['not', 'a', 'float'], 'Cu')

    @unittest.skipIf(DEBUG, 'Only for debug')
    def test_that_gen_label_with_element_none_returns_none(self):
        self.assertEqual(self.gui._gen_label('label', 0.0), None)

    @unittest.skipIf(DEBUG, 'Only for debug')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.Label')
    def test_that_gen_label_name_is_appended_to_list_if_not_present(self, mock_label):
        element = 'Cu'
        name = u'new_label'
        self.gui.element_lines = {'Cu': [u'old_label']}
        self.gui._gen_label(name, 1.0, element)

        self.assertIn(name, self.gui.element_lines[element])
        mock_label.assert_called_with(str(name), 1.0, False, 0.9, True, rotation=-90, protected=True)

    @unittest.skipIf(DEBUG, 'Only for debug')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.Label')
    def test_that_gen_label_name_is_not_duplicated_in_list_if_already_present(self, mock_label):
        element = 'Cu'
        name = u'old_label'
        self.gui.element_lines = {'Cu': [u'old_label']}
        self.gui._gen_label(name, 1.0, element)

        self.assertIn(name, self.gui.element_lines[element])
        self.assertEqual(self.gui.element_lines[element].count(name), 1)
        mock_label.assert_called_with(str(name), 1.0, False, 0.9, True, rotation=-90, protected=True)

    @unittest.skipIf(DEBUG, 'Only for debug')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._gen_label')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._plot_line_once')
    def test_that_plot_line_returns_if_plot_window_is_none(self, mock_plot_line_once, mock_gen_label):
        self.gui.plot_window = None
        mock_gen_label.return_value = 'name of the label'
        self.gui._plot_line('name', 1.0, 'C0', None)

        self.assertEqual(mock_plot_line_once.call_count, 0)

    @unittest.skipIf(DEBUG, 'Only for debug')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.MultiPlotWidget.get_subplots')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._gen_label')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._plot_line_once')
    def test_that_plot_line_calls_plot_line_once_if_window_not_none(self, mock_plot_line_once,
                                                                    mock_gen_label, mock_get_subplot):
        self.gui.plot_window = mock.create_autospec(MultiPlotWindow)
        self.gui.plotting = MultiPlotWidget(mock.Mock())
        mock_gen_label.return_value = 'name of the label'
        mock_get_subplot.return_value = ['plot1']
        self.gui._plot_line('name', 1.0, 'C0', None)

        self.assertEqual(mock_plot_line_once.call_count, 1)
        mock_plot_line_once.assert_called_with('plot1', 1.0, 'name of the label', 'C0')

    @unittest.skipIf(DEBUG, 'Only for debug')
    def test_plot_line_once_calls_correct_multiplot_function(self):
        self.gui.plotting = mock.create_autospec(MultiPlotWidget)
        self.gui._plot_line_once('GE1', 1.0, 'label', 'C0')

        self.assertEqual(self.gui.plotting.add_vline_and_annotate.call_count, 1)

    @unittest.skipIf(DEBUG, 'Only for debug')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.MultiPlotWidget.get_subplots')
    def test_that_rm_line_returns_if_plot_window_is_none(self, mock_get_subplot):
        self.gui.plotting = MultiPlotWidget(mock.Mock())
        self.gui.plot_window = None
        mock_get_subplot.return_value = ['plot1', 'plot2', 'plot3']

        self.gui._rm_line('line')

        self.assertEqual(mock_get_subplot.call_count, 0)

    @unittest.skipIf(DEBUG, 'Only for debug')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.MultiPlotWidget.rm_vline_and_annotate')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.MultiPlotWidget.get_subplots')
    def test_that_rm_line_calls_correct_function_if_window_not_none(self, mock_get_subplot, mock_rm_vline):
        self.gui.plotting = MultiPlotWidget(mock.Mock())
        self.gui.plot_window = mock.create_autospec(MultiPlotWindow)
        mock_get_subplot.return_value = ['plot1', 'plot2', 'plot3']

        self.gui._rm_line('line')

        self.assertEqual(mock_get_subplot.call_count, 1)
        self.assertEqual(mock_rm_vline.call_count, 3)
        mock_rm_vline.assert_called_with('plot3', 'line')

    @unittest.skipIf(DEBUG, 'Only for debug')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.get_color')
    def test_that_plotting_lines_will_call_get_color(self, mock_get_color):
        self.gui._add_element_lines('Cu')
        self.assertEqual(mock_get_color.call_count, 1)

    @unittest.skipIf(DEBUG, 'Only for debug')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._plot_line')
    def test_that_plotting_lines_will_call_plot_line(self, mock_plot_line):
        data = {'line1': 10.0, 'line2': 20.0, 'line3': 30.0}
        self.gui._add_element_lines('Cu', data)

        self.assertEqual(mock_plot_line.call_count, 3)
        mock_plot_line.assert_called_with(gen_name('Cu', 'line1'), 10.0, 'C0', 'Cu')


if __name__ == '__main__':
    unittest.main()