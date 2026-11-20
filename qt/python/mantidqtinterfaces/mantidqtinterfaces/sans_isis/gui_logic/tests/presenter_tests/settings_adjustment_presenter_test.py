# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from unittest.mock import Mock
from sans.common.enums import SANSInstrument
from mantidqtinterfaces.sans_isis.gui_logic.presenter.settings_adjustment_presenter import SettingsAdjustmentPresenter


class SettingsAdjustmentPresenterTest(unittest.TestCase):
    @staticmethod
    def test_monitor_5_correctly_disabled():
        mock_model = Mock()
        mock_model.does_instrument_support_monitor_5.return_value = False

        mock_view = Mock()
        settings_presenter = SettingsAdjustmentPresenter(view=mock_view, model=mock_model)
        settings_presenter.default_gui_setup()

        mock_view.set_monitor_5_enabled.assert_called_with(False)

    @staticmethod
    def test_monitor_5_correctly_enabled():
        mock_model = Mock()
        mock_model.does_instrument_support_monitor_5.return_value = True

        mock_view = Mock()
        settings_presenter = SettingsAdjustmentPresenter(view=mock_view, model=mock_model)
        settings_presenter.default_gui_setup()

        mock_view.set_monitor_5_enabled.assert_called_with(True)

    @staticmethod
    def test_monitor_5_considered_with_update_instrument():
        mock_model = Mock()
        mock_model.does_instrument_support_monitor_5.return_value = True

        mock_view = Mock()

        settings_presenter = SettingsAdjustmentPresenter(view=mock_view, model=mock_model)
        settings_presenter.update_instrument(SANSInstrument.ZOOM)

        mock_model.does_instrument_support_monitor_5.assert_called()
        mock_view.set_monitor_5_enabled.assert_called_with(True)

    def test_instrument_update_passed_to_model(self):
        mock_view = Mock()
        mock_model = Mock()

        presenter = SettingsAdjustmentPresenter(model=mock_model, view=mock_view)

        param = SANSInstrument.SANS2D
        presenter.update_instrument(param)
        self.assertEqual(param, mock_model.instrument)

    def test_set_on_view_with_decimal_places_keeps_intended_precision(self):
        mock_model = Mock()
        mock_view = Mock()

        mock_model.transmission_mn_4_shift = 0.001234
        mock_model.transmission_mn_5_shift = 0.0012346  # 6dp is the limit so it should be rounded

        presenter = SettingsAdjustmentPresenter(view=mock_view, model=mock_model)
        presenter.update_view_from_model()

        self.assertEqual(0.001234, mock_view.transmission_mn_4_shift)
        self.assertEqual(0.001235, mock_view.transmission_mn_5_shift)


if __name__ == "__main__":
    unittest.main()
