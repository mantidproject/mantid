from __future__ import absolute_import, print_function

import unittest
import matplotlib
from qtpy.QtGui import QCloseEvent

from mantid.py3compat import mock
from mantidqt.utils.qt.testing import GuiTest

from Muon.GUI.Common import message_box
from Muon.GUI.ElementalAnalysis.elemental_analysis import ElementalAnalysisGui
from Muon.GUI.ElementalAnalysis.elemental_analysis import gen_name

from MultiPlotting.multi_plotting_widget import MultiPlotWindow
from MultiPlotting.multi_plotting_widget import MultiPlotWidget
from MultiPlotting.label import Label


class ElementalAnalysisTest(GuiTest):
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
        self.has_raise_ValueError_once_been_called = False

    def raise_ValueError_once(self):
        if not self.has_raise_ValueError_once_been_called:
            self.has_raise_ValueError_once_been_called = True
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
        self.gui.closeEvent(QCloseEvent())

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

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.get_color')
    def test_that_add_element_lines_will_call_get_color(self, mock_get_color):
        self.gui._add_element_lines('Cu')
        self.assertEqual(mock_get_color.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui._plot_line')
    def test_that_add_element_lines_will_call_plot_line(self, mock_plot_line):
        data = {'line1': 10.0, 'line2': 20.0, 'line3': 30.0}
        self.gui._add_element_lines('Cu', data)
        self.assertEqual(mock_plot_line.call_count, 3)
        mock_plot_line.assert_called_with(gen_name('Cu', 'line1'), 10.0, 'C0', 'Cu')

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

    def test_checked_data_changes_all_states_in_list(self):
        self.gui._update_peak_data = mock.Mock()
        selection = [mock.Mock() for i in range(10)]
        self.gui.checked_data('Cu', selection, True)

        self.assertTrue(all(map(lambda m: m.setChecked.call_count == 1, selection)))
        self.gui._update_peak_data.assert_called_with('Cu', self.gui.element_widgets['Cu'].get_checked())

    def test_get_electron_peaks_returns_a_dict_with_correct_length(self):
        peaks = len(self.gui.ptable.peak_data["Electrons"])
        electron_dict = self.gui._get_electron_peaks()
        self.assertEqual(len(electron_dict), peaks)
        for _, peak in electron_dict.items():
            float(peak)


if __name__ == '__main__':
    unittest.main()