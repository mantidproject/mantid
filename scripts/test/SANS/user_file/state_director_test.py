# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import os
import unittest
import mantid


from sans.user_file.state_director import StateDirectorISIS
from sans.common.enums import (SANSFacility, ISISReductionMode, RangeStepType, RebinType, DataType, FitType,
                               DetectorType, SampleShape, SANSInstrument)
from sans.common.configurations import Configurations
from sans.state.data import get_data_builder

from sans.test_helper.user_file_test_helper import create_user_file, sample_user_file
from sans.test_helper.file_information_mock import SANSFileInformationMock


# -----------------------------------------------------------------
# --- Tests -------------------------------------------------------
# -----------------------------------------------------------------
class UserFileStateDirectorISISTest(unittest.TestCase):
    def _assert_data(self, state):
        data = state.data
        self.assertTrue(data.calibration == "TUBE_SANS2D_BOTH_31681_25Sept15.nxs")
        self.assertTrue(data.user_file == "USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger_FRONT.txt")

    def _assert_move(self, state):
        move = state.move
        # Check the elements which were set on move
        self.assertTrue(move.sample_offset == 53.0/1000.)

        # Detector specific
        lab = move.detectors[DetectorType.to_string(DetectorType.LAB)]
        hab = move.detectors[DetectorType.to_string(DetectorType.HAB)]
        self.assertTrue(lab.x_translation_correction == -16.0/1000.)
        self.assertTrue(lab.z_translation_correction == 47.0/1000.)
        self.assertTrue(hab.x_translation_correction == -44.0/1000.)
        self.assertTrue(hab.y_translation_correction == -20.0/1000.)
        self.assertTrue(hab.z_translation_correction == 47.0/1000.)
        self.assertTrue(hab.rotation_correction == 0.0)

        # SANS2D-specific
        self.assertTrue(move.monitor_n_offset == -70.0/1000.)

    def _assert_mask(self, state):
        mask = state.mask
        self.assertTrue(mask.radius_min == 12/1000.)
        self.assertTrue(mask.radius_max == 15/1000.)
        self.assertTrue(mask.clear is True)
        self.assertTrue(mask.clear_time is True)
        self.assertTrue(mask.detectors[DetectorType.to_string(DetectorType.LAB)].single_horizontal_strip_mask == [0])
        self.assertTrue(mask.detectors[DetectorType.to_string(DetectorType.LAB)].single_vertical_strip_mask == [0, 191])
        self.assertTrue(mask.detectors[DetectorType.to_string(DetectorType.HAB)].single_horizontal_strip_mask == [0])
        self.assertTrue(mask.detectors[DetectorType.to_string(DetectorType.HAB)].single_vertical_strip_mask == [0, 191])
        self.assertTrue(mask.detectors[DetectorType.to_string(DetectorType.LAB)].range_horizontal_strip_start
                        == [190, 167])
        self.assertTrue(mask.detectors[DetectorType.to_string(DetectorType.LAB)].range_horizontal_strip_stop
                        == [191, 172])
        self.assertTrue(mask.detectors[DetectorType.to_string(DetectorType.HAB)].range_horizontal_strip_start
                        == [190, 156])
        self.assertTrue(mask.detectors[DetectorType.to_string(DetectorType.HAB)].range_horizontal_strip_stop
                        == [191, 159])

    def _assert_reduction(self, state):
        reduction = state.reduction
        self.assertTrue(reduction.reduction_mode is ISISReductionMode.LAB)
        self.assertFalse(reduction.merge_mask)
        self.assertTrue(reduction.merge_min == None)
        self.assertTrue(reduction.merge_max == None)

    def _assert_scale(self, state):
        scale = state.scale
        self.assertTrue(scale.scale == 0.074)

    def _assert_wavelength(self, state):
        wavelength = state.wavelength
        self.assertTrue(wavelength.wavelength_low == [1.5])
        self.assertTrue(wavelength.wavelength_high == [12.5])
        self.assertTrue(wavelength.wavelength_step == 0.125)
        self.assertTrue(wavelength.wavelength_step_type is RangeStepType.Lin)

    def _assert_convert_to_q(self, state):
        convert_to_q = state.convert_to_q
        self.assertTrue(convert_to_q.wavelength_cutoff == 8.0)
        self.assertTrue(convert_to_q.radius_cutoff == 0.2)
        self.assertTrue(convert_to_q.q_min == .001)
        self.assertTrue(convert_to_q.q_max == .2)
        self.assertTrue(convert_to_q.q_1d_rebin_string == "0.001,0.001,0.0126,-0.08,0.2")
        self.assertTrue(convert_to_q.use_gravity)

        self.assertTrue(convert_to_q.use_q_resolution)
        self.assertTrue(convert_to_q.q_resolution_a1 == 13./1000.)
        self.assertTrue(convert_to_q.q_resolution_a2 == 14./1000.)
        self.assertTrue(convert_to_q.q_resolution_delta_r == 11./1000.)
        self.assertTrue(convert_to_q.moderator_file == "moderator_rkh_file.txt")
        self.assertTrue(convert_to_q.q_resolution_collimation_length == 12.)

    def _assert_adjustment(self, state):
        adjustment = state.adjustment

        # Normalize to monitor settings
        normalize_to_monitor = adjustment.normalize_to_monitor
        self.assertTrue(normalize_to_monitor.prompt_peak_correction_min == 1000)
        self.assertTrue(normalize_to_monitor.prompt_peak_correction_max == 2000)
        self.assertTrue(normalize_to_monitor.rebin_type is RebinType.InterpolatingRebin)
        self.assertTrue(normalize_to_monitor.wavelength_low == [1.5])
        self.assertTrue(normalize_to_monitor.wavelength_high == [12.5])
        self.assertTrue(normalize_to_monitor.wavelength_step == 0.125)
        self.assertTrue(normalize_to_monitor.wavelength_step_type is RangeStepType.Lin)
        self.assertTrue(normalize_to_monitor.background_TOF_general_start == 3500)
        self.assertTrue(normalize_to_monitor.background_TOF_general_stop == 4500)
        self.assertTrue(normalize_to_monitor.background_TOF_monitor_start["1"] == 35000)
        self.assertTrue(normalize_to_monitor.background_TOF_monitor_stop["1"] == 65000)
        self.assertTrue(normalize_to_monitor.background_TOF_monitor_start["2"] == 85000)
        self.assertTrue(normalize_to_monitor.background_TOF_monitor_stop["2"] == 98000)
        self.assertTrue(normalize_to_monitor.incident_monitor == 1)

        # Calculate Transmission
        calculate_transmission = adjustment.calculate_transmission
        self.assertTrue(calculate_transmission.prompt_peak_correction_min == 1000)
        self.assertTrue(calculate_transmission.prompt_peak_correction_max == 2000)
        self.assertTrue(calculate_transmission.default_transmission_monitor == 3)
        self.assertTrue(calculate_transmission.default_incident_monitor == 2)
        self.assertTrue(calculate_transmission.incident_monitor == 1)
        self.assertTrue(calculate_transmission.transmission_radius_on_detector == 0.007)  # This is in mm
        self.assertTrue(calculate_transmission.transmission_roi_files == ["test.xml", "test2.xml"])
        self.assertTrue(calculate_transmission.transmission_mask_files == ["test3.xml", "test4.xml"])
        self.assertTrue(calculate_transmission.transmission_monitor == 4)
        self.assertTrue(calculate_transmission.rebin_type is RebinType.InterpolatingRebin)
        self.assertTrue(calculate_transmission.wavelength_low == [1.5])
        self.assertTrue(calculate_transmission.wavelength_high == [12.5])
        self.assertTrue(calculate_transmission.wavelength_step == 0.125)
        self.assertTrue(calculate_transmission.wavelength_step_type is RangeStepType.Lin)
        self.assertFalse(calculate_transmission.use_full_wavelength_range)
        self.assertTrue(calculate_transmission.wavelength_full_range_low ==
                        Configurations.SANS2D.wavelength_full_range_low)
        self.assertTrue(calculate_transmission.wavelength_full_range_high ==
                        Configurations.SANS2D.wavelength_full_range_high)
        self.assertTrue(calculate_transmission.background_TOF_general_start == 3500)
        self.assertTrue(calculate_transmission.background_TOF_general_stop == 4500)
        self.assertTrue(calculate_transmission.background_TOF_monitor_start["1"] == 35000)
        self.assertTrue(calculate_transmission.background_TOF_monitor_stop["1"] == 65000)
        self.assertTrue(calculate_transmission.background_TOF_monitor_start["2"] == 85000)
        self.assertTrue(calculate_transmission.background_TOF_monitor_stop["2"] == 98000)
        self.assertTrue(calculate_transmission.background_TOF_roi_start == 123)
        self.assertTrue(calculate_transmission.background_TOF_roi_stop == 466)
        self.assertTrue(calculate_transmission.fit[DataType.to_string(DataType.Sample)].fit_type is FitType.Logarithmic)
        self.assertTrue(calculate_transmission.fit[DataType.to_string(DataType.Sample)].wavelength_low == 1.5)
        self.assertTrue(calculate_transmission.fit[DataType.to_string(DataType.Sample)].wavelength_high == 12.5)
        self.assertTrue(calculate_transmission.fit[DataType.to_string(DataType.Sample)].polynomial_order == 0)
        self.assertTrue(calculate_transmission.fit[DataType.to_string(DataType.Can)].fit_type is FitType.Logarithmic)
        self.assertTrue(calculate_transmission.fit[DataType.to_string(DataType.Can)].wavelength_low == 1.5)
        self.assertTrue(calculate_transmission.fit[DataType.to_string(DataType.Can)].wavelength_high == 12.5)
        self.assertTrue(calculate_transmission.fit[DataType.to_string(DataType.Can)].polynomial_order == 0)

        # Wavelength and Pixel Adjustment
        wavelength_and_pixel_adjustment = adjustment.wavelength_and_pixel_adjustment
        self.assertTrue(wavelength_and_pixel_adjustment.wavelength_low == [1.5])
        self.assertTrue(wavelength_and_pixel_adjustment.wavelength_high == [12.5])
        self.assertTrue(wavelength_and_pixel_adjustment.wavelength_step == 0.125)
        self.assertTrue(wavelength_and_pixel_adjustment.wavelength_step_type is RangeStepType.Lin)
        self.assertTrue(wavelength_and_pixel_adjustment.adjustment_files[
                        DetectorType.to_string(DetectorType.LAB)].wavelength_adjustment_file ==
                        "DIRECTM1_15785_12m_31Oct12_v12.dat")
        self.assertTrue(wavelength_and_pixel_adjustment.adjustment_files[
                        DetectorType.to_string(DetectorType.HAB)].wavelength_adjustment_file ==
                        "DIRECTM1_15785_12m_31Oct12_v12.dat")

        # Assert wide angle correction
        self.assertTrue(state.adjustment.wide_angle_correction)

    def test_state_can_be_created_from_valid_user_file_with_data_information(self):
        # Arrange
        file_information = SANSFileInformationMock(instrument=SANSInstrument.SANS2D, run_number=22024)
        data_builder = get_data_builder(SANSFacility.ISIS, file_information,
                                        user_file="USER_SANS2D_154E_2p4_4m_M3_Xpress_8mm_SampleChanger_FRONT.txt")
        data_builder.set_sample_scatter("SANS2D00022024")
        data_builder.set_sample_scatter_period(3)
        data_state = data_builder.build()

        director = StateDirectorISIS(data_state, file_information)
        user_file_path = create_user_file(sample_user_file)

        director.set_user_file(user_file_path)
        state = director.construct()

        # Assert
        self._assert_data(state)
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

    def test_stat_can_be_created_from_valid_user_file_and_later_on_reset(self):
        # Arrange
        file_information = SANSFileInformationMock(instrument=SANSInstrument.SANS2D, run_number=22024)
        data_builder = get_data_builder(SANSFacility.ISIS, file_information)
        data_builder.set_sample_scatter("SANS2D00022024")
        data_builder.set_sample_scatter_period(3)
        data_state = data_builder.build()

        director = StateDirectorISIS(data_state, file_information)
        user_file_path = create_user_file(sample_user_file)
        director.set_user_file(user_file_path)

        # Set additional items
        director.set_mask_builder_radius_min(0.001298)
        director.set_mask_builder_radius_max(0.003298)
        director.set_scale_builder_width(1.)
        director.set_scale_builder_height(1.5)
        director.set_scale_builder_thickness(12.)
        director.set_scale_builder_shape(SampleShape.FlatPlate)

        # Act
        state = director.construct()

        # Assert
        self.assertTrue(state.mask.radius_min == 0.001298)
        self.assertTrue(state.mask.radius_max == 0.003298)
        self.assertTrue(state.scale.width == 1.)
        self.assertTrue(state.scale.height == 1.5)
        self.assertTrue(state.scale.thickness == 12.)
        self.assertTrue(state.scale.shape is SampleShape.FlatPlate)

        # clean up
        if os.path.exists(user_file_path):
            os.remove(user_file_path)

if __name__ == "__main__":
    unittest.main()

if __name__ == "__main__":
    unittest.main()
