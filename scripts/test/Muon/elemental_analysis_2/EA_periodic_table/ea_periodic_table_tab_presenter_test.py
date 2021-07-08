# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
import matplotlib
from Muon.GUI.ElementalAnalysis2.periodic_table_tab.periodic_table_tab_presenter import PeriodicTableTabPresenter
from Muon.GUI.ElementalAnalysis2.periodic_table_tab.periodic_table_tab_view import PeriodicTableTabView


@start_qapplication
class PeriodicTableTabPresenterTest(unittest.TestCase):

    def raise_ValueError_once(self):
        if not self.has_raise_ValueError_been_called_once:
            self.has_raise_ValueError_been_called_once = True
            raise ValueError()

    def setUp(self):
        self.periodic_table = PeriodicTableTabPresenter(mock.Mock(), PeriodicTableTabView())
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

    @mock.patch('Muon.GUI.ElementalAnalysis2.periodic_table_tab.periodic_table_tab_presenter.PeriodicTableTabPresenter.'
                '_add_element_lines')
    def test_table_left_clicked_adds_lines_if_element_selected(self, mock_add_element_lines):
        self.periodic_table.ptable.is_selected = mock.Mock(return_value=True)
        self.periodic_table._add_element_lines(mock.Mock())
        self.assertEqual(mock_add_element_lines.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis2.periodic_table_tab.periodic_table_tab_presenter.PeriodicTableTabPresenter.'
                '_remove_element_lines')
    def test_table_left_clicked_removed_lines_if_element_not_selected(self,
                                                                      mock_remove_element_lines):
        self.periodic_table.ptable.is_selected = mock.Mock(return_value=False)
        self.periodic_table.table_left_clicked(mock.Mock())
        self.assertEqual(mock_remove_element_lines.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis2.periodic_table_tab.periodic_table_tab_presenter.PeriodicTableTabPresenter.'
                'get_color')
    def test_that_add_element_lines_will_call_get_color(self, mock_get_color):
        self.periodic_table._add_element_lines('Cu')
        self.assertEqual(mock_get_color.call_count, 1)

    @mock.patch('Muon.GUI.ElementalAnalysis2.periodic_table_tab.periodic_table_tab_presenter.PeriodicTableTabPresenter.'
                '_add_element_lines')
    @mock.patch('Muon.GUI.ElementalAnalysis2.periodic_table_tab.periodic_table_tab_presenter.PeriodicTableTabPresenter.'
                '_remove_element_lines')
    def test_update_peak_data_element_is_not_selected(self, mock_remove_element_lines,
                                                      mock_add_element_lines):
        self.periodic_table.ptable.is_selected = mock.Mock(return_value=False)
        self.periodic_table._update_peak_data('test_element')
        mock_remove_element_lines.assert_called_with('test_element')
        self.assertEqual(mock_add_element_lines.call_count, 0)

    @mock.patch('Muon.GUI.ElementalAnalysis.elemental_analysis.message_box.warning')
    @mock.patch('Muon.GUI.ElementalAnalysis2.periodic_table_tab.periodic_table_tab_presenter.PeriodicTablePresenter.'
                'set_peak_datafile')
    def test_that_set_peak_datafile_is_called_with_select_data_file(self, mock_set_peak_datafile, mock_warning):

        self.periodic_table.set_peak_data_file('filename')
        mock_set_peak_datafile.assert_called_with('filename')
        self.assertEqual(0, mock_warning.call_count)

    @mock.patch('Muon.GUI.ElementalAnalysis2.periodic_table_tab.periodic_table_tab_presenter.message_box.warning')
    @mock.patch('Muon.GUI.ElementalAnalysis2.periodic_table_tab.periodic_table_tab_presenter.PeriodicTableTabPresenter.'
                '_generate_element_widgets')
    def test_that_select_data_file_raises_warning_with_correct_text(self, mock_generate_element_widgets, mock_warning):
        mock_generate_element_widgets.side_effect = self.raise_ValueError_once
        self.periodic_table.set_peak_data_file('filename')
        warning_text = 'The file does not contain correctly formatted data, resetting to default data file.' \
                       'See "https://docs.mantidproject.org/nightly/interfaces/muon/' \
                       'Muon%20Elemental%20Analysis.html" for more information.'
        mock_warning.assert_called_with(warning_text)

    def test_select_data_file_calls_update_checked_data(self):
        tmp = self.periodic_table._update_checked_data
        self.periodic_table._update_checked_data = mock.Mock()
        self.periodic_table.set_peak_data_file()

        self.assertEqual(1, self.periodic_table._update_checked_data.call_count)
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

    @mock.patch('Muon.GUI.ElementalAnalysis2.periodic_table_tab.periodic_table_tab_presenter.PeriodicTableTabPresenter'
                '._update_peak_data')
    def test_checked_data_changes_all_states_in_list(self, mock_update_peak_data):
        selection = [mock.Mock() for i in range(10)]
        self.periodic_table.checked_data('Cu', selection, True)

        self.assertTrue(all(map(lambda m: m.setChecked.call_count == 1, selection)))
        mock_update_peak_data.assert_called_with('Cu')

    def test_that_ptable_contains_no_peak_not_part_of_an_element(self):
        self.assertTrue('Electrons' not in self.periodic_table.ptable.peak_data)
        self.assertTrue('Gammas' not in self.periodic_table.ptable.peak_data)

    @mock.patch('Muon.GUI.ElementalAnalysis2.periodic_table_tab.periodic_table_tab_presenter.PeriodicTableTabPresenter'
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

    @mock.patch('Muon.GUI.ElementalAnalysis2.periodic_table_tab.periodic_table_tab_presenter.PeriodicTableTabPresenter'
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


if __name__ == '__main__':
    unittest.main()
