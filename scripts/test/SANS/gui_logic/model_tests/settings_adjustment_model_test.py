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
        user_file = {DetectorId.INSTRUMENT: [SANSInstrument.ZOOM]}
        model_under_test = self.create_model(user_file)

        self.assertTrue(model_under_test.does_instrument_support_monitor_5())

    def test_monitor_5_disabled_for_no_inst(self):
        user_file = {DetectorId.INSTRUMENT: [SANSInstrument.NO_INSTRUMENT]}
        model_under_test = self.create_model(user_file)

        self.assertFalse(model_under_test.does_instrument_support_monitor_5())

    def test_monitor_5_disabled_for_sans(self):
        user_file = {DetectorId.INSTRUMENT: [SANSInstrument.SANS2D]}
        model_under_test = self.create_model(user_file)

        self.assertFalse(model_under_test.does_instrument_support_monitor_5())

    def test_that_can_set_only_interpolation(self):
        run_tab_model = self.create_model()
        run_tab_model.normalization_interpolate = True
        self.assertEqual(run_tab_model.normalization_incident_monitor, None)
        self.assertTrue(run_tab_model.normalization_interpolate)

    def test_that_transmission_monitor_defaults_are_empty_and_false_for_interpolating_rebin(self):
        run_tab_model = self.create_model()
        self.assertEqual(run_tab_model.transmission_incident_monitor, "")
        self.assertFalse(run_tab_model.transmission_interpolate)

    def test_that_can_set_transmission_monitor(self):
        run_tab_model = self.create_model()
        run_tab_model.transmission_incident_monitor = 2
        run_tab_model.transmission_interpolate = True
        self.assertEqual(run_tab_model.transmission_incident_monitor, 2)
        self.assertTrue(run_tab_model.transmission_interpolate)
        # # Reassign
        run_tab_model.transmission_incident_monitor = 3
        self.assertEqual(run_tab_model.transmission_incident_monitor, 3)

    def test_that_can_set_only_transmission_interpolation(self):
        run_tab_model = self.create_model()
        run_tab_model.transmission_interpolate = True
        self.assertEqual(run_tab_model.transmission_incident_monitor, None)
        self.assertTrue(run_tab_model.transmission_interpolate)

    def test_that_can_set_normalization_and_transmission_monitor_and_rebin_type_settings(self):
        pass

    def test_that_the_default_transmission_roi_and_mask_files_and_radius_are_empty(self):
        run_tab_model = self.create_model()
        self.assertEqual(run_tab_model.transmission_roi_files, "")
        self.assertEqual(run_tab_model.transmission_mask_files, "")
        self.assertEqual(run_tab_model.transmission_radius, "")

    def test_that_can_set_transmission_roi_mask_and_radius(self):
        run_tab_model = self.create_model()
        run_tab_model.transmission_roi_files = "roi.xml"
        run_tab_model.transmission_mask_files = "mask.xml"
        run_tab_model.transmission_radius = 8.
        self.assertEqual(run_tab_model.transmission_roi_files, "roi.xml")
        self.assertEqual(run_tab_model.transmission_mask_files, "mask.xml")
        self.assertEqual(run_tab_model.transmission_radius, 8)

    def test_that_default_transmission_monitor_is_3(self):
        run_tab_model = self.create_model()
        self.assertEqual(run_tab_model.transmission_monitor, 3)

    def test_that_transmission_monitor_can_be_set(self):
        run_tab_model = self.create_model()
        run_tab_model.transmission_monitor = 4
        self.assertEqual(run_tab_model.transmission_monitor, 4)

    def test_that_transmission_mn_shift_default_is_empty(self):
        run_tab_model = self.create_model()
        self.assertEqual(run_tab_model.transmission_mn_4_shift, "")

    def test_that_transmission_mn_5_shift_default_is_empty(self):
        run_tab_model = self.create_model()
        self.assertEqual(run_tab_model.transmission_mn_5_shift, "")

    def test_that_transmission_mn_shift_can_be_set(self):
        run_tab_model = self.create_model()
        run_tab_model.transmission_mn_shift = 234
        self.assertEqual(run_tab_model.transmission_mn_shift, 234)

    def test_that_default_for_adjustment_files_are_empty(self):
        run_tab_model = self.create_model()
        self.assertEqual(run_tab_model.wavelength_adjustment_det_1, "")
        self.assertEqual(run_tab_model.wavelength_adjustment_det_2, "")
        self.assertEqual(run_tab_model.pixel_adjustment_det_1, "")
        self.assertEqual(run_tab_model.pixel_adjustment_det_2, "")

    def test_that_adjustment_files_can_be_set(self):
        run_tab_model = self.create_model()
        run_tab_model.wavelength_adjustment_det_1 = "wav1.txt"
        run_tab_model.wavelength_adjustment_det_2 = "wav2.txt"
        run_tab_model.pixel_adjustment_det_1 = "pix1.txt"
        run_tab_model.pixel_adjustment_det_2 = "pix2.txt"
        self.assertEqual(run_tab_model.wavelength_adjustment_det_1, "wav1.txt")
        self.assertEqual(run_tab_model.wavelength_adjustment_det_2, "wav2.txt")
        self.assertEqual(run_tab_model.pixel_adjustment_det_1, "pix1.txt")
        self.assertEqual(run_tab_model.pixel_adjustment_det_2, "pix2.txt")

    def test_transmission_fit_defaults(self):
        run_tab_model = self.create_model()
        self.assertEqual(run_tab_model.transmission_sample_fit_type, FitType.NO_FIT)
        self.assertEqual(run_tab_model.transmission_can_fit_type, FitType.NO_FIT)
        self.assertEqual(run_tab_model.transmission_sample_polynomial_order, 2)
        self.assertEqual(run_tab_model.transmission_can_polynomial_order, 2)

    def test_that_can_set_transmission_fit_options(self):
        run_tab_model = self.create_model()
        run_tab_model.transmission_sample_fit_type = FitType.LOGARITHMIC
        run_tab_model.transmission_can_fit_type = FitType.LINEAR
        run_tab_model.transmission_sample_polynomial_order = 2
        run_tab_model.transmission_can_polynomial_order = 2
        self.assertEqual(run_tab_model.transmission_sample_fit_type, FitType.LOGARITHMIC)
        self.assertEqual(run_tab_model.transmission_can_fit_type, FitType.LINEAR)
        self.assertEqual(run_tab_model.transmission_sample_polynomial_order, 2)
        self.assertEqual(run_tab_model.transmission_can_polynomial_order, 2)

    def test_that_transmission_fit_wavelength_defaults_to_empty(self):
        run_tab_model = self.create_model()
        self.assertEqual(run_tab_model.transmission_sample_wavelength_min, "")
        self.assertEqual(run_tab_model.transmission_sample_wavelength_max, "")
        self.assertEqual(run_tab_model.transmission_can_wavelength_min, "")
        self.assertEqual(run_tab_model.transmission_can_wavelength_max, "")

    def test_that_transmission_fit_wavelength_can_be_set(self):
        run_tab_model = self.create_model()
        run_tab_model.transmission_sample_wavelength_min = 1.3
        run_tab_model.transmission_sample_wavelength_max = 10.3
        run_tab_model.transmission_can_wavelength_min = 1.3
        run_tab_model.transmission_can_wavelength_max = 10.3

        self.assertEqual(run_tab_model.transmission_sample_wavelength_min, 1.3)
        self.assertEqual(run_tab_model.transmission_sample_wavelength_max, 10.3)
        self.assertEqual(run_tab_model.transmission_can_wavelength_min, 1.3)
        self.assertEqual(run_tab_model.transmission_can_wavelength_max, 10.3)

    def test_that_normalize_to_monitor_defaults_are_empty_for_monitor_and_false_for_interpolating_rebin(self):
        run_tab_model = self.create_model()
        self.assertEqual(run_tab_model.normalization_incident_monitor, "")
        self.assertFalse(run_tab_model.normalization_interpolate)

    def test_that_can_set_normalize_to_monitor(self):
        run_tab_model = self.create_model()
        run_tab_model.normalization_incident_monitor = 2
        run_tab_model.normalization_interpolate = True
        self.assertEqual(run_tab_model.normalization_incident_monitor, 2)
        self.assertTrue(run_tab_model.normalization_interpolate)
        # Reassign
        run_tab_model.normalization_incident_monitor = 3
        self.assertEqual(run_tab_model.normalization_incident_monitor, 3)


if __name__ == '__main__':
    unittest.main()
