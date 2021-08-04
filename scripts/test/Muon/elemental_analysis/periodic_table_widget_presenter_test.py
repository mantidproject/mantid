import unittest
import unittest.mock as mock
import matplotlib
from mantidqt.utils.qt.testing import start_qapplication
from Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter import gen_name
from MultiPlotting.multi_plotting_widget import MultiPlotWindow
from MultiPlotting.multi_plotting_widget import MultiPlotWidget
from Muon.GUI.ElementalAnalysis.elemental_analysis import ElementalAnalysisGui
from MultiPlotting.label import Label
from testhelpers import assertRaisesNothing


@start_qapplication
class PeriodicTableWidgetPresenterTest(unittest.TestCase):

    def raise_ValueError_once(self):
        if not self.has_raise_ValueError_been_called_once:
            self.has_raise_ValueError_been_called_once = True
            raise ValueError()

    @classmethod
    def setUpClass(cls):
        cls.view = ElementalAnalysisGui()
        cls.periodic_table = cls.view.ptable

    def setUp(self):
        self.periodic_table.plot_window = None
        self.periodic_table.used_colors = {}
        self.periodic_table.element_lines = {}
        self.has_raise_ValueError_been_called_once = False

    def test_that_get_color_returns_C0_as_first_color(self):
        self.assertEqual('C0', self.periodic_table.get_color('bla'))

    def test_that_default_colour_cycle_is_used(self):
        cycle = len(matplotlib.rcParams['axes.prop_cycle'])
        self.assertEqual(cycle, self.periodic_table.num_colors)

        expected = ['C%d' % i for i in range(cycle)]
        returned = [self.periodic_table.get_color('el_{}'.format(i)) for i in range(cycle)]
        self.assertEqual(expected, returned)

    def test_that_get_color_returns_the_same_color_for_the_same_element(self):
        self.assertEqual(self.periodic_table.used_colors, {})

        colors = [self.periodic_table.get_color('Cu') for _ in range(10)]
        colors += [self.periodic_table.get_color('Fe') for _ in range(10)]
        colors = sorted(list(set(colors)))

        self.assertEqual(colors, ['C0', 'C1'])

    def test_that_get_color_returns_a_color_if_it_has_been_freed(self):
        elements = ['Cu', 'Fe', 'Ni']
        colors = [self.periodic_table.get_color(el) for el in elements]

        self.assertEqual(colors, ['C0', 'C1', 'C2'])

        del self.periodic_table.used_colors['Fe']

        new_col = self.periodic_table.get_color('O')
        self.assertEqual(new_col, 'C1')

    def test_that_get_color_wraps_around_when_all_colors_in_cycle_have_been_used(self):
        [self.periodic_table.get_color(i) for i in range(self.periodic_table.num_colors)]

        self.assertEqual(self.periodic_table.get_color('Cu'), 'C0')
        self.assertEqual(self.periodic_table.get_color('Fe'), 'C1')

    def test_that_generate_element_widgets_creates_widget_once_for_each_element(self):
        elem = len(self.periodic_table.ptable.peak_data)
        self.assertEqual(len(self.periodic_table.element_widgets), elem)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter.'
                '_add_element_lines')
    def test_table_left_clicked_adds_lines_if_element_selected(self, mock_add_element_lines):
        self.periodic_table.ptable.is_selected = mock.Mock(return_value=True)
        self.periodic_table._add_element_lines(mock.Mock())
        self.assertEqual(mock_add_element_lines.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter.'
                '_remove_element_lines')
    def test_table_left_clicked_removed_lines_if_element_not_selected(self,
                                                                      mock_remove_element_lines):
        self.periodic_table.ptable.is_selected = mock.Mock(return_value=False)
        self.periodic_table.table_left_clicked(mock.Mock())
        self.assertEqual(mock_remove_element_lines.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter.'
                'get_color')
    def test_that_add_element_lines_will_call_get_color(self, mock_get_color):
        self.periodic_table._add_element_lines('Cu')
        self.assertEqual(mock_get_color.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter.'
                '_plot_line')
    def test_that_add_element_lines_will_call_plot_line(self, mock_plot_line):
        data = {'line1': 10.0, 'line2': 20.0, 'line3': 30.0}
        self.periodic_table._add_element_lines('Cu', data)
        self.assertEqual(mock_plot_line.call_count, 3)
        call_list = [
            mock.call(gen_name('Cu', 'line3'), 30.0, 'C0', 'Cu'),
            mock.call(gen_name('Cu', 'line2'), 20.0, 'C0', 'Cu'),
            mock.call(gen_name('Cu', 'line1'), 10.0, 'C0', 'Cu')
        ]
        mock_plot_line.assert_has_calls(call_list, any_order=True)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter'
                '._rm_line')
    def test_remove_element_lines_does_nothing_if_element_not_in_element_lines(self, mock_rm_line):
        self.periodic_table.element_lines['H'] = ['alpha', 'beta']
        self.periodic_table._remove_element_lines('He')
        self.assertEqual(mock_rm_line.call_count, 0)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter'
                '._rm_line')
    def test_remove_element_lines_removes_all_values_for_a_given_element(self, mock_rm_line):
        self.periodic_table.element_lines['H'] = ['alpha', 'beta']
        self.periodic_table._remove_element_lines('H')
        self.assertEqual(mock_rm_line.call_count, 2)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter.'
                '_gen_label')
    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter'
                '._plot_line_once')
    def test_add_peak_data_plot_line_called_with_correct_terms(self, mock_plot_line_once,
                                                               mock_gen_label):
        mock_subplot = mock.Mock()
        mock_gen_label.return_value = 'label'
        test_data = {'name1': 1.0}
        self.periodic_table.add_peak_data('H', mock_subplot, data=test_data)
        mock_plot_line_once.assert_called_with(mock_subplot, 1.0, 'label', 'C0')

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter.'
                '_add_element_lines')
    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter'
                '._remove_element_lines')
    def test_update_peak_data_element_is_selected(self, mock_remove_element_lines,
                                                  mock_add_element_lines):
        self.periodic_table.ptable.is_selected = mock.Mock(return_value=True)
        self.periodic_table._update_peak_data('test_element')
        mock_remove_element_lines.assert_called_with('test_element')
        mock_add_element_lines.assert_called_with('test_element')

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter.'
                '_add_element_lines')
    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter.'
                '_remove_element_lines')
    def test_update_peak_data_element_is_not_selected(self, mock_remove_element_lines,
                                                      mock_add_element_lines):
        self.periodic_table.ptable.is_selected = mock.Mock(return_value=False)
        self.periodic_table._update_peak_data('test_element')
        mock_remove_element_lines.assert_called_with('test_element')
        self.assertEqual(mock_add_element_lines.call_count, 0)

    @mock.patch('Muon.GUI.Common.message_box.warning')
    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.QFileDialog.getOpenFileName')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.warning_popup')
    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter.'
                '_generate_element_widgets')
    def test_that_select_data_file_raises_warning_with_correct_text(self, mock_generate_element_widgets, mock_warning,
                                                                    mock_getOpenFileName, mock_message_box):
        mock_generate_element_widgets.side_effect = self.raise_ValueError_once
        mock_getOpenFileName.return_value = 'filename'
        self.periodic_table.select_data_file()
        warning_text = 'The file does not contain correctly formatted data, resetting to default data file.' \
                       'See "https://docs.mantidproject.org/nightly/interfaces/muon/' \
                       'Muon%20Elemental%20Analysis.html" for more information.'
        mock_warning.assert_called_with(warning_text)
        mock_message_box.assert_called_once()

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.ElementalAnalysisGui.warning_popup')
    @mock.patch('Muon.GUI.ElementalAnalysis.PeriodicTable.periodic_table_presenter.PeriodicTablePresenter.'
                'set_peak_datafile')
    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.QtWidgets.QFileDialog.getOpenFileName')
    def test_that_set_peak_datafile_is_called_with_select_data_file(self,
                                                                    mock_get_open_file_name,
                                                                    mock_set_peak_datafile,
                                                                    mock_warning):
        mock_get_open_file_name.return_value = 'filename'
        self.periodic_table.select_data_file()
        mock_set_peak_datafile.assert_called_with('filename')
        self.assertEqual(0, mock_warning.call_count)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTablePresenter.'
                'set_peak_datafile')
    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.QFileDialog.getOpenFileName')
    def test_that_select_data_file_uses_the_first_element_of_a_tuple_when_given_as_a_filename(self,
                                                                                              mock_get_open_file_name,
                                                                                              mock_set_peak_datafile):
        mock_get_open_file_name.return_value = ('string1', 'string2')
        self.periodic_table.select_data_file()
        mock_set_peak_datafile.assert_called_with('string1')

    @mock.patch('Muon.GUI.Common.message_box.warning')
    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.QFileDialog.getOpenFileName')
    def test_select_data_file_calls_update_checked_data(self, mock_getOpenFileName, mock_warning):
        mock_getOpenFileName.return_value = 'filename'
        tmp = self.periodic_table._update_checked_data
        self.periodic_table._update_checked_data = mock.Mock()
        self.periodic_table.select_data_file()

        self.assertEqual(1, self.periodic_table._update_checked_data.call_count)
        mock_warning.assert_called_once()
        self.periodic_table._update_checked_data = tmp

    def test_update_checked_data_calls_the_right_functions(self):
        self.periodic_table.major_peaks_changed = mock.Mock()
        self.periodic_table.minor_peaks_changed = mock.Mock()
        self.periodic_table.gammas_changed = mock.Mock()
        self.periodic_table.electrons_changed = mock.Mock()

        self.periodic_table._update_checked_data()

        self.assertEqual(1, self.periodic_table.major_peaks_changed.call_count)
        self.assertEqual(1, self.periodic_table.minor_peaks_changed.call_count)
        self.assertEqual(1, self.periodic_table.gammas_changed.call_count)
        self.assertEqual(1, self.periodic_table.electrons_changed.call_count)
        self.periodic_table.major_peaks_changed.assert_called_with(self.periodic_table.peakpresenter.major)
        self.periodic_table.minor_peaks_changed.assert_called_with(self.periodic_table.peakpresenter.minor)
        self.periodic_table.gammas_changed.assert_called_with(self.periodic_table.peakpresenter.gamma)
        self.periodic_table.electrons_changed.assert_called_with(self.periodic_table.peakpresenter.electron)

    def test_major_changed_calls_checked_data_for_each_element(self):
        elem = len(self.periodic_table.ptable.peak_data)
        self.periodic_table.checked_data = mock.Mock()
        self.periodic_table.major_peaks_changed(self.periodic_table.peakpresenter.major)
        self.assertEqual(self.periodic_table.checked_data.call_count, elem)

    def test_minor_changed_calls_checked_data_for_each_element(self):
        elem = len(self.periodic_table.ptable.peak_data)
        self.periodic_table.checked_data = mock.Mock()
        self.periodic_table.minor_peaks_changed(self.periodic_table.peakpresenter.minor)
        self.assertEqual(self.periodic_table.checked_data.call_count, elem)

    def test_gammas_changed_calls_checked_data_for_each_element(self):
        elem = len(self.periodic_table.ptable.peak_data)
        self.periodic_table.checked_data = mock.Mock()
        self.periodic_table.gammas_changed(self.periodic_table.peakpresenter.gamma)
        self.assertEqual(self.periodic_table.checked_data.call_count, elem)

    def test_electrons_changed_calls_checked_data_for_each_element(self):
        elem = len(self.periodic_table.ptable.peak_data)
        self.periodic_table.checked_data = mock.Mock()
        self.periodic_table.electrons_changed(self.periodic_table.peakpresenter.electron)
        self.assertEqual(self.periodic_table.checked_data.call_count, elem)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter'
                '._update_peak_data')
    def test_checked_data_changes_all_states_in_list(self, mock_update_peak_data):
        selection = [mock.Mock() for i in range(10)]
        self.periodic_table.checked_data('Cu', selection, True)

        self.assertTrue(all(map(lambda m: m.setChecked.call_count == 1, selection)))
        mock_update_peak_data.assert_called_with('Cu')

    def test_that_ptable_contains_no_peak_not_part_of_an_element(self):
        self.assertTrue('Electrons' not in self.periodic_table.ptable.peak_data)
        self.assertTrue('Gammas' not in self.periodic_table.ptable.peak_data)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter'
                '._remove_element_lines')
    def test_deselect_elements(self, mock_remove_element_lines):
        self.periodic_table.ptable.deselect_element = mock.Mock()

        self.periodic_table.peakpresenter.enable_deselect_elements_btn = mock.Mock()
        self.periodic_table.peakpresenter.disable_deselect_elements_btn = mock.Mock()

        self.periodic_table.deselect_elements()

        self.assertEquals(self.periodic_table.peakpresenter.enable_deselect_elements_btn.call_count, 1)
        self.assertEquals(self.periodic_table.peakpresenter.disable_deselect_elements_btn.call_count, 1)

        self.assertEquals(self.periodic_table.ptable.deselect_element.call_count,
                          len(self.periodic_table.element_widgets))
        self.assertEquals(mock_remove_element_lines.call_count, len(self.periodic_table.element_widgets))

        calls = [mock.call(element) for element in self.periodic_table.element_widgets.keys()]
        self.periodic_table.ptable.deselect_element.assert_has_calls(calls)
        mock_remove_element_lines.assert_has_calls(calls)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter'
                '._remove_element_lines')
    def test_deselect_elements_fails(self, mock_remove_element_lines):
        self.periodic_table.ptable.deselect_element = mock.Mock()

        self.periodic_table.peakpresenter.enable_deselect_elements_btn = mock.Mock()
        self.periodic_table.peakpresenter.disable_deselect_elements_btn = mock.Mock()

        self.periodic_table.deselect_elements()
        # Test passes only if deselect_element is not called with "Hydrogen"
        with self.assertRaises(AssertionError):
            self.periodic_table.ptable.deselect_element.assert_any_call("Hydrogen")

        with self.assertRaises(AssertionError):
            mock_remove_element_lines.assert_any_call("Hydrogen")

    def test_that_rm_line_returns_if_plot_window_is_none(self):
        self.periodic_table.plotting = MultiPlotWidget(mock.Mock())
        self.periodic_table.plotting.get_subplots = mock.Mock(return_value=['plot1', 'plot2', 'plot3'])
        self.periodic_table.plot_window = None

        self.periodic_table._rm_line('line')

        self.assertEqual(self.periodic_table.plotting.get_subplots.call_count, 0)

    def test_that_rm_line_calls_correct_function_if_window_not_none(self):
        self.periodic_table.plotting = MultiPlotWidget(mock.Mock())
        self.periodic_table.plotting.get_subplots = mock.Mock(return_value=['plot1', 'plot2', 'plot3'])
        self.periodic_table.plotting.rm_vline_and_annotate = mock.Mock()
        self.periodic_table.plot_window = mock.create_autospec(MultiPlotWindow)
        self.periodic_table._rm_line('line')

        self.assertEqual(self.periodic_table.plotting.get_subplots.call_count, 1)
        self.assertEqual(self.periodic_table.plotting.rm_vline_and_annotate.call_count, 3)
        self.periodic_table.plotting.rm_vline_and_annotate.assert_called_with('plot3', 'line')

    def test_that_gen_label_does_not_throw_with_non_float_x_values(self):
        assertRaisesNothing(self, self.periodic_table._gen_label, 'name', 'not_a_float', 'Cu')
        assertRaisesNothing(self, self.periodic_table._gen_label, 'name', u'not_a_float', 'Cu')
        assertRaisesNothing(self, self.periodic_table._gen_label, 'name', None, 'Cu')
        assertRaisesNothing(self, self.periodic_table._gen_label, 'name', ('not', 'a', 'float'), 'Cu')
        assertRaisesNothing(self, self.periodic_table._gen_label, 'name', ['not', 'a', 'float'], 'Cu')

    def test_gen_label_output_is_Label_type(self):
        name = "string"
        x_value_in = 1.0
        self.periodic_table.element_lines["H"] = []
        gen_label_output = self.periodic_table._gen_label(name, x_value_in, element="H")
        self.assertEquals(Label, type(gen_label_output))

    def test_that_gen_label_with_element_none_returns_none(self):
        self.assertEqual(self.periodic_table._gen_label('label', 0.0), None)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.Label')
    def test_that_gen_label_name_is_appended_to_list_if_not_present(self, mock_label):
        element = 'Cu'
        name = u'new_label'
        self.periodic_table.element_lines = {'Cu': [u'old_label']}
        self.periodic_table._gen_label(name, 1.0, element)

        self.assertIn(name, self.periodic_table.element_lines[element])
        mock_label.assert_called_with(str(name),
                                      1.0,
                                      False,
                                      0.9,
                                      True,
                                      rotation=-90,
                                      protected=True)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.Label')
    def test_that_gen_label_name_is_not_duplicated_in_list_if_already_present(self, mock_label):
        element = 'Cu'
        name = u'old_label'
        self.periodic_table.element_lines = {'Cu': [u'old_label']}
        self.periodic_table._gen_label(name, 1.0, element)

        self.assertIn(name, self.periodic_table.element_lines[element])
        self.assertEqual(self.periodic_table.element_lines[element].count(name), 1)
        mock_label.assert_called_with(str(name),
                                      1.0,
                                      False,
                                      0.9,
                                      True,
                                      rotation=-90,
                                      protected=True)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter'
                '._gen_label')
    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter'
                '._plot_line_once')
    def test_that_plot_line_returns_if_plot_window_is_none(self, mock_plot_line_once,
                                                           mock_gen_label):
        self.periodic_table.plot_window = None
        mock_gen_label.return_value = 'name of the label'
        self.periodic_table._plot_line('name', 1.0, 'C0', None)

        self.assertEqual(mock_plot_line_once.call_count, 0)

    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter'
                '._gen_label')
    @mock.patch('Muon.GUI.ElementalAnalysis.periodic_table_widget_presenter.PeriodicTableWidgetPresenter'
                '._plot_line_once')
    def test_that_plot_line_calls_plot_line_once_if_window_not_none(self, mock_plot_line_once,
                                                                    mock_gen_label):
        self.periodic_table.plot_window = mock.create_autospec(MultiPlotWindow)
        self.periodic_table.plotting = MultiPlotWidget(mock.Mock())
        self.periodic_table.plotting.get_subplots = mock.Mock(return_value=['plot1'])
        mock_gen_label.return_value = 'name of the label'
        self.periodic_table._plot_line('name', 1.0, 'C0', None)

        self.assertEqual(mock_plot_line_once.call_count, 1)
        mock_plot_line_once.assert_called_with('plot1', 1.0, 'name of the label', 'C0')

    def test_plot_line_once_calls_correct_multiplot_function(self):
        self.periodic_table.plotting = mock.create_autospec(MultiPlotWidget)
        self.periodic_table._plot_line_once('GE1', 1.0, 'label', 'C0')

        self.assertEqual(self.periodic_table.plotting.add_vline_and_annotate.call_count, 1)


if __name__ == '__main__':
    unittest.main()
