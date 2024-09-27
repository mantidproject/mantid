# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import unittest
from unittest import mock

from sans_core.common.configurations import Configurations
from sans_core.common.enums import DetectorType, SANSInstrument, ReductionMode, RangeStepType, RebinType, DataType, FitType
from sans_core.state.StateObjects.StateMoveDetectors import StateMoveZOOM
from sans_core.test_helper.user_file_test_helper import create_user_file, sample_user_file
from sans_core.user_file.settings_tags import DetectorId, TransId
from sans_core.user_file.txt_parsers.UserFileReaderAdapter import UserFileReaderAdapter
from sans_core.user_file.user_file_reader import UserFileReader


class ParsedDictConverterTest(unittest.TestCase):
    @staticmethod
    def create_mock_inst_file_information(inst):
        mocked = mock.Mock()
        mocked.get_instrument.return_value = inst
        mocked.get_number_of_periods.return_value = 0
        mocked.get_idf_file_path.return_value = None
        mocked.get_ipf_file_path.return_value = None
        return mocked

    def test_state_can_be_created_from_valid_user_file_with_data_information(self):
        user_file_path = create_user_file(sample_user_file)

        mocked_sans = self.create_mock_inst_file_information(SANSInstrument.SANS2D)

        parser = UserFileReaderAdapter(user_file_name=user_file_path, file_information=mocked_sans)
        state = parser.get_all_states(file_information=mocked_sans)

        # Assert
        self._assert_move(state)
        self._assert_mask(state)
        self._assert_reduction(state)
        self._assert_wavelength(state)
        self._assert_scale(state)
        self._assert_adjustment(state)
        self._assert_convert_to_q(state)

        # clean up
        if os.path.exists(user_file_path):
            os.remove(user_file_path)

    def _assert_move(self, state):
        move = state.move
        # Check the elements which were set on move
        self.assertEqual(move.sample_offset, 53.0 / 1000.0)

        # Detector specific
        lab = move.detectors[DetectorType.LAB.value]
        hab = move.detectors[DetectorType.HAB.value]
        self.assertEqual(lab.sample_centre_pos1, 155.45 / 1000.0)
        self.assertEqual(lab.sample_centre_pos2, -169.6 / 1000.0)
        self.assertEqual(lab.x_translation_correction, -16.0 / 1000.0)
        self.assertEqual(lab.z_translation_correction, 47.0 / 1000.0)
        self.assertEqual(hab.sample_centre_pos1, 155.45 / 1000.0)
        self.assertEqual(hab.sample_centre_pos2, -169.6 / 1000.0)
        self.assertEqual(hab.x_translation_correction, -44.0 / 1000.0)
        self.assertEqual(hab.y_translation_correction, -20.0 / 1000.0)
        self.assertEqual(hab.z_translation_correction, 47.0 / 1000.0)
        self.assertEqual(hab.rotation_correction, 0.0)

        # SANS2D-specific
        self.assertEqual(move.monitor_4_offset, -70.0 / 1000.0)

    def _assert_mask(self, state):
        mask = state.mask
        self.assertEqual(mask.radius_min, 12 / 1000.0)
        self.assertEqual(mask.radius_max, 15 / 1000.0)
        self.assertEqual(mask.clear, True)
        self.assertEqual(mask.clear_time, True)
        self.assertEqual(mask.detectors[DetectorType.LAB.value].single_horizontal_strip_mask, [0])
        self.assertEqual(mask.detectors[DetectorType.LAB.value].single_vertical_strip_mask, [0, 191])
        self.assertEqual(mask.detectors[DetectorType.HAB.value].single_horizontal_strip_mask, [0])
        self.assertEqual(mask.detectors[DetectorType.HAB.value].single_vertical_strip_mask, [0, 191])
        self.assertTrue(mask.detectors[DetectorType.LAB.value].range_horizontal_strip_start == [190, 167])
        self.assertTrue(mask.detectors[DetectorType.LAB.value].range_horizontal_strip_stop == [191, 172])
        self.assertTrue(mask.detectors[DetectorType.HAB.value].range_horizontal_strip_start == [190, 156])
        self.assertTrue(mask.detectors[DetectorType.HAB.value].range_horizontal_strip_stop == [191, 159])

    def _assert_reduction(self, state):
        reduction = state.reduction
        self.assertEqual(reduction.reduction_mode, ReductionMode.LAB)
        self.assertFalse(reduction.merge_mask)
        self.assertEqual(reduction.merge_min, None)
        self.assertEqual(reduction.merge_max, None)

    def _assert_scale(self, state):
        scale = state.scale
        self.assertEqual(scale.scale, 0.074)

    def _assert_wavelength(self, state):
        wavelength = state.wavelength
        self.assertEqual(wavelength.wavelength_interval.wavelength_full_range, (1.5, 12.5))
        self.assertEqual(wavelength.wavelength_interval.selected_ranges, [(1.5, 12.5)])
        self.assertEqual(wavelength.wavelength_interval.wavelength_step, 0.125)
        self.assertEqual(wavelength.wavelength_step_type, RangeStepType.LIN)

    def _assert_convert_to_q(self, state):
        convert_to_q = state.convert_to_q
        self.assertEqual(convert_to_q.wavelength_cutoff, 8.0)
        self.assertEqual(convert_to_q.radius_cutoff, 0.2)
        self.assertEqual(convert_to_q.q_min, 0.001)
        self.assertEqual(convert_to_q.q_max, 0.2)
        self.assertEqual(convert_to_q.q_1d_rebin_string, "0.001,0.001,0.0126,-0.08,0.2")
        self.assertTrue(convert_to_q.use_gravity)

        self.assertTrue(convert_to_q.use_q_resolution)
        self.assertEqual(convert_to_q.q_resolution_a1, 13.0 / 1000.0)
        self.assertEqual(convert_to_q.q_resolution_a2, 14.0 / 1000.0)
        self.assertEqual(convert_to_q.q_resolution_delta_r, 11.0 / 1000.0)
        self.assertEqual(convert_to_q.moderator_file, "moderator_rkh_file.txt")
        self.assertEqual(convert_to_q.q_resolution_collimation_length, 12.0)

    def _assert_adjustment(self, state):
        adjustment = state.adjustment

        # Normalize to monitor settings
        normalize_to_monitor = adjustment.normalize_to_monitor
        self.assertEqual(normalize_to_monitor.prompt_peak_correction_min, 1000)
        self.assertEqual(normalize_to_monitor.prompt_peak_correction_max, 2000)
        self.assertEqual(normalize_to_monitor.rebin_type, RebinType.INTERPOLATING_REBIN)
        self.assertEqual(normalize_to_monitor.background_TOF_general_start, 3500)
        self.assertEqual(normalize_to_monitor.background_TOF_general_stop, 4500)
        self.assertEqual(normalize_to_monitor.background_TOF_monitor_start["1"], 35000)
        self.assertEqual(normalize_to_monitor.background_TOF_monitor_stop["1"], 65000)
        self.assertEqual(normalize_to_monitor.background_TOF_monitor_start["2"], 85000)
        self.assertEqual(normalize_to_monitor.background_TOF_monitor_stop["2"], 98000)
        self.assertEqual(normalize_to_monitor.incident_monitor, 1)

        # Calculate Transmission
        calculate_transmission = adjustment.calculate_transmission
        self.assertEqual(calculate_transmission.prompt_peak_correction_min, 1000)
        self.assertEqual(calculate_transmission.prompt_peak_correction_max, 2000)
        self.assertEqual(calculate_transmission.incident_monitor, 1)
        self.assertEqual(calculate_transmission.transmission_radius_on_detector, 0.007)  # This is in mm
        self.assertEqual(calculate_transmission.transmission_roi_files, ["test.xml", "test2.xml"])
        self.assertEqual(calculate_transmission.transmission_mask_files, ["test3.xml", "test4.xml"])
        self.assertEqual(calculate_transmission.transmission_monitor, 4)
        self.assertEqual(calculate_transmission.rebin_type, RebinType.INTERPOLATING_REBIN)
        self.assertEqual(calculate_transmission.wavelength_interval.wavelength_full_range, (1.5, 12.5))
        self.assertEqual(calculate_transmission.wavelength_interval.wavelength_step, 0.125)
        self.assertEqual(calculate_transmission.wavelength_step_type, RangeStepType.LIN)
        self.assertFalse(calculate_transmission.use_full_wavelength_range)
        self.assertEqual(calculate_transmission.wavelength_full_range_low, Configurations.SANS2D.wavelength_full_range_low)
        self.assertEqual(calculate_transmission.wavelength_full_range_high, Configurations.SANS2D.wavelength_full_range_high)
        self.assertEqual(calculate_transmission.background_TOF_general_start, 3500)
        self.assertEqual(calculate_transmission.background_TOF_general_stop, 4500)
        self.assertEqual(calculate_transmission.background_TOF_monitor_start["1"], 35000)
        self.assertEqual(calculate_transmission.background_TOF_monitor_stop["1"], 65000)
        self.assertEqual(calculate_transmission.background_TOF_monitor_start["2"], 85000)
        self.assertEqual(calculate_transmission.background_TOF_monitor_stop["2"], 98000)
        self.assertEqual(calculate_transmission.background_TOF_roi_start, 123)
        self.assertEqual(calculate_transmission.background_TOF_roi_stop, 466)
        self.assertEqual(calculate_transmission.fit[DataType.SAMPLE.value].fit_type, FitType.LOGARITHMIC)
        self.assertEqual(calculate_transmission.fit[DataType.SAMPLE.value].wavelength_low, 1.5)
        self.assertEqual(calculate_transmission.fit[DataType.SAMPLE.value].wavelength_high, 12.5)
        self.assertEqual(calculate_transmission.fit[DataType.SAMPLE.value].polynomial_order, 0)
        self.assertEqual(calculate_transmission.fit[DataType.CAN.value].fit_type, FitType.LOGARITHMIC)
        self.assertEqual(calculate_transmission.fit[DataType.CAN.value].wavelength_low, 1.5)
        self.assertEqual(calculate_transmission.fit[DataType.CAN.value].wavelength_high, 12.5)
        self.assertEqual(calculate_transmission.fit[DataType.CAN.value].polynomial_order, 0)

        # Wavelength and Pixel Adjustment
        wavelength_and_pixel_adjustment = adjustment.wavelength_and_pixel_adjustment
        self.assertEqual(wavelength_and_pixel_adjustment.wavelength_interval.wavelength_full_range, (1.5, 12.5))
        self.assertEqual(wavelength_and_pixel_adjustment.wavelength_interval.wavelength_step, 0.125)
        self.assertEqual(wavelength_and_pixel_adjustment.wavelength_step_type, RangeStepType.LIN)
        self.assertTrue(
            wavelength_and_pixel_adjustment.adjustment_files[DetectorType.LAB.value].wavelength_adjustment_file
            == "DIRECTM1_15785_12m_31Oct12_v12.dat"
        )
        self.assertTrue(
            wavelength_and_pixel_adjustment.adjustment_files[DetectorType.HAB.value].wavelength_adjustment_file
            == "DIRECTM1_15785_12m_31Oct12_v12.dat"
        )

        # Assert wide angle correction
        self.assertTrue(state.adjustment.wide_angle_correction)
        self.assertEqual("TUBE_SANS2D_BOTH_31681_25Sept15.nxs", state.adjustment.calibration)

    def test_move_with_hab_centre_uses_hab_centre_value(self):
        user_file_centre = """
        set centre 160.2 -170.5
        set centre/hab 160.5 -170.1
        """
        user_file_path = create_user_file(user_file_centre)
        mocked_sans = self.create_mock_inst_file_information(SANSInstrument.SANS2D)
        parser = UserFileReaderAdapter(user_file_name=user_file_path, file_information=mocked_sans)
        state = parser.get_all_states(file_information=mocked_sans)
        move = state.move
        lab = move.detectors[DetectorType.LAB.value]
        hab = move.detectors[DetectorType.HAB.value]
        self.assertEqual(lab.sample_centre_pos1, 160.2 / 1000.0)
        self.assertEqual(lab.sample_centre_pos2, -170.5 / 1000.0)
        self.assertEqual(hab.sample_centre_pos1, 160.5 / 1000.0)
        self.assertEqual(hab.sample_centre_pos2, -170.1 / 1000.0)

    MM_TO_M = 1000

    def test_move_sets_shift_correctly_m4(self):
        mocked_values = {DetectorId.INSTRUMENT: [SANSInstrument.SANS2D], TransId.SPEC_4_SHIFT: [-10.0]}
        adapter = mock.create_autospec(UserFileReader)
        adapter.read_user_file.return_value = mocked_values
        parser = UserFileReaderAdapter(file_information=None, user_file_name=None, txt_user_file_reader=adapter)
        state_move = parser.get_state_move(file_information=None)

        self.assertAlmostEqual(-10.0 / self.MM_TO_M, state_move.monitor_4_offset)

    def test_move_ignores_m5_for_non_zoom(self):
        mocked_values = {DetectorId.INSTRUMENT: [SANSInstrument.SANS2D], TransId.SPEC_5_SHIFT: [-10.0]}
        adapter = mock.create_autospec(UserFileReader)
        adapter.read_user_file.return_value = mocked_values
        parser = UserFileReaderAdapter(file_information=None, user_file_name=None, txt_user_file_reader=adapter)
        state_move = parser.get_state_move(file_information=None)

        self.assertAlmostEqual(0.0, state_move.monitor_4_offset)

    def test_move_m5_works_on_zoom(self):
        mocked_values = {DetectorId.INSTRUMENT: [SANSInstrument.ZOOM], TransId.SPEC_5_SHIFT: [-5.0]}
        adapter = mock.create_autospec(UserFileReader)
        adapter.read_user_file.return_value = mocked_values
        parser = UserFileReaderAdapter(file_information=None, user_file_name=None, txt_user_file_reader=adapter)
        state_move = parser.get_state_move(file_information=None)

        self.assertIsInstance(state_move, StateMoveZOOM)
        self.assertAlmostEqual(0.0, state_move.monitor_4_offset)
        self.assertAlmostEqual(-5.0 / self.MM_TO_M, state_move.monitor_5_offset)


if __name__ == "__main__":
    unittest.main()
