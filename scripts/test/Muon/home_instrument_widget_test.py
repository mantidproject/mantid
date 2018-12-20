# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
import sys

from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_view import InstrumentWidgetView
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_presenter import InstrumentWidgetPresenter
from Muon.GUI.Common.home_instrument_widget.home_instrument_widget_model import InstrumentWidgetModel
from Muon.GUI.Common.muon_data_context import MuonDataContext
from Muon.GUI.Common import mock_widget
import unittest
from PyQt4 import QtGui
from Muon.GUI.Common.observer_pattern import Observer


if sys.version_info.major < 2:
    from unittest import mock
else:
    import mock


class HomeTabInstrumentPresenterTest(unittest.TestCase):
    def setUp(self):
        self._qapp = mock_widget.mockQapp()
        self.obj = QtGui.QWidget()
        self.context = MuonDataContext()
        self.view = InstrumentWidgetView(self.obj)
        self.model = InstrumentWidgetModel(self.context)
        self.presenter = InstrumentWidgetPresenter(self.view, self.model)

        self.view.warning_popup = mock.MagicMock()
        self.view.instrument_changed_warning = mock.MagicMock(return_value=1)

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

    def test_that_instrument_not_changed_is_user_responds_no(self):
        self.view.set_instrument('CHRONUS')
        self.view.instrument_changed_warning = mock.MagicMock(return_value=0)
        self.view.set_instrument('MUSR')

        self.assertEqual(self.model._data.instrument, 'CHRONUS')

    def test_that_subscribers_notified_when_instrument_changed(self):
        observer = Observer()
        observer.update = mock.MagicMock()
        self.presenter.instrumentNotifier.add_subscriber(observer)

        self.view.set_instrument('CHRONUS')

        observer.update.assert_called_once_with(self.presenter.instrumentNotifier,'CHRONUS')

    def test_that_changeing_time_zero_updates_model(self):
        time_zero = 1.23456
        self.view.timezero_checkbox.setChecked(False)
        self.view.set_time_zero(time_zero)
        self.view.timezero_edit.editingFinished.emit()

        self.assertEqual(self.model.get_user_time_zero(), round(time_zero, 3))

    def test_that_changeing_time_zero_does_nothing_if_checkbox_is_checked(self):
        time_zero = 1.23456
        self.view.timezero_checkbox.setChecked(True)
        self.view.set_time_zero(time_zero)
        self.view.timezero_edit.editingFinished.emit()

        self.assertEqual(self.model.get_user_time_zero(), 0.0)

    def test_that_changeing_time_zero_checkbox_toggles_between_user_and_file_time_zero_displayed(self):
        user_time_zero = 1.234
        self.model.set_user_time_zero(user_time_zero)

        self.view.timezero_checkbox.setChecked(True)
        self.assertEqual(self.view.get_time_zero(), 0.0)

        self.view.timezero_checkbox.setChecked(False)
        self.assertEqual(self.view.get_time_zero(), user_time_zero)

    def test_that_changeing_first_good_data_updates_model(self):
        time_zero = 1.23456
        self.view.firstgooddata_checkbox.setChecked(False)
        self.view.set_first_good_data(time_zero)
        self.view.firstgooddata_edit.editingFinished.emit()

        self.assertEqual(self.model.get_user_first_good_data(), round(time_zero, 3))

    def test_that_changeing_first_good_data_does_nothing_if_checkbox_is_checked(self):
        time_zero = 1.23456
        self.view.firstgooddata_checkbox.setChecked(True)
        self.view.set_first_good_data(time_zero)
        self.view.firstgooddata_edit.editingFinished.emit()

        self.assertEqual(self.model.get_user_time_zero(), 0.0)

    def test_that_changeing_first_good_data_checkbox_toggles_between_user_and_file_time_zero_displayed(self):
        user_time_zero = 1.234
        self.model.set_user_first_good_data(user_time_zero)

        self.view.firstgooddata_checkbox.setChecked(True)
        self.assertEqual(self.view.get_first_good_data(), 0.0)

        self.view.firstgooddata_checkbox.setChecked(False)
        self.assertEqual(self.view.get_first_good_data(), user_time_zero)

    def test_that_changeing_steps_rebin_updates_fixed_binning_in_model(self):
        self.view.rebin_steps_edit.setText('50')
        self.view.rebin_steps_edit.editingFinished.emit()

        self.assertEqual(self.model._data.loaded_data['Rebin'], '50')

    def test_that_changeing_variable_rebin_updates_fixed_binning_in_model(self):
        self.view.rebin_variable_edit.setText('1,5,8,150')
        self.view.rebin_variable_edit.editingFinished.emit()

        self.assertEqual(self.model._data.loaded_data['Rebin'], '1,5,8,150')

    #TODO Need to add validation to rebin input strings once I've worked out what they should be

    def test_that_on_dead_time_unselected_deadtime_model_set_to_none(self):
        self.view.deadtime_selector.setCurrentIndex(1)
        self.view.deadtime_selector.setCurrentIndex(0)

        self.assertEqual(self.view.deadtime_label_3.text(), self.presenter.dead_time_from_data_text([0.0]))
        self.assertEqual(self.model._data.loaded_data["DeadTimeTable"], None)

    def test_that_on_deadtime_data_selected_updates_with_no_loaded_data(self):
        self.view.deadtime_selector.setCurrentIndex(1)

        self.assertEqual(self.view.deadtime_label_3.text(), "No loaded dead time")

    # def test_that_on_deadtime_data_selected_updates_with_loaded_data(self):
    #     self.presenter._model.get_dead_time_table_from_data = mock.MagicMock(return_value={'dead-time' : 'fake deadtime table'})
    #
    #     self.view.deadtime_selector.setCurrentIndex(1)
    #
    #     self.assertEqual(self.view.deadtime_label_3.text(), self.presenter.dead_time_from_data_text(['fake deadtime table']))


if __name__ == '__main__':
    unittest.main(buffer=False, verbosity=2)