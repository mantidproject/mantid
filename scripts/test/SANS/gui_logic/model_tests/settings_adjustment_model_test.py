import unittest

from sans.common.enums import SANSInstrument
from sans.gui_logic.models.settings_adjustment_model import SettingsAdjustmentModel
from sans.user_file.settings_tags import DetectorId


class SettingsTransmissionModelTest(unittest.TestCase):
    @staticmethod
    def create_model(user_file):
        model_under_test = SettingsAdjustmentModel(user_file)
        return model_under_test

    def test_monitor_5_reported_for_zoom(self):
        user_file = {DetectorId.instrument: [SANSInstrument.ZOOM]}
        model_under_test = self.create_model(user_file)

        self.assertTrue(model_under_test.does_instrument_support_monitor_5())

    def test_monitor_5_disabled_for_no_inst(self):
        user_file = {DetectorId.instrument: [SANSInstrument.NoInstrument]}
        model_under_test = self.create_model(user_file)

        self.assertFalse(model_under_test.does_instrument_support_monitor_5())

    def test_monitor_5_disabled_for_sans(self):
        user_file = {DetectorId.instrument: [SANSInstrument.SANS2D]}
        model_under_test = self.create_model(user_file)

        self.assertFalse(model_under_test.does_instrument_support_monitor_5())


if __name__ == '__main__':
    unittest.main()
