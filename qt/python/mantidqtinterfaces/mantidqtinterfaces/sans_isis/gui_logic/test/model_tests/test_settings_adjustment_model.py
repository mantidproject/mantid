# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from sans_core.common.enums import SANSInstrument, FitType
from mantidqtinterfaces.sans_isis.gui_logic.models.settings_adjustment_model import SettingsAdjustmentModel
from sans_core.user_file.settings_tags import DetectorId
from sans_core.user_file.txt_parsers.CommandInterfaceAdapter import CommandInterfaceAdapter


class SettingsTransmissionModelTest(unittest.TestCase):
    @staticmethod
    def create_model(user_file, file_information=None):
        adapter = CommandInterfaceAdapter(file_information=file_information, processed_state=user_file)
        model_under_test = SettingsAdjustmentModel(adapter.get_all_states(file_information=file_information))
        return model_under_test

    @staticmethod
    def create_mock_inst_file_information(inst):
        mocked = mock.Mock()
        mocked.get_instrument.return_value = inst
        mocked.get_number_of_periods.return_value = 0
        mocked.get_idf_file_path.return_value = None
        mocked.get_ipf_file_path.return_value = None
        mocked.get_run_number.return_value = 0
        return mocked

    def test_monitor_5_reported_for_zoom(self):
        user_file = {DetectorId.INSTRUMENT: [SANSInstrument.ZOOM]}

        model_under_test = self.create_model(user_file, self.create_mock_inst_file_information(SANSInstrument.ZOOM))

        self.assertTrue(model_under_test.does_instrument_support_monitor_5())

    def test_monitor_5_disabled_for_no_inst(self):
        user_file = {DetectorId.INSTRUMENT: [SANSInstrument.NO_INSTRUMENT]}
        model_under_test = self.create_model(user_file, self.create_mock_inst_file_information(SANSInstrument.NO_INSTRUMENT))

        self.assertFalse(model_under_test.does_instrument_support_monitor_5())

    def test_monitor_5_disabled_for_other_instruments(self):
        for inst in SANSInstrument:
            if inst is SANSInstrument.ZOOM:
                continue  # We expect monitor 5 here

            user_file = {DetectorId.INSTRUMENT: [inst]}
            model_under_test = self.create_model(user_file, self.create_mock_inst_file_information(inst))
            self.assertFalse(model_under_test.does_instrument_support_monitor_5())

    def test_that_can_set_only_interpolation(self):
        state_gui_model = self.create_model({}, self.create_mock_inst_file_information(SANSInstrument.SANS2D))
        state_gui_model.normalization_interpolate = True
        # This is normally found in the IPF which we have mocked out, so it will be blank
        self.assertEqual(state_gui_model.normalization_incident_monitor, "")
        self.assertTrue(state_gui_model.normalization_interpolate)

    def test_that_transmission_monitor_defaults_are_empty_and_false_for_interpolating_rebin(self):
        state_gui_model = self.create_model({}, self.create_mock_inst_file_information(SANSInstrument.SANS2D))
        self.assertEqual(state_gui_model.transmission_incident_monitor, 2)
        self.assertFalse(state_gui_model.transmission_interpolate)

    def test_that_can_set_transmission_monitor(self):
        state_gui_model = self.create_model({}, self.create_mock_inst_file_information(SANSInstrument.SANS2D))
        state_gui_model.transmission_incident_monitor = 2
        state_gui_model.transmission_interpolate = True
        self.assertEqual(state_gui_model.transmission_incident_monitor, 2)
        self.assertTrue(state_gui_model.transmission_interpolate)
        # # Reassign
        state_gui_model.transmission_incident_monitor = 3
        self.assertEqual(state_gui_model.transmission_incident_monitor, 3)

    def test_that_can_set_only_transmission_interpolation(self):
        state_gui_model = self.create_model({}, self.create_mock_inst_file_information(SANSInstrument.SANS2D))
        state_gui_model.transmission_interpolate = True
        self.assertEqual(state_gui_model.transmission_incident_monitor, 2)
        self.assertTrue(state_gui_model.transmission_interpolate)

    def test_that_the_default_transmission_roi_and_mask_files_and_radius_are_empty(self):
        state_gui_model = self.create_model({})
        self.assertEqual(state_gui_model.transmission_roi_files, "")
        self.assertEqual(state_gui_model.transmission_mask_files, "")
        self.assertEqual(state_gui_model.transmission_radius, "")

    def test_that_can_set_transmission_roi_mask_and_radius(self):
        state_gui_model = self.create_model({})
        state_gui_model.transmission_roi_files = "roi.xml"
        state_gui_model.transmission_mask_files = "mask.xml"
        state_gui_model.transmission_radius = 8.0
        self.assertEqual(state_gui_model.transmission_roi_files, "roi.xml")
        self.assertEqual(state_gui_model.transmission_mask_files, "mask.xml")
        self.assertEqual(state_gui_model.transmission_radius, 8)

    def test_that_default_transmission_monitor_is_3(self):
        state_gui_model = self.create_model({}, self.create_mock_inst_file_information(SANSInstrument.SANS2D))
        self.assertEqual(state_gui_model.transmission_monitor, 3)

    def test_that_transmission_monitor_can_be_set(self):
        state_gui_model = self.create_model({}, self.create_mock_inst_file_information(SANSInstrument.SANS2D))
        state_gui_model.transmission_monitor = 4
        self.assertEqual(state_gui_model.transmission_monitor, 4)

    def test_that_transmission_mn_shift_default_is_empty(self):
        state_gui_model = self.create_model({}, self.create_mock_inst_file_information(SANSInstrument.SANS2D))
        self.assertEqual(state_gui_model.transmission_mn_4_shift, 0.0)

    def test_that_transmission_mn_5_shift_default_is_empty(self):
        state_gui_model = self.create_model({}, self.create_mock_inst_file_information(SANSInstrument.SANS2D))
        self.assertEqual(state_gui_model.transmission_mn_5_shift, 0.0)

    def test_that_transmission_mn_shift_can_be_set(self):
        state_gui_model = self.create_model({})
        state_gui_model.transmission_mn_4_shift = 234
        self.assertEqual(state_gui_model.transmission_mn_4_shift, 234)

    def test_that_default_for_adjustment_files_are_empty(self):
        state_gui_model = self.create_model({})
        self.assertEqual(state_gui_model.wavelength_adjustment_det_1, "")
        self.assertEqual(state_gui_model.wavelength_adjustment_det_2, "")
        self.assertEqual(state_gui_model.pixel_adjustment_det_1, "")
        self.assertEqual(state_gui_model.pixel_adjustment_det_2, "")

    def test_that_adjustment_files_can_be_set(self):
        state_gui_model = self.create_model({})
        state_gui_model.wavelength_adjustment_det_1 = "wav1.txt"
        state_gui_model.wavelength_adjustment_det_2 = "wav2.txt"
        state_gui_model.pixel_adjustment_det_1 = "pix1.txt"
        state_gui_model.pixel_adjustment_det_2 = "pix2.txt"
        self.assertEqual(state_gui_model.wavelength_adjustment_det_1, "wav1.txt")
        self.assertEqual(state_gui_model.wavelength_adjustment_det_2, "wav2.txt")
        self.assertEqual(state_gui_model.pixel_adjustment_det_1, "pix1.txt")
        self.assertEqual(state_gui_model.pixel_adjustment_det_2, "pix2.txt")

    def test_transmission_fit_defaults(self):
        state_gui_model = self.create_model({})
        # Not this MUST be Logarithmic for legacy user files to continue working
        self.assertEqual(state_gui_model.transmission_sample_fit_type, FitType.LOGARITHMIC)
        self.assertEqual(state_gui_model.transmission_can_fit_type, FitType.LOGARITHMIC)
        self.assertEqual(state_gui_model.transmission_sample_polynomial_order, 0)
        self.assertEqual(state_gui_model.transmission_can_polynomial_order, 0)

    def test_that_can_set_transmission_fit_options(self):
        state_gui_model = self.create_model({}, self.create_mock_inst_file_information(SANSInstrument.SANS2D))
        state_gui_model.transmission_sample_fit_type = FitType.LOGARITHMIC
        state_gui_model.transmission_can_fit_type = FitType.LINEAR
        state_gui_model.transmission_sample_polynomial_order = 2
        state_gui_model.transmission_can_polynomial_order = 2
        self.assertEqual(state_gui_model.transmission_sample_fit_type, FitType.LOGARITHMIC)
        self.assertEqual(state_gui_model.transmission_can_fit_type, FitType.LINEAR)
        self.assertEqual(state_gui_model.transmission_sample_polynomial_order, 2)
        self.assertEqual(state_gui_model.transmission_can_polynomial_order, 2)

    def test_that_transmission_fit_wavelength_defaults_to_empty(self):
        state_gui_model = self.create_model({})
        self.assertEqual(state_gui_model.transmission_sample_wavelength_min, "")
        self.assertEqual(state_gui_model.transmission_sample_wavelength_max, "")
        self.assertEqual(state_gui_model.transmission_can_wavelength_min, "")
        self.assertEqual(state_gui_model.transmission_can_wavelength_max, "")

    def test_that_transmission_fit_wavelength_can_be_set(self):
        state_gui_model = self.create_model({})
        state_gui_model.transmission_sample_wavelength_min = 1.3
        state_gui_model.transmission_sample_wavelength_max = 10.3
        state_gui_model.transmission_can_wavelength_min = 1.3
        state_gui_model.transmission_can_wavelength_max = 10.3

        self.assertEqual(state_gui_model.transmission_sample_wavelength_min, 1.3)
        self.assertEqual(state_gui_model.transmission_sample_wavelength_max, 10.3)
        self.assertEqual(state_gui_model.transmission_can_wavelength_min, 1.3)
        self.assertEqual(state_gui_model.transmission_can_wavelength_max, 10.3)


if __name__ == "__main__":
    unittest.main()
