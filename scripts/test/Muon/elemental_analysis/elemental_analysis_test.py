# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import copy
import unittest

from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from testhelpers import assertRaisesNothing

from qtpy.QtGui import QCloseEvent

from Muon.GUI.ElementalAnalysis.elemental_analysis import ElementalAnalysisGui
from Muon.GUI.ElementalAnalysis.LoadWidget.load_utils import spectrum_index
from MultiPlotting.multi_plotting_widget import MultiPlotWindow


@start_qapplication
class ElementalAnalysisTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        super(ElementalAnalysisTest, cls).setUpClass()
        cls.gui = ElementalAnalysisGui()

    @classmethod
    def tearDownClass(cls):
        cls.gui = None

    def setUp(self):
        self.gui.plot_window = None
        self.gui.used_colors = {}
        self.gui.element_lines = {}
        self.has_raise_ValueError_been_called_once = False
        self.gui.detectors = mock.Mock()
        self.gui.detectors.detectors = [mock.Mock(), mock.Mock(), mock.Mock(), mock.Mock()]
        self.gui.lines = mock.Mock()

    def raise_ValueError_once(self):
        if not self.has_raise_ValueError_been_called_once:
            self.has_raise_ValueError_been_called_once = True
            raise ValueError()

    def test_that_closing_with_no_plot_will_not_throw(self):
        self.gui.plot_window = None

        assertRaisesNothing(self, self.gui.closeEvent, QCloseEvent())

    def test_that_closing_with_a_plot_will_close_the_window(self):
        self.gui.plot_window = mock.create_autospec(MultiPlotWindow)
        self.gui.closeEvent(QCloseEvent())

        self.assertEqual(self.gui.plot_window.closeEvent.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.MultiPlotWindow')
    def test_load_run_opens_new_plot_window_if_none_open(self, mock_multi_plot_window):
        self.gui.add_detector_to_plot = mock.Mock()
        self.gui.load_run('GE1', '2695')
        self.assertEqual(mock_multi_plot_window.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.MultiPlotWindow')
    def test_load_run_does_not_open_new_plot_window_if_one_is_open(self, mock_multi_plot_window):
        self.gui.add_detector_to_plot = mock.Mock()
        self.gui.plot_window = MultiPlotWindow(str('2695'))
        self.gui.load_run('GE1', 2695)
        self.assertEqual(mock_multi_plot_window.call_count, 0)

    @mock.patch('Muon.GUI.ElementalAnalysis.Detectors.detectors_view.QtWidgets.QWidget')
    def test_loading_finished_returns_nothing_if_no_run_loaded(self, mock_qwidget):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=None)
        mock_qwidget.return_value = True
        self.gui.plot_window = mock.Mock()
        self.gui.plotting = mock.Mock()

        self.gui.loading_finished()
        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 0)

    @mock.patch('Muon.GUI.ElementalAnalysis.Detectors.detectors_view.QtWidgets.QWidget')
    def test_loading_finished_returns_correctly_if_no_plot_window_but_has_to_plot(self, mock_qwidget):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        self.gui.load_widget.get_run_num_loaded_detectors = mock.Mock(return_value=4)
        self.gui.detectors.getNames.return_value = ['1', '2', '3']
        self.gui.plot_window = None
        self.gui.plotting = mock.Mock()
        self.gui.plotting.get_subplots.return_value = ['1', '2', '3']
        mock_qwidget.return_value = True

        self.gui.loading_finished()
        self.assertEqual(self.gui.detectors.setStateQuietly.call_count, 3)
        for detector in self.gui.detectors.detectors:
            self.assertEqual(detector.setChecked.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.Detectors.detectors_view.QtWidgets.QWidget')
    def test_loading_finished_returns_correctly_if_no_to_plot_but_has_plot_window(
            self, mock_qwidget):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        self.gui.load_widget.get_run_num_loaded_detectors = mock.Mock(return_value=4)
        self.gui.detectors.getNames.return_value = ['1', '2', '3']
        mock_qwidget.return_value = True
        self.gui.plot_window = mock.Mock()

        self.gui.loading_finished()
        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 3)

    @mock.patch('Muon.GUI.ElementalAnalysis.Detectors.detectors_view.QtWidgets.QWidget')
    def test_loading_finished_correctly_disables_detectors_if_less_detectors_are_loaded(
            self, mock_qwidget):
        num_loaded_detectors = 1
        num_detectors = 4
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        self.gui.load_widget.get_run_num_loaded_detectors = mock.Mock(return_value=num_loaded_detectors)
        self.gui.detectors.getNames.return_value = ['1', '2', '3', '4']
        self.gui.plotting.get_subplots.return_value = ['1', '2', '3', '4']
        mock_qwidget.return_value = True
        self.gui.plot_window = mock.Mock()

        self.gui.loading_finished()
        # should have set the states of num_detectors - num_loaded_detectors
        self.assertEqual(self.gui.detectors.setStateQuietly.call_count, num_detectors - num_loaded_detectors)
        # should have only enabled the detector we have loaded
        self.assertEqual(self.gui.detectors.enableDetector.call_count, num_loaded_detectors)
        # Should disable (num_detectors - num_loaded_detectors) detectors
        self.assertEqual(self.gui.detectors.disableDetector.call_count, num_detectors - num_loaded_detectors)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.PeriodicTableWidgetPresenter.add_peak_data')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.mantid')
    def test_add_detectors_to_plot_plots_all_given_ws_and_all_selected_elements(
            self, mock_mantid, mock_add_peak_data):
        mock_mantid.mtd = {
            'name1': mock.Mock(),
            'name2': mock.Mock(),
        }
        self.gui.plotting = mock.Mock()
        self.gui.lines = mock.Mock()
        self.gui.lines.total.isChecked.return_value = True
        self.gui.lines.prompt.isChecked.return_value = False
        self.gui.lines.delayed.isChecked.return_value = True
        mock_mantid.mtd['name1'].name.return_value = 'Detector 1'
        self.gui.add_detector_to_plot('GE1', 'name1')
        self.assertEqual(self.gui.plotting.add_subplot.call_count, 1)
        self.assertEqual(self.gui.plotting.plot.call_count, 2)
        self.assertEqual(mock_add_peak_data.call_count, 0)

    def test_unset_detectors_resets_plot_window_and_detectors(self):
        self.gui.plot_window = mock.Mock()
        self.gui.detectors = mock.Mock()
        self.gui.detectors.getNames.return_value = ['name1', 'name2', 'name3']
        self.gui._unset_detectors()
        self.assertEqual(self.gui.detectors.setStateQuietly.call_count, 3)
        self.assertEqual(self.gui.plot_window, None)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.load_run')
    def test_add_plot_does_nothing_is_no_loaded_run(self, mock_load_run):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=None)
        self.gui.add_plot(mock.Mock())
        self.assertEqual(mock_load_run.call_count, 0)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.PeriodicTableWidgetPresenter.add_peak_data')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.load_run')
    def test_add_plot_loads_run_and_electron_peaks_not_plotted(self, mock_load_run, add_peak_data):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        self.gui.ptable.peakpresenter.electron.isChecked = mock.Mock(return_value=False)
        mock_checkbox = mock.Mock()
        mock_checkbox.name = 'GE1'

        self.gui.add_plot(mock_checkbox)

        mock_load_run.assert_called_with('GE1', 2695)
        self.assertEqual(add_peak_data.call_count, 0)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.load_run')
    def test_add_plot_loads_run(self, mock_load_run):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        mock_checkbox = mock.Mock()
        mock_checkbox.name = 'GE1'

        self.gui.add_plot(mock_checkbox)

        mock_load_run.assert_called_with('GE1', 2695)

    def test_del_plot_does_nothing_if_no_loaded_run(self):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=None)
        self.gui.plotting.remove_subplot = mock.Mock()
        mock_checkbox = mock.Mock()
        mock_checkbox.name = 'GE1'

        self.gui.del_plot(mock_checkbox)

        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 0)

    def test_del_plot_removes_subplot_only_if_other_subplots_exist(self):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        self.gui.plotting.remove_subplot = mock.Mock()
        self.gui.plotting.get_subplots = mock.Mock(return_value=True)
        self.gui.plot_window = 'plot_window'
        mock_checkbox = mock.Mock()
        mock_checkbox.name = 'GE1'

        self.gui.del_plot(mock_checkbox)

        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 1)
        self.assertEqual(self.gui.plot_window, 'plot_window')

    def test_del_plot_closes_plot_if_no_subplots_left(self):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        self.gui.plotting = mock.Mock()
        self.gui.plotting.remove_subplot = mock.Mock()
        self.gui.plotting.get_subplots = mock.Mock(return_value=False)
        self.gui.plot_window = mock.Mock()
        mock_checkbox = mock.Mock()
        mock_checkbox.name = 'GE1'

        self.gui.del_plot(mock_checkbox)

        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 1)
        self.assertEqual(self.gui.plot_window, None)

    def test_subplot_removed_changes_state_only_if_other_subplots_exist(self):
        self.gui.detectors.setStateQuietly = mock.Mock()
        self.gui.plotting.get_subplots = mock.Mock(return_value=True)
        self.gui.plot_window = 'plot_window'

        self.gui.subplot_removed('name')

        self.assertEqual(self.gui.detectors.setStateQuietly.call_count, 1)
        self.assertEqual(self.gui.plot_window, 'plot_window')

    def test_subplot_removed_closes_plot_if_no_other_subplots_exist(self):
        self.gui.detectors.setStateQuietly = mock.Mock()
        self.gui.plotting.get_subplots = mock.Mock(return_value=False)
        self.gui.plot_window = mock.Mock()

        self.gui.subplot_removed('name')

        self.assertEqual(self.gui.detectors.setStateQuietly.call_count, 1)
        self.assertEqual(self.gui.plot_window, None)

    def test_that_add_line_by_type_enables_all_detectors(self):
        self.gui.detectors.detectors = [mock.Mock(), mock.Mock(), mock.Mock(), mock.Mock()]
        for detector in self.gui.detectors.detectors:
            detector.isEnabled.return_value = False

        self.gui.add_line_by_type(2695, 'Total')

        for detector in self.gui.detectors.detectors:
            detector.setEnabled.assert_called_with(True)

    def texst_that_add_line_by_type_returns_is_plot_window_is_none(self):
        self.gui.plotting = mock.Mock()
        self.gui.add_line_by_type(2695, 'Total')

        self.assertEqual(0, self.gui.plotting.get_subplots.call_count)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.mantid')
    def test_that_add_line_by_type_plots_correct_lines_with_correct_colour(self, mock_mantid):
        self.gui.plotting = mock.Mock()
        self.gui.plot_window = mock.Mock()
        self.gui.plotting.get_subplots.return_value = ['1', '2']
        mock_mantid.mtd = {
            '2695; Detector 1': mock.Mock(),
            '2695; Detector 2': mock.Mock(),
            '2695; Detector 3': mock.Mock()
        }
        mock_mantid.mtd['2695; Detector 1'].name.return_value = '2695; Detector 1'
        mock_mantid.mtd['2695; Detector 2'].name.return_value = '2695; Detector 2'
        expected_calls = [
            mock.call('1', '2695; Detector 1', color='C0', spec_num=spectrum_index['Total']),
            mock.call('2', '2695; Detector 2', color='C0', spec_num=spectrum_index['Total'])
        ]
        self.gui.add_line_by_type(2695, 'Total')

        self.assertEqual(1, self.gui.plotting.get_subplots.call_count)
        self.gui.plotting.plot.assert_has_calls(expected_calls)

    def test_remove_line_type_returns_if_no_plot_open(self):
        self.gui.plot_window = None
        self.gui.plotting = mock.Mock()
        self.gui.remove_line_type(2695, 'Total')

        self.assertEqual(0, self.gui.plotting.get_subplots.call_count)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.mantid')
    def test_remove_line_type_calls_remove_line(self, mock_mantid):
        self.gui.plot_window = mock.Mock()
        self.gui.plotting = mock.Mock()
        self.gui.plotting.get_subplots.return_value = ['1', '2']
        mock_mantid.mtd = {
            '2695; Detector 1': mock.Mock(),
            '2695; Detector 2': mock.Mock(),
            '2695; Detector 3': mock.Mock()
        }
        mock_mantid.mtd['2695; Detector 1'].name.return_value = '2695; Detector 1'
        mock_mantid.mtd['2695; Detector 2'].name.return_value = '2695; Detector 2'
        expected_calls = [mock.call('1', '2695; Detector 1', spec=spectrum_index['Total']),
                          mock.call('2', '2695; Detector 2', spec=spectrum_index['Total'])]
        self.gui.remove_line_type(2695, 'Total')

        self.assertEqual(1, self.gui.plotting.get_subplots.call_count)
        self.gui.plotting.remove_line.assert_has_calls(expected_calls)

    def test_that_uncheck_detectors_if_no_line_plotted_unchecks_all_detectors(self):
        self.gui.lines.total.isChecked.return_value = False
        self.gui.lines.prompt.isChecked.return_value = False
        self.gui.lines.delayed.isChecked.return_value = False

        self.gui.uncheck_detectors_if_no_line_plotted()

        for detector in self.gui.detectors.detectors:
            detector.setEnabled.assert_called_with(False)
            self.assertEqual(1, detector.setEnabled.call_count)

    def test_that_uncheck_detectors_if_no_line_plotted_does_not_uncheck_if_plotting_some_lines(
            self):
        self.gui.lines.total.isChecked.return_value = False
        self.gui.lines.prompt.isChecked.return_value = True
        self.gui.lines.delayed.isChecked.return_value = False

        self.gui.uncheck_detectors_if_no_line_plotted()

        for detector in self.gui.detectors.detectors:
            self.assertEqual(0, detector.setEnabled.call_count)

    def test_that_uncheck_on_removed_uncheck_correct_lines(self):
        rem_lines = ['line Total', 'line Prompt', 'line 3']
        tmp_detectors = copy.deepcopy(self.gui.detectors.detectors)
        for detector in self.gui.detectors.detectors:
            detector.isChecked.return_value = False
        self.gui.uncheck_on_removed(rem_lines)

        self.gui.lines.total.setChecked.assert_called_with(False)
        self.gui.lines.prompt.setChecked.assert_called_with(False)
        self.assertEqual(0, self.gui.lines.delayed.setChecked.call_count)

        self.gui.detectors.detectors = tmp_detectors

    def test_that_uncheck_on_removed_blocks_signals_and_calls_right_function(self):
        rem_lines = ['line Total', 'line Prompt', 'line 3']
        calls = [mock.call(True), mock.call(False)]
        self.gui.uncheck_detectors_if_no_line_plotted = mock.Mock()
        tmp_detectors = copy.deepcopy(self.gui.detectors.detectors)
        for detector in self.gui.detectors.detectors:
            detector.isChecked.return_value = False

        self.gui.uncheck_on_removed(rem_lines)

        self.gui.lines.total.blockSignals.assert_has_calls(calls)
        self.gui.lines.prompt.blockSignals.assert_has_calls(calls)
        self.gui.lines.delayed.blockSignals.assert_has_calls([])
        self.assertEqual(1, self.gui.uncheck_detectors_if_no_line_plotted.call_count)

        self.gui.detectors.detectors = tmp_detectors

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.add_line_by_type')
    def test_line_total_checked_checks_line_and_calls_add_line_by_type(self, mock_add_line_by_type):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        mock_line = mock.Mock()
        mock_line.isChecked.return_value = True
        self.gui.line_total_changed(mock_line)

        self.gui.lines.total.setChecked.assert_called_with(True)
        self.assertEqual(1, mock_add_line_by_type.call_count)

    @mock.patch(
        'Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.add_line_by_type')
    def test_line_prompt_checked_checks_line_and_calls_add_line_by_type(self, mock_add_line_by_type):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        mock_line = mock.Mock()
        mock_line.isChecked.return_value = True
        self.gui.line_prompt_changed(mock_line)

        self.gui.lines.prompt.setChecked.assert_called_with(True)
        self.assertEqual(1, mock_add_line_by_type.call_count)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.add_line_by_type')
    def test_line_delayed_checked_checks_line_and_calls_add_line_by_type(self, mock_add_line_by_type):
        self.gui.load_widget.last_loaded_run = mock.Mock(return_value=2695)
        mock_line = mock.Mock()
        mock_line.isChecked.return_value = True
        self.gui.line_delayed_changed(mock_line)

        self.gui.lines.delayed.setChecked.assert_called_with(True)
        self.assertEqual(1, mock_add_line_by_type.call_count)


if __name__ == '__main__':
    unittest.main()
