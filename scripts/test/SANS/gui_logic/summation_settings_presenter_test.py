import unittest
import sys
from sans.gui_logic.presenter.summation_settings_presenter import SummationSettingsPresenter
from sans.gui_logic.models.summation_settings import SummationSettings
from sans.gui_logic.models.binning_type import BinningType
from ui.sans_isis.summation_settings_widget import SummationSettingsWidget
from fake_signal import FakeSignal

from assert_called import assert_called
if sys.version_info.major == 3:
    from unittest import mock
else:
    import mock


class SummationSettingsPresenterTest(unittest.TestCase):
    def setUp(self):
        self.view = self._make_mock_view()
        self.summation_settings = self._make_mock_settings()
        self.presenter = self._make_presenter(self.summation_settings, self.view)

    def _make_mock_view(self):
        mock_view = mock.create_autospec(SummationSettingsWidget, spec_set=True)
        mock_view.binningTypeChanged = FakeSignal()
        mock_view.preserveEventsChanged = FakeSignal()
        mock_view.additionalTimeShiftsChanged = FakeSignal()
        mock_view.binSettingsChanged = FakeSignal()
        return mock_view

    def _make_mock_settings(self):
        return mock.create_autospec(SummationSettings)

    def _make_presenter(self, summation_settings, view):
        return SummationSettingsPresenter(summation_settings, view, None)

    def test_sets_binning_type_when_changed(self):
        new_binning_type = BinningType.Custom
        self.view.binningTypeChanged.emit(new_binning_type)
        self.summation_settings.set_histogram_binning_type.assert_called_with(BinningType.Custom)

    def test_retrieves_additional_time_shifts_when_changed(self):
        self.view.additionalTimeShiftsChanged.emit()
        assert_called(self.view.additional_time_shifts)

    def test_retrieves_bin_settings_when_changed(self):
        self.view.binSettingsChanged.emit()
        assert_called(self.view.bin_settings)

    def test_updates_model_when_bin_settings_changed(self):
        new_bin_settings = 'bin settings'
        self.view.bin_settings.return_value = new_bin_settings
        self.view.binSettingsChanged.emit()
        self.assertEqual(new_bin_settings, self.summation_settings.bin_settings)

    def test_updates_model_when_additional_time_shifts_changed(self):
        new_additional_time_shifts = '213221.123123'
        self.view.additional_time_shifts.return_value = new_additional_time_shifts
        self.view.additionalTimeShiftsChanged.emit()
        self.assertEqual(new_additional_time_shifts, self.summation_settings.additional_time_shifts)

    def test_refreshes_view_when_binning_type_changed(self):
        new_binning_type = BinningType.Custom
        self.view.binningTypeChanged.emit(new_binning_type)
        self.assertEqual(2, self.view.draw_settings.call_count)
        self.view.draw_settings.assert_called_with(self.summation_settings)

    def test_refreshes_view_when_overlay_event_workspaces_changed(self):
        self.view.preserveEventsChanged.emit(True)
        self.assertEqual(2, self.view.draw_settings.call_count)
        self.view.draw_settings.assert_called_with(self.summation_settings)

    def test_enables_overlay_workspace_when_enabled(self):
        self.view.preserveEventsChanged.emit(True)
        assert_called(self.summation_settings.enable_overlay_event_workspaces)

    def test_disabled_overay_workspace_when_disabled(self):
        self.view.preserveEventsChanged.emit(False)
        assert_called(self.summation_settings.disable_overlay_event_workspaces)

if __name__ == '__main__': unittest.main()
