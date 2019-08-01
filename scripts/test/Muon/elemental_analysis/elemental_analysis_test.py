# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import print_function, absolute_import

import unittest

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import start_qapplication
from testhelpers import assertRaisesNothing

from qtpy.QtGui import QCloseEvent

import matplotlib
from Muon.GUI.ElementalAnalysis.elemental_analysis import ElementalAnalysisGui
from Muon.GUI.ElementalAnalysis.elemental_analysis import gen_name
from MultiPlotting.multi_plotting_widget import MultiPlotWindow
from MultiPlotting.multi_plotting_widget import MultiPlotWidget
from MultiPlotting.label import Label


@start_qapplication
class ElementalAnalysisTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        super(ElementalAnalysisTest, self).setUpClass()
        self.gui = ElementalAnalysisGui()

    @classmethod
    def tearDownClass(self):
        self.gui = None

    def setUp(self):
        self.gui.plot_window = None
        self.gui.color_index = 0
        self.gui.element_lines = {}
        self.has_raise_ValueError_been_called_once = False

    def raise_ValueError_once(self):
        if not self.has_raise_ValueError_been_called_once:
            self.has_raise_ValueError_been_called_once = True
            raise ValueError()

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

    def test_that_closing_with_no_plot_will_not_throw(self):
        self.gui.plot_window = None

        assertRaisesNothing(self, self.gui.closeEvent, QCloseEvent())

    def test_that_closing_with_a_plot_will_close_the_window(self):
        self.gui.plot_window = mock.create_autospec(MultiPlotWindow)
        self.gui.closeEvent(QCloseEvent())

        self.assertEqual(self.gui.plot_window.closeEvent.call_count, 1)

    def test_that_gen_label_does_not_throw_with_non_float_x_values(self):
        self.gui._gen_label('name', 'not_a_float', 'Cu')
        self.gui._gen_label('name', u'not_a_float', 'Cu')
        self.gui._gen_label('name', None, 'Cu')
        self.gui._gen_label('name', ('not', 'a', 'float'), 'Cu')
        self.gui._gen_label('name', ['not', 'a', 'float'], 'Cu')

    def test_gen_label_output_is_Label_type(self):
        name = "string"
        x_value_in = 1.0
        self.gui.element_lines["H"] = []
        gen_label_output = self.gui._gen_label(name, x_value_in, element="H")
        self.assertEquals(Label, type(gen_label_output))

    def test_that_gen_label_with_element_none_returns_none(self):
        self.assertEqual(self.gui._gen_label('label', 0.0), None)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.Label')
    def test_that_gen_label_name_is_appended_to_list_if_not_present(self, mock_label):
        element = 'Cu'
        name = u'new_label'
        self.gui.element_lines = {'Cu': [u'old_label']}
        self.gui._gen_label(name, 1.0, element)

        self.assertIn(name, self.gui.element_lines[element])
        mock_label.assert_called_with(str(name), 1.0, False, 0.9, True, rotation=-90, protected=True)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.Label')
    def test_that_gen_label_name_is_not_duplicated_in_list_if_already_present(self, mock_label):
        element = 'Cu'
        name = u'old_label'
        self.gui.element_lines = {'Cu': [u'old_label']}
        self.gui._gen_label(name, 1.0, element)

        self.assertIn(name, self.gui.element_lines[element])
        self.assertEqual(self.gui.element_lines[element].count(name), 1)
        mock_label.assert_called_with(str(name), 1.0, False, 0.9, True, rotation=-90, protected=True)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._gen_label')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._plot_line_once')
    def test_that_plot_line_returns_if_plot_window_is_none(self, mock_plot_line_once, mock_gen_label):
        self.gui.plot_window = None
        mock_gen_label.return_value = 'name of the label'
        self.gui._plot_line('name', 1.0, 'C0', None)

        self.assertEqual(mock_plot_line_once.call_count, 0)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._gen_label')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._plot_line_once')
    def test_that_plot_line_calls_plot_line_once_if_window_not_none(self, mock_plot_line_once,
                                                                    mock_gen_label):
        self.gui.plot_window = mock.create_autospec(MultiPlotWindow)
        self.gui.plotting = MultiPlotWidget(mock.Mock())
        mock_get_subplots = mock.Mock()
        mock_get_subplots.return_value = ['plot1']
        self.gui.plotting.get_subplots = mock_get_subplots
        mock_gen_label.return_value = 'name of the label'
        self.gui._plot_line('name', 1.0, 'C0', None)

        self.assertEqual(mock_plot_line_once.call_count, 1)
        mock_plot_line_once.assert_called_with('plot1', 1.0, 'name of the label', 'C0')

    def test_plot_line_once_calls_correct_multiplot_function(self):
        self.gui.plotting = mock.create_autospec(MultiPlotWidget)
        self.gui._plot_line_once('GE1', 1.0, 'label', 'C0')

        self.assertEqual(self.gui.plotting.add_vline_and_annotate.call_count, 1)

    def test_that_rm_line_returns_if_plot_window_is_none(self):
        self.gui.plotting = MultiPlotWidget(mock.Mock())
        mock_get_subplots = mock.Mock()
        mock_get_subplots.return_value = ['plot1', 'plot2', 'plot3']
        self.gui.plotting.get_subplots = mock_get_subplots
        self.gui.plot_window = None

        self.gui._rm_line('line')

        self.assertEqual(mock_get_subplots.call_count, 0)

    def test_that_rm_line_calls_correct_function_if_window_not_none(self):
        self.gui.plotting = MultiPlotWidget(mock.Mock())
        mock_get_subplots = mock.Mock()
        mock_get_subplots.return_value = ['plot1', 'plot2', 'plot3']
        self.gui.plotting.get_subplots = mock_get_subplots
        mock_rm_vline_and_annotate = mock.Mock()
        self.gui.plotting.rm_vline_and_annotate = mock_rm_vline_and_annotate
        self.gui.plot_window = mock.create_autospec(MultiPlotWindow)
        self.gui._rm_line('line')

        self.assertEqual(mock_get_subplots.call_count, 1)
        self.assertEqual(mock_rm_vline_and_annotate.call_count, 3)
        mock_rm_vline_and_annotate.assert_called_with('plot3', 'line')

    def test_that_generate_element_widgets_creates_widget_once_for_each_element(self):
        # TODO remove -2 on element number once json file has been restructured
        elem = len(self.gui.ptable.peak_data)-2
        self.assertEqual(len(self.gui.element_widgets), elem)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._add_element_lines')
    def test_table_left_clicked_adds_lines_if_element_selected(self, mock_add_element_lines):
        self.gui.ptable.is_selected = mock.Mock()
        self.gui.ptable.is_selected.return_value = True
        test_item = mock.Mock()
        self.gui._add_element_lines(test_item)
        self.assertEqual(mock_add_element_lines.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._remove_element_lines')
    def test_table_left_clicked_removed_lines_if_element_not_selected(self, mock_remove_element_lines):
        self.gui.ptable.is_selected = mock.Mock()
        self.gui.ptable.is_selected.return_value = False
        test_item = mock.Mock()
        self.gui.table_left_clicked(test_item)
        self.assertEqual(mock_remove_element_lines.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.get_color')
    def test_that_add_element_lines_will_call_get_color(self, mock_get_color):
        self.gui._add_element_lines('Cu')
        self.assertEqual(mock_get_color.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._plot_line')
    def test_that_add_element_lines_will_call_plot_line(self, mock_plot_line):
        data = {'line1': 10.0, 'line2': 20.0, 'line3': 30.0}
        self.gui._add_element_lines('Cu', data)
        self.assertEqual(mock_plot_line.call_count, 3)
        call_list = [mock.call(gen_name('Cu', 'line3'), 30.0, 'C0', 'Cu'),
                     mock.call(gen_name('Cu', 'line2'), 20.0, 'C0', 'Cu'),
                     mock.call(gen_name('Cu', 'line1'), 10.0, 'C0', 'Cu')]
        mock_plot_line.assert_has_calls(call_list, any_order=True)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._rm_line')
    def test_remove_element_lines_does_nothing_if_element_not_in_element_lines(self, mock_rm_line):
        self.gui.element_lines['H'] = ['alpha', 'beta']
        self.gui._remove_element_lines('He')
        self.assertEqual(mock_rm_line.call_count, 0)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._rm_line')
    def test_remove_element_lines_removes_all_values_for_a_given_element(self, mock_rm_line):
        self.gui.element_lines['H'] = ['alpha', 'beta']
        self.gui._remove_element_lines('H')
        self.assertEqual(mock_rm_line.call_count, 2)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.MultiPlotWindow')
    def test_load_run_opens_new_plot_window_if_none_open(self, mock_MultiPlotWindow):
        self.gui.add_detector_to_plot = mock.Mock()
        self.gui.load_run('GE1', '2695')
        self.assertEqual(mock_MultiPlotWindow.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.MultiPlotWindow')
    def test_load_run_does_not_open_new_plot_window_if_one_is_open(self, mock_MultiPlotWindow):
        self.gui.add_detector_to_plot = mock.Mock()
        self.gui.plot_window = MultiPlotWindow(str('2695'))
        self.gui.load_run('GE1', 2695)
        self.assertEqual(mock_MultiPlotWindow.call_count, 0)

    def test_loading_finished_returns_nothing_if_no_run_loaded(self):
        self.gui.load_widget.last_loaded_run = mock.Mock()
        self.gui.load_widget.last_loaded_run.return_value = None
        self.gui.detectors = mock.Mock()
        self.gui.detectors.detectors = [mock.Mock(), mock.Mock(), mock.Mock(), mock.Mock()]
        for i in self.gui.detectors.detectors:
            i.isChecked.return_value = True
        self.gui.plot_window = mock.Mock()
        self.gui.plotting = mock.Mock()

        self.gui.loading_finished()
        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 0)

    def test_loading_finished_returns_correctly_if_no_plot_window_but_has_to_plot(self):
        self.gui.load_widget.last_loaded_run = mock.Mock()
        self.gui.load_widget.last_loaded_run.return_value = ['run1', 'run2', 'run3']
        self.gui.detectors = mock.Mock()
        self.gui.detectors.getNames.return_value = ['1', '2', '3']
        self.gui.detectors.detectors = [mock.Mock(), mock.Mock(), mock.Mock(), mock.Mock()]
        for i in self.gui.detectors.detectors:
            i.isChecked.return_value = True
        self.gui.plot_window = None
        self.gui.plotting = mock.Mock()
        self.gui.plotting.get_subplots.return_value = ['1', '2', '3']

        self.gui.loading_finished()
        self.assertEqual(self.gui.detectors.setStateQuietly.call_count, 3)
        for j in self.gui.detectors.detectors:
            self.assertEqual(j.setChecked.call_count, 1)

    def test_loading_finished_returns_correctly_if_no_to_plot_but_has_plot_window(self):
        self.gui.load_widget.last_loaded_run = mock.Mock()
        self.gui.load_widget.last_loaded_run.return_value = ['run1', 'run2', 'run3']
        self.gui.detectors = mock.Mock()
        self.gui.detectors.getNames.return_value = ['1', '2', '3']
        self.gui.detectors.detectors = [mock.Mock(), mock.Mock(), mock.Mock(), mock.Mock()]
        for i in self.gui.detectors.detectors:
            i.isChecked.return_value = False
        self.gui.plot_window = mock.Mock()

        self.gui.loading_finished()
        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 3)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.add_peak_data')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.mantid')
    def test_add_detectors_to_plot_plots_all_given_ws_and_all_selected_elements(self, mock_mantid, mock_add_peak_data):
        mock_mantid.mtd = {'name1': [mock.Mock(), mock.Mock()],
                           'name2': [mock.Mock(), mock.Mock()]}
        self.gui.plotting = mock.Mock()

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

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._gen_label')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._plot_line_once')
    def test_add_peak_data_plot_line_called_with_correct_terms(self, mock_plot_line_once, mock_gen_label):
        mock_subplot = mock.Mock()
        mock_gen_label.return_value = 'label'
        test_data = {'name1': 1.0}
        self.gui.add_peak_data('H', mock_subplot, data=test_data)
        mock_plot_line_once.assert_called_with(mock_subplot, 1.0, 'label', 'C0')

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._add_element_lines')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._remove_element_lines')
    def test_update_peak_data_element_is_selected(self,
                                                  mock_remove_element_lines,
                                                  mock_add_element_lines):
        self.gui.ptable.is_selected = mock.Mock()
        self.gui.ptable.is_selected.return_value = True
        self.gui._update_peak_data('test_element')
        mock_remove_element_lines.assert_called_with('test_element')
        mock_add_element_lines.assert_called_with('test_element')

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._add_element_lines')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._remove_element_lines')
    def test_update_peak_data_element_is_not_selected(self,
                                                      mock_remove_element_lines,
                                                      mock_add_element_lines):
        self.gui.ptable.is_selected = mock.Mock()
        self.gui.ptable.is_selected.return_value = False
        self.gui._update_peak_data('test_element')
        mock_remove_element_lines.assert_called_with('test_element')
        self.assertEqual(mock_add_element_lines.call_count, 0)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.load_run')
    def test_add_plot_does_nothing_is_no_loaded_run(self, mock_load_run):
        self.gui.load_widget.last_loaded_run = mock.Mock()
        mock_checkbox = mock.Mock()
        self.gui.load_widget.last_loaded_run.return_value = None
        self.gui.add_plot(mock_checkbox)
        self.assertEqual(mock_load_run.call_count, 0)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.add_peak_data')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.load_run')
    def test_add_plot_loads_run_and_electron_peaks_not_plotted(self, mock_load_run, add_peak_data):
        self.gui.load_widget.last_loaded_run = mock.Mock()
        self.gui.load_widget.last_loaded_run.return_value = 2695
        self.gui.peaks.electron.isChecked = mock.Mock()
        self.gui.peaks.electron.isChecked.return_value = False
        mock_checkbox = mock.Mock()
        mock_checkbox.name = 'GE1'
        self.gui.add_plot(mock_checkbox)
        mock_load_run.assert_called_with('GE1', 2695)
        self.assertEqual(add_peak_data.call_count, 0)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.add_peak_data')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.load_run')
    def test_add_plot_loads_run_and_electron_peaks_plotted(self, mock_load_run, add_peak_data):
        self.gui.load_widget.last_loaded_run = mock.Mock()
        self.gui.load_widget.last_loaded_run.return_value = 2695
        self.gui.peaks.electron.isChecked = mock.Mock()
        self.gui.peaks.electron.isChecked.return_value = True
        mock_checkbox = mock.Mock()
        mock_checkbox.name = 'GE1'
        self.gui.add_plot(mock_checkbox)
        mock_load_run.assert_called_with('GE1', 2695)
        add_peak_data.assert_called_with('e-', 'GE1', data=self.gui.electron_peaks)

    def test_del_plot_does_nothing_if_no_loaded_run(self):
        self.gui.load_widget.last_loaded_run = mock.Mock()
        self.gui.load_widget.last_loaded_run.return_value = None
        self.gui.plotting.remove_subplot = mock.Mock()
        mock_checkbox = mock.Mock()
        mock_checkbox.name = 'GE1'
        self.gui.del_plot(mock_checkbox)
        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 0)

    def test_del_plot_removes_subplot_only_if_other_subplots_exist(self):
        self.gui.load_widget.last_loaded_run = mock.Mock()
        self.gui.load_widget.last_loaded_run.return_value = 2695
        self.gui.plotting.remove_subplot = mock.Mock()
        self.gui.plotting.get_subplots = mock.Mock()
        self.gui.plotting.get_subplots.return_value = True
        self.gui.plot_window = 'plot_window'
        mock_checkbox = mock.Mock()
        mock_checkbox.name = 'GE1'
        self.gui.del_plot(mock_checkbox)
        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 1)
        self.assertEqual(self.gui.plot_window, 'plot_window')

    def test_del_plot_closes_plot_if_no_subplots_left(self):
        self.gui.load_widget.last_loaded_run = mock.Mock()
        self.gui.load_widget.last_loaded_run.return_value = 2695
        self.gui.plotting.remove_subplot = mock.Mock()
        self.gui.plotting.get_subplots = mock.Mock()
        self.gui.plotting.get_subplots.return_value = False
        self.gui.plot_window = mock.Mock()
        mock_checkbox = mock.Mock()
        mock_checkbox.name = 'GE1'
        self.gui.del_plot(mock_checkbox)
        self.assertEqual(self.gui.plotting.remove_subplot.call_count, 1)
        self.assertEqual(self.gui.plot_window, None)

    def test_subplotRemoved_changes_state_only_if_other_subplots_exist(self):
        self.gui.detectors.setStateQuietly = mock.Mock()
        self.gui.plotting.get_subplots = mock.Mock()
        self.gui.plotting.get_subplots.return_value = True
        self.gui.plot_window = 'plot_window'
        self.gui.subplotRemoved('name')
        self.assertEqual(self.gui.detectors.setStateQuietly.call_count, 1)
        self.assertEqual(self.gui.plot_window, 'plot_window')

    def test_subplotRemoved_closes_plot_if_no_other_subplots_exist(self):
        self.gui.detectors.setStateQuietly = mock.Mock()
        self.gui.plotting.get_subplots = mock.Mock()
        self.gui.plotting.get_subplots.return_value = False
        self.gui.plot_window = mock.Mock()
        self.gui.subplotRemoved('name')
        self.assertEqual(self.gui.detectors.setStateQuietly.call_count, 1)
        self.assertEqual(self.gui.plot_window, None)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.PeriodicTablePresenter.set_peak_datafile')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.QtWidgets.QFileDialog.getOpenFileName')
    def test_that_set_peak_datafile_is_called_with_select_data_file(self, mock_getOpenFileName, mock_set_peak_datafile):
        mock_getOpenFileName.return_value = 'filename'
        self.gui.select_data_file()
        mock_set_peak_datafile.assert_called_with('filename')

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.PeriodicTablePresenter.set_peak_datafile')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.QtWidgets.QFileDialog.getOpenFileName')
    def test_that_select_data_file_uses_the_first_element_of_a_tuple_when_given_as_a_filename(self,
                                                                                              mock_getOpenFileName,
                                                                                              mock_set_peak_datafile):
        mock_getOpenFileName.return_value = ('string1', 'string2')
        self.gui.select_data_file()
        mock_set_peak_datafile.assert_called_with('string1')

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.message_box.warning')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._generate_element_widgets')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.QtWidgets.QFileDialog.getOpenFileName')
    def test_that_select_data_file_raises_warning_with_correct_text(self,
                                                                    mock_getOpenFileName,
                                                                    mock_generate_element_widgets,
                                                                    mock_warning):
        mock_getOpenFileName.return_value = 'filename'
        mock_generate_element_widgets.side_effect = self.raise_ValueError_once
        self.gui.select_data_file()
        warning_text = 'The file does not contain correctly formatted data, resetting to default data file.'\
                       'See "https://docs.mantidproject.org/nightly/interfaces/'\
                       'Muon%20Elemental%20Analysis.html" for more information.'
        mock_warning.assert_called_with(warning_text)

    def test_gamms_checked_calls_checked_data_for_each_element(self):
        # TODO remove -2 on element number once json file has been restructured
        elem = len(self.gui.ptable.peak_data)-2
        self.gui.checked_data = mock.Mock()
        self.gui.gammas_checked()
        self.assertEqual(self.gui.checked_data.call_count, elem)

    def test_gamms_unchecked_calls_checked_data_for_each_element(self):
        # TODO remove -2 on element number once json file has been restructured
        elem = len(self.gui.ptable.peak_data)-2
        self.gui.checked_data = mock.Mock()
        self.gui.gammas_unchecked()
        self.assertEqual(self.gui.checked_data.call_count, elem)

    def test_major_checked_calls_checked_data_for_each_element(self):
        # TODO remove -2 on element number once json file has been restructured
        elem = len(self.gui.ptable.peak_data)-2
        self.gui.checked_data = mock.Mock()
        self.gui.major_peaks_checked()
        self.assertEqual(self.gui.checked_data.call_count, elem)

    def test_major_unchecked_calls_checked_data_for_each_element(self):
        # TODO remove -2 on element number once json file has been restructured
        elem = len(self.gui.ptable.peak_data)-2
        self.gui.checked_data = mock.Mock()
        self.gui.major_peaks_unchecked()
        self.assertEqual(self.gui.checked_data.call_count, elem)

    def test_minor_checked_calls_checked_data_for_each_element(self):
        # TODO remove -2 on element number once json file has been restructured
        elem = len(self.gui.ptable.peak_data)-2
        self.gui.checked_data = mock.Mock()
        self.gui.minor_peaks_checked()
        self.assertEqual(self.gui.checked_data.call_count, elem)

    def test_minor_unchecked_calls_checked_data_for_each_element(self):
        # TODO remove -2 on element number once json file has been restructured
        elem = len(self.gui.ptable.peak_data)-2
        self.gui.checked_data = mock.Mock()
        self.gui.minor_peaks_unchecked()
        self.assertEqual(self.gui.checked_data.call_count, elem)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._update_peak_data')
    def test_checked_data_changes_all_states_in_list(self, mock_update_peak_data):
        selection = [mock.Mock() for i in range(10)]
        self.gui.checked_data('Cu', selection, True)

        self.assertTrue(all(map(lambda m: m.setChecked.call_count == 1, selection)))
        mock_update_peak_data.assert_called_with('Cu')

    def test_get_electron_peaks_returns_a_dict_with_correct_length(self):
        peaks = len(self.gui.ptable.peak_data["Electrons"])
        electron_dict = self.gui._get_electron_peaks()
        self.assertEqual(len(electron_dict), peaks)
        for _, peak in electron_dict.items():
            float(peak)


if __name__ == '__main__':
    unittest.main()
