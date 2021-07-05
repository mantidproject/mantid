# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest


from unittest import mock
from mantidqt.utils.qt.testing import start_qapplication
from qtpy.QtWidgets import QWidget

from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_model import InstrumentWidgetModel
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_presenter import InstrumentWidgetPresenter
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_view import InstrumentWidgetView
from mantidqt.utils.observer_pattern import Observer
from Muon.GUI.Common.test_helpers.context_setup import setup_context_for_tests


@start_qapplication
class HomeTabInstrumentPresenterTest(unittest.TestCase):
    def setUp(self):
        self.obj = QWidget()
        setup_context_for_tests(self)
        self.gui_variable_observer = Observer()

        self.gui_context.gui_variables_notifier.add_subscriber(self.gui_variable_observer)
        self.data_context.instrument = 'MUSR'
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
        self.data_context.instrumentNotifier.add_subscriber(observer)

        self.view.set_instrument('MUSR', block=True)
        self.view.set_instrument('CHRONUS')

        observer.update.assert_called_once_with(self.data_context.instrumentNotifier, 'CHRONUS')

    def test_that_changing_time_zero_updates_model(self):
        time_zero = 1.23456
        self.view.time_zero_checkbox.setChecked(False)
        self.view.set_time_zero(time_zero)
        self.view.time_zero_edit.editingFinished.emit()

        self.assertEqual(self.model.get_user_time_zero(), round(time_zero, 3))
        self.assertEqual(self.gui_variable_observer.update.call_count, 2)

    def test_that_changing_time_zero_does_nothing_if_checkbox_is_checked(self):
        time_zero = 1.23456
        self.view.time_zero_checkbox.setChecked(True)
        self.view.set_time_zero(time_zero)
        self.view.time_zero_edit.editingFinished.emit()

        self.assertEqual(self.model.get_user_time_zero(), 0.0)
        self.assertEqual(self.gui_variable_observer.update.call_count, 0)

    def test_that_changing_time_zero_checkbox_toggles_between_user_and_file_time_zero_displayed(self):
        user_time_zero = 1.234
        self.model.set_user_time_zero(user_time_zero)

        self.view.time_zero_checkbox.setChecked(True)
        self.assertEqual(self.view.get_time_zero(), 0.0)
        self.assertEqual(self.gui_variable_observer.update.call_count, 1)

        self.view.time_zero_checkbox.setChecked(False)
        self.assertEqual(self.view.get_time_zero(), user_time_zero)
        self.assertEqual(self.gui_variable_observer.update.call_count, 2)

    def test_that_changing_first_good_data_updates_model(self):
        time_zero = 1.23456
        self.view.first_good_data_checkbox.setChecked(False)
        self.view.set_first_good_data(time_zero)
        self.view.first_good_data_edit.editingFinished.emit()

        self.assertEqual(self.model.get_user_first_good_data(), round(time_zero, 3))
        self.assertEqual(self.gui_variable_observer.update.call_count, 2)

    def test_that_changing_first_good_data_does_nothing_if_checkbox_is_checked(self):
        time_zero = 1.23456
        self.view.first_good_data_checkbox.setChecked(True)
        self.view.set_first_good_data(time_zero)
        self.view.first_good_data_edit.editingFinished.emit()

        self.assertEqual(self.model.get_user_time_zero(), 0.0)
        self.assertEqual(self.gui_variable_observer.update.call_count, 0)

    def test_that_changing_first_good_data_checkbox_toggles_between_user_and_file_time_zero_displayed(self):
        user_time_zero = 1.234
        self.model.set_user_first_good_data(user_time_zero)

        self.view.first_good_data_checkbox.setChecked(True)
        self.assertEqual(self.view.get_first_good_data(), 0.0)
        self.assertEqual(self.gui_variable_observer.update.call_count, 1)

        self.view.first_good_data_checkbox.setChecked(False)
        self.assertEqual(self.view.get_first_good_data(), user_time_zero)
        self.assertEqual(self.gui_variable_observer.update.call_count, 2)

    def test_that_changing_steps_rebin_updates_fixed_binning_in_model(self):
        self.view.rebin_steps_edit.setText('50')
        self.view.rebin_steps_edit.editingFinished.emit()

        self.assertEqual(self.model._context.gui_context['RebinFixed'], '50')
        self.gui_variable_observer.update.assert_called_once_with(self.gui_context.gui_variables_notifier, {'RebinFixed': '50'})

    def test_that_changing_variable_rebin_updates_variable_binning_in_model(self):
        self.view.rebin_variable_edit.setText('1,5,21')
        self.view.rebin_variable_edit.editingFinished.emit()

        self.assertEqual(self.model._context.gui_context['RebinVariable'], '1,5,21')
        self.gui_variable_observer.update.assert_called_once_with(self.gui_context.gui_variables_notifier, {'RebinVariable': '1,5,21'})

    def test_that_steps_only_accepts_a_single_integer(self):
        self.view.rebin_steps_edit.insert('1,0.1,70')
        self.view.rebin_steps_edit.editingFinished.emit()

        self.assertEqual(self.view.rebin_steps_edit.text(), '')
        self.assertEqual(self.model._context.gui_context['RebinFixed'], '')
        self.gui_variable_observer.update.assert_called_once_with(self.gui_context.gui_variables_notifier, {'RebinFixed': ''})

    def test_that_variable_rebin_only_accepts_a_valid_rebin_string(self):
        self.view.rebin_variable_edit.insert('1-0.1-80')
        self.view.rebin_variable_edit.editingFinished.emit()

        self.assertEqual(self.view.rebin_variable_edit.text(), '')

    def test_that_rebin_type_starts_as_none(self):
        self.assertEqual(self.model._context.gui_context['RebinType'], 'None')

    def test_that_updating_rebin_combobox_updates_context(self):
        self.view.rebin_selector.setCurrentIndex(1)

        self.assertEqual(self.model._context.gui_context['RebinType'], 'Fixed')
        self.assertEqual(self.gui_variable_observer.update.call_count, 1)

        self.view.rebin_selector.setCurrentIndex(2)

        self.assertEqual(self.model._context.gui_context['RebinType'], 'Variable')
        self.assertEqual(self.gui_variable_observer.update.call_count, 2)

        self.view.rebin_selector.setCurrentIndex(0)

        self.assertEqual(self.model._context.gui_context['RebinType'], 'None')
        self.assertEqual(self.gui_variable_observer.update.call_count, 3)

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

    def test_last_good_data_from_file(self):
        self.view.last_good_data_state = mock.Mock(return_value=True)
        self.view.set_last_good_data = mock.MagicMock()
        self.presenter.update_view_from_model()

        self.assertEqual(0.0, self.model.get_file_last_good_data())
        self.view.set_last_good_data.assert_called_once_with(0.0)

    def test_last_good_data_from_gui(self):
        self.view.last_good_data_state = mock.Mock(return_value=False)
        self.view.set_last_good_data = mock.MagicMock()
        self.context.gui_context["LastGoodData"] = 2.0
        self.presenter.update_view_from_model()

        self.assertEqual(2.0, self.model.get_last_good_data())
        self.view.set_last_good_data.assert_called_once_with(2.0)


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)
