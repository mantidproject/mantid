# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from PyQt4 import QtGui
import unittest

from mantid.api import FileFinder
from mantid.py3compat import mock
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_view import InstrumentWidgetView
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_presenter import InstrumentWidgetPresenter
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_model import InstrumentWidgetModel
from Muon.GUI.Common.muon_data_context import MuonDataContext
from Muon.GUI.Common import mock_widget

from Muon.GUI.Common.observer_pattern import Observer


class HomeTabInstrumentPresenterTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.obj = QtGui.QWidget()
        self.context = MuonDataContext()
        self.gui_variable_observer = Observer()

        self.context.gui_variables_notifier.add_subscriber(self.gui_variable_observer)
        self.context.instrument = 'MUSR'
        self.view = InstrumentWidgetView(self.obj)
        self.view.set_instrument('MUSR', block=True)
        self.model = InstrumentWidgetModel(self.context)
        self.presenter = InstrumentWidgetPresenter(self.view, self.model)

        self.view.warning_popup = mock.MagicMock()
        self.view.instrument_changed_warning = mock.MagicMock(return_value=1)
        self.gui_variable_observer.update = mock.MagicMock()

    def tearDown(self):
        self.obj = None

    def test_that_on_instrument_change_model_instrument_updated(self):
        self.view.set_instrument('MUSR')

        self.assertEqual(self.model._data.instrument, 'MUSR')

    def test_that_setting_instrument_to_non_Muon_instrument_fails(self):
        self.view.set_instrument('MUSR')
        self.view.set_instrument('RANDOM')

        self.assertEqual(self.model._data.instrument, 'MUSR')

    def test_that_setting_instrument_with_signal_blocking_does_not_update_model(self):
        self.view.set_instrument('CHRONUS', block=False)
        self.view.set_instrument('MUSR', block=True)

        self.assertEqual(self.model._data.instrument, 'CHRONUS')

    def test_that_subscribers_notified_when_instrument_changed(self):
        observer = Observer()
        observer.update = mock.MagicMock()
        self.context.instrumentNotifier.add_subscriber(observer)

        self.view.set_instrument('MUSR', block=True)
        self.view.set_instrument('CHRONUS')

        observer.update.assert_called_once_with(self.context.instrumentNotifier, 'CHRONUS')

    def test_that_changeing_time_zero_updates_model(self):
        time_zero = 1.23456
        self.view.timezero_checkbox.setChecked(False)
        self.view.set_time_zero(time_zero)
        self.view.timezero_edit.editingFinished.emit()

        self.assertEqual(self.model.get_user_time_zero(), round(time_zero, 3))
        self.assertEqual(self.gui_variable_observer.update.call_count, 2)

    def test_that_changeing_time_zero_does_nothing_if_checkbox_is_checked(self):
        time_zero = 1.23456
        self.view.timezero_checkbox.setChecked(True)
        self.view.set_time_zero(time_zero)
        self.view.timezero_edit.editingFinished.emit()

        self.assertEqual(self.model.get_user_time_zero(), 0.0)
        self.assertEqual(self.gui_variable_observer.update.call_count, 0)

    def test_that_changeing_time_zero_checkbox_toggles_between_user_and_file_time_zero_displayed(self):
        user_time_zero = 1.234
        self.model.set_user_time_zero(user_time_zero)

        self.view.timezero_checkbox.setChecked(True)
        self.assertEqual(self.view.get_time_zero(), 0.0)
        self.assertEqual(self.gui_variable_observer.update.call_count, 1)

        self.view.timezero_checkbox.setChecked(False)
        self.assertEqual(self.view.get_time_zero(), user_time_zero)
        self.assertEqual(self.gui_variable_observer.update.call_count, 2)

    def test_that_changeing_first_good_data_updates_model(self):
        time_zero = 1.23456
        self.view.firstgooddata_checkbox.setChecked(False)
        self.view.set_first_good_data(time_zero)
        self.view.firstgooddata_edit.editingFinished.emit()

        self.assertEqual(self.model.get_user_first_good_data(), round(time_zero, 3))
        self.assertEqual(self.gui_variable_observer.update.call_count, 2)

    def test_that_changeing_first_good_data_does_nothing_if_checkbox_is_checked(self):
        time_zero = 1.23456
        self.view.firstgooddata_checkbox.setChecked(True)
        self.view.set_first_good_data(time_zero)
        self.view.firstgooddata_edit.editingFinished.emit()

        self.assertEqual(self.model.get_user_time_zero(), 0.0)
        self.assertEqual(self.gui_variable_observer.update.call_count, 0)

    def test_that_changeing_first_good_data_checkbox_toggles_between_user_and_file_time_zero_displayed(self):
        user_time_zero = 1.234
        self.model.set_user_first_good_data(user_time_zero)

        self.view.firstgooddata_checkbox.setChecked(True)
        self.assertEqual(self.view.get_first_good_data(), 0.0)
        self.assertEqual(self.gui_variable_observer.update.call_count, 1)

        self.view.firstgooddata_checkbox.setChecked(False)
        self.assertEqual(self.view.get_first_good_data(), user_time_zero)
        self.assertEqual(self.gui_variable_observer.update.call_count, 2)

    def test_that_changeing_steps_rebin_updates_fixed_binning_in_model(self):
        self.view.rebin_steps_edit.setText('50')
        self.view.rebin_steps_edit.editingFinished.emit()

        self.assertEqual(self.model._data.gui_variables['RebinFixed'], '50')
        self.gui_variable_observer.update.assert_called_once_with(self.context.gui_variables_notifier, None)

    def test_that_changeing_variable_rebin_updates_variable_binning_in_model(self):
        self.view.rebin_variable_edit.setText('1,5,21')
        self.view.rebin_variable_edit.editingFinished.emit()

        self.assertEqual(self.model._data.gui_variables['RebinVariable'], '1,5,21')
        self.gui_variable_observer.update.assert_called_once_with(self.context.gui_variables_notifier, None)

    def test_that_steps_only_accepts_a_single_integer(self):
        self.view.rebin_steps_edit.insert('1,0.1,70')
        self.view.rebin_steps_edit.editingFinished.emit()

        self.assertEqual(self.view.rebin_steps_edit.text(), '')
        self.assertEqual(self.model._data.gui_variables['RebinFixed'], '')
        self.gui_variable_observer.update.assert_called_once_with(self.context.gui_variables_notifier, None)

    def test_that_variable_rebin_only_accepts_a_valid_rebin_string(self):
        self.view.rebin_variable_edit.insert('1-0.1-80')
        self.view.rebin_variable_edit.editingFinished.emit()

        self.assertEqual(self.view.rebin_variable_edit.text(), '')

    def test_that_rebin_type_starts_as_none(self):
        self.assertEqual(self.model._data.gui_variables['RebinType'], 'None')

    def test_that_updating_rebin_combobox_updates_context(self):
        self.view.rebin_selector.setCurrentIndex(1)

        self.assertEqual(self.model._data.gui_variables['RebinType'], 'Fixed')
        self.assertEqual(self.gui_variable_observer.update.call_count, 1)

        self.view.rebin_selector.setCurrentIndex(2)

        self.assertEqual(self.model._data.gui_variables['RebinType'], 'Variable')
        self.assertEqual(self.gui_variable_observer.update.call_count, 2)

        self.view.rebin_selector.setCurrentIndex(0)

        self.assertEqual(self.model._data.gui_variables['RebinType'], 'None')
        self.assertEqual(self.gui_variable_observer.update.call_count, 3)

    def test_that_on_dead_time_unselected_deadtime_model_set_to_none(self):
        self.view.deadtime_selector.setCurrentIndex(1)
        self.view.deadtime_selector.setCurrentIndex(0)

        self.assertEqual(self.view.deadtime_label_3.text(), self.presenter.dead_time_from_data_text([0.0]))
        self.assertEqual(self.model._data.current_data["DeadTimeTable"], None)
        self.gui_variable_observer.update.assert_called_once_with(self.context.gui_variables_notifier, None)

    def test_that_on_deadtime_data_selected_updates_with_no_loaded_data(self):
        self.view.deadtime_selector.setCurrentIndex(1)

        self.assertEqual(self.view.deadtime_label_3.text(), "No loaded dead time")
        self.assertEqual(self.gui_variable_observer.update.call_count, 0)

    def test_that_on_deadtime_data_selected_updates_with_loaded_data(self):
        dead_time_data = mock.MagicMock()
        dead_time_data.toDict.return_value = {'dead-time': [0.001, 0.002, 0.003]}
        self.presenter._model.get_dead_time_table_from_data = mock.MagicMock(return_value=dead_time_data)

        self.view.deadtime_selector.setCurrentIndex(1)

        self.assertEqual(self.view.deadtime_label_3.text(), 'From 0.001 to 0.003 (ave. 0.002)')
        self.gui_variable_observer.update.assert_called_once_with(self.context.gui_variables_notifier, None)

    @mock.patch(
        'Muon.GUI.Common.home_instrument_widget.home_instrument_widget_presenter.load_utils.get_table_workspace_names_from_ADS')
    def test_that_selecting_from_table_workspace_deadtime_option_enables_table_workspace_combo_box(self,
                                                                                                   get_table_names_mock):
        get_table_names_mock.return_value = ['table_1', 'table_2', 'table_3']
        self.assertTrue(self.view.deadtime_file_selector.isHidden())

        self.view.deadtime_selector.setCurrentIndex(2)

        self.assertEqual(self.view.deadtime_label_3.text(), "From 0.000 to 0.000 (ave. 0.000)")
        self.assertFalse(self.view.deadtime_file_selector.isHidden())
        self.assertEqual(self.view.deadtime_file_selector.count(), 4)
        self.assertEqual(self.view.deadtime_file_selector.itemText(0), 'None')
        self.assertEqual(self.view.deadtime_file_selector.itemText(1), 'table_1')
        self.assertEqual(self.view.deadtime_file_selector.itemText(2), 'table_2')
        self.assertEqual(self.view.deadtime_file_selector.itemText(3), 'table_3')
        self.assertEqual(self.gui_variable_observer.update.call_count, 0)

    def test_that_returning_to_None_options_hides_table_workspace_selector(self):
        self.view.deadtime_selector.setCurrentIndex(2)
        self.view.deadtime_selector.setCurrentIndex(0)

        self.assertEqual(self.view.deadtime_label_3.text(), "From 0.000 to 0.000 (ave. 0.000)")
        self.assertTrue(self.view.deadtime_file_selector.isHidden())
        self.gui_variable_observer.update.assert_called_once_with(self.context.gui_variables_notifier, None)

    def test_browse_button_displayed_when_from_other_file_selected(self):
        self.assertTrue(self.view.deadtime_browse_button.isHidden())

        self.view.deadtime_selector.setCurrentIndex(3)

        self.assertFalse(self.view.deadtime_browse_button.isHidden())
        self.assertEqual(self.gui_variable_observer.update.call_count, 0)

    @mock.patch(
        'Muon.GUI.Common.home_instrument_widget.home_instrument_widget_presenter.load_utils.load_dead_time_from_filename')
    def test_browse_clicked_displays_warning_popup_if_file_does_not_contain_table(self, load_deadtime_mock):
        self.view.show_file_browser_and_return_selection = mock.MagicMock()
        load_deadtime_mock.return_value = ''
        self.view.deadtime_selector.setCurrentIndex(3)

        self.view.deadtime_browse_button.clicked.emit(True)

        self.view.show_file_browser_and_return_selection.assert_called_once_with('Files (*.nxs)', [''],
                                                                                 multiple_files=False)
        self.view.warning_popup.assert_called_once_with("File does not appear to contain dead time data.")
        self.assertEqual(self.gui_variable_observer.update.call_count, 0)


    @mock.patch(
        'Muon.GUI.Common.home_instrument_widget.home_instrument_widget_presenter.load_utils.load_dead_time_from_filename')
    def test_browse_clicked_does_nothing_if_no_file_selected(self, load_deadtime_mock):
        self.view.show_file_browser_and_return_selection = mock.MagicMock(return_value=[''])
        self.view.deadtime_selector.setCurrentIndex(3)

        self.view.deadtime_browse_button.clicked.emit(True)

        load_deadtime_mock.assert_not_called()
        self.view.warning_popup.assert_not_called()
        self.gui_variable_observer.update.assert_not_called()

    @mock.patch(
        'Muon.GUI.Common.home_instrument_widget.home_instrument_widget_presenter.load_utils.load_dead_time_from_filename')
    def test_browse_clicked_fails_if_table_not_loaded_into_ADS(self, load_deadtime_mock):
        self.view.show_file_browser_and_return_selection = mock.MagicMock(return_value=['filename'])
        load_deadtime_mock.return_value = 'dead_time_table_name'
        self.view.deadtime_selector.setCurrentIndex(3)

        self.view.deadtime_browse_button.clicked.emit(True)

        self.assertEqual(self.view.deadtime_selector.currentIndex(), 2)
        self.view.warning_popup.assert_called_once_with("Dead time table cannot be loaded")
        self.gui_variable_observer.update.assert_not_called()

    def test_browse_clicked_suceeds_if_table_in_ADS(self):
        filename = FileFinder.findRuns('MUSR00015196.nxs')[0]
        self.view.show_file_browser_and_return_selection = mock.MagicMock(return_value=[filename])
        self.model.check_dead_time_file_selection = mock.MagicMock(return_value=True)

        self.view.deadtime_browse_button.clicked.emit(True)

        self.assertEqual(self.view.deadtime_selector.currentIndex(), 2)
        self.view.warning_popup.assert_not_called()
        self.assertEqual(self.view.deadtime_file_selector.currentText(), 'MUSR00015196_deadTimes')
        self.gui_variable_observer.update.assert_called_once_with(self.context.gui_variables_notifier, None)

    def test_validate_variable_rebin_string_allows_single_number(self):
        result, message = self.model.validate_variable_rebin_string('0.034')

        self.assertTrue(result)

    def test_validate_variable_rebin_string_does_not_allow_empty(self):
        result, message = self.model.validate_variable_rebin_string('')

        self.assertFalse(result)

    def test_validate_variable_rebin_string_does_not_allow_non_numbers(self):
        result, message = self.model.validate_variable_rebin_string('abc')

        self.assertFalse(result)

    def test_validate_variable_rebin_string_requires_second_number_of_two_to_be_greater(self):
        result, message = self.model.validate_variable_rebin_string('4,2')

        self.assertFalse(result)

        result, message = self.model.validate_variable_rebin_string('2,4')

        self.assertTrue(result)

    def test_validate_variable_rebin_string_of_length_greater_than_three_must_have_bins_line_up(self):
        result, messag = self.model.validate_variable_rebin_string('1,5,21')

        self.assertTrue(result)

        result, message = self.model.validate_variable_rebin_string('1,5,20')

        self.assertFalse(result)

        result, message = self.model.validate_variable_rebin_string('1,5,21,4')

        self.assertFalse(result)

        result, message = self.model.validate_variable_rebin_string('1,-5,19')

        self.assertTrue(result)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
