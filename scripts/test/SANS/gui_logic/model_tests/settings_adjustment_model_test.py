import unittest

from sans.common.enums import SANSInstrument, FitType
from sans.gui_logic.models.settings_adjustment_model import SettingsAdjustmentModel
from sans.user_file.settings_tags import DetectorId


class SettingsTransmissionModelTest(unittest.TestCase):
    @staticmethod
    def create_model(user_file = None):
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

    def test_that_can_set_only_interpolation(self):
        state_gui_model = self.create_model()
        state_gui_model.normalization_interpolate = True
        self.assertEqual(state_gui_model.normalization_incident_monitor, None)
        self.assertTrue(state_gui_model.normalization_interpolate)

    def test_that_transmission_monitor_defaults_are_empty_and_false_for_interpolating_rebin(self):
        state_gui_model = self.create_model()
        self.assertEqual(state_gui_model.transmission_incident_monitor, "")
        self.assertFalse(state_gui_model.transmission_interpolate)

    def test_that_can_set_transmission_monitor(self):
        state_gui_model = self.create_model()
        state_gui_model.transmission_incident_monitor = 2
        state_gui_model.transmission_interpolate = True
        self.assertEqual(state_gui_model.transmission_incident_monitor, 2)
        self.assertTrue(state_gui_model.transmission_interpolate)
        # # Reassign
        state_gui_model.transmission_incident_monitor = 3
        self.assertEqual(state_gui_model.transmission_incident_monitor, 3)

    def test_that_can_set_only_transmission_interpolation(self):
        state_gui_model = self.create_model()
        state_gui_model.transmission_interpolate = True
        self.assertEqual(state_gui_model.transmission_incident_monitor, None)
        self.assertTrue(state_gui_model.transmission_interpolate)

    def test_that_can_set_normalization_and_transmission_monitor_and_rebin_type_settings(self):
        pass

    def test_that_the_default_transmission_roi_and_mask_files_and_radius_are_empty(self):
        state_gui_model = self.create_model()
        self.assertEqual(state_gui_model.transmission_roi_files, "")
        self.assertEqual(state_gui_model.transmission_mask_files, "")
        self.assertEqual(state_gui_model.transmission_radius, "")

    def test_that_can_set_transmission_roi_mask_and_radius(self):
        state_gui_model = self.create_model()
        state_gui_model.transmission_roi_files = "roi.xml"
        state_gui_model.transmission_mask_files = "mask.xml"
        state_gui_model.transmission_radius = 8.
        self.assertEqual(state_gui_model.transmission_roi_files, "roi.xml")
        self.assertEqual(state_gui_model.transmission_mask_files, "mask.xml")
        self.assertEqual(state_gui_model.transmission_radius, 8)

    def test_that_default_transmission_monitor_is_3(self):
        state_gui_model = self.create_model()
        self.assertEqual(state_gui_model.transmission_monitor, 3)

    def test_that_transmission_monitor_can_be_set(self):
        state_gui_model = self.create_model()
        state_gui_model.transmission_monitor = 4
        self.assertEqual(state_gui_model.transmission_monitor, 4)

    def test_that_transmission_mn_shift_default_is_empty(self):
        state_gui_model = self.create_model()
        self.assertEqual(state_gui_model.transmission_mn_4_shift, "")

    def test_that_transmission_mn_5_shift_default_is_empty(self):
        state_gui_model = self.create_model()
        self.assertEqual(state_gui_model.transmission_mn_5_shift, "")

    def test_that_transmission_mn_shift_can_be_set(self):
        state_gui_model = self.create_model()
        state_gui_model.transmission_mn_shift = 234
        self.assertEqual(state_gui_model.transmission_mn_shift, 234)

    def test_that_default_for_adjustment_files_are_empty(self):
        state_gui_model = self.create_model()
        self.assertEqual(state_gui_model.wavelength_adjustment_det_1, "")
        self.assertEqual(state_gui_model.wavelength_adjustment_det_2, "")
        self.assertEqual(state_gui_model.pixel_adjustment_det_1, "")
        self.assertEqual(state_gui_model.pixel_adjustment_det_2, "")

    def test_that_adjustment_files_can_be_set(self):
        state_gui_model = self.create_model()
        state_gui_model.wavelength_adjustment_det_1 = "wav1.txt"
        state_gui_model.wavelength_adjustment_det_2 = "wav2.txt"
        state_gui_model.pixel_adjustment_det_1 = "pix1.txt"
        state_gui_model.pixel_adjustment_det_2 = "pix2.txt"
        self.assertEqual(state_gui_model.wavelength_adjustment_det_1, "wav1.txt")
        self.assertEqual(state_gui_model.wavelength_adjustment_det_2, "wav2.txt")
        self.assertEqual(state_gui_model.pixel_adjustment_det_1, "pix1.txt")
        self.assertEqual(state_gui_model.pixel_adjustment_det_2, "pix2.txt")

    def test_transmission_fit_defaults(self):
        state_gui_model = self.create_model()
        self.assertEqual(state_gui_model.transmission_sample_fit_type, FitType.NoFit)
        self.assertEqual(state_gui_model.transmission_can_fit_type, FitType.NoFit)
        self.assertEqual(state_gui_model.transmission_sample_polynomial_order, 2)
        self.assertEqual(state_gui_model.transmission_can_polynomial_order, 2)

    def test_that_can_set_transmission_fit_options(self):
        state_gui_model = self.create_model()
        state_gui_model.transmission_sample_fit_type = FitType.Logarithmic
        state_gui_model.transmission_can_fit_type = FitType.Linear
        state_gui_model.transmission_sample_polynomial_order = 2
        state_gui_model.transmission_can_polynomial_order = 2
        self.assertEqual(state_gui_model.transmission_sample_fit_type, FitType.Logarithmic)
        self.assertEqual(state_gui_model.transmission_can_fit_type, FitType.Linear)
        self.assertEqual(state_gui_model.transmission_sample_polynomial_order, 2)
        self.assertEqual(state_gui_model.transmission_can_polynomial_order, 2)

    def test_that_transmission_fit_wavelength_defaults_to_empty(self):
        state_gui_model = self.create_model()
        self.assertEqual(state_gui_model.transmission_sample_wavelength_min, "")
        self.assertEqual(state_gui_model.transmission_sample_wavelength_max, "")
        self.assertEqual(state_gui_model.transmission_can_wavelength_min, "")
        self.assertEqual(state_gui_model.transmission_can_wavelength_max, "")

    def test_that_transmission_fit_wavelength_can_be_set(self):
        state_gui_model = self.create_model()
        state_gui_model.transmission_sample_wavelength_min = 1.3
        state_gui_model.transmission_sample_wavelength_max = 10.3
        state_gui_model.transmission_can_wavelength_min = 1.3
        state_gui_model.transmission_can_wavelength_max = 10.3

        self.assertEqual(state_gui_model.transmission_sample_wavelength_min, 1.3)
        self.assertEqual(state_gui_model.transmission_sample_wavelength_max, 10.3)
        self.assertEqual(state_gui_model.transmission_can_wavelength_min, 1.3)
        self.assertEqual(state_gui_model.transmission_can_wavelength_max, 10.3)


    def test_that_normalize_to_monitor_defaults_are_empty_for_monitor_and_false_for_interpolating_rebin(self):
        state_gui_model = self.create_model()
        self.assertEqual(state_gui_model.normalization_incident_monitor, "")
        self.assertFalse(state_gui_model.normalization_interpolate)
        
    def test_that_can_set_normalize_to_monitor(self):
        state_gui_model = self.create_model()
        state_gui_model.normalization_incident_monitor = 2
        state_gui_model.normalization_interpolate = True
        self.assertEqual(state_gui_model.normalization_incident_monitor, 2)
        self.assertTrue(state_gui_model.normalization_interpolate)
        # Reassign
        state_gui_model.normalization_incident_monitor = 3
        self.assertEqual(state_gui_model.normalization_incident_monitor, 3)

if __name__ == '__main__':
    unittest.main()
