import os
import unittest
import mantid


from SANS2.UserFile.UserFileStateDirector import UserFileStateDirectorISIS
from SANS2.Common.SANSEnumerations import (SANSFacility, ISISReductionMode, RangeStepType)
from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.UserFile.UserFileCommon import *

from SANS2.Common.SANSConstants import SANSConstants
from UserFileTestHelper import create_user_file, sample_user_file


# -----------------------------------------------------------------
# --- Tests -------------------------------------------------------
# -----------------------------------------------------------------
class UserFileStateDirectorISISTest(unittest.TestCase):
    def _assert_move(self, state):
        move = state.move
        # Check the elements which were set on move
        self.assertTrue(move.sample_offset == 53.0/1000.)

        # Detector specific
        lab = move.detectors[SANSConstants.low_angle_bank]
        hab = move.detectors[SANSConstants.high_angle_bank]
        self.assertTrue(lab.x_translation_correction == -16.0/1000.)
        self.assertTrue(lab.z_translation_correction == 47.0/1000.)
        self.assertTrue(hab.x_translation_correction == -44.0/1000.)
        self.assertTrue(hab.y_translation_correction == -20.0/1000.)
        self.assertTrue(hab.z_translation_correction == 47.0/1000.)
        self.assertTrue(hab.rotation_correction == 0.0)

        # SANS2D-specific
        self.assertTrue(move.monitor_4_offset == -70.0/1000.)

    def _assert_mask(self, state):
        mask = state.mask
        self.assertTrue(mask.radius_min == 12/1000.)
        self.assertTrue(mask.radius_max == 15/1000.)
        self.assertTrue(mask.clear is True)
        self.assertTrue(mask.clear_time is True)
        self.assertTrue(mask.detectors[SANSConstants.low_angle_bank].single_horizontal_strip_mask == [0])
        self.assertTrue(mask.detectors[SANSConstants.low_angle_bank].single_vertical_strip_mask == [0, 191])
        self.assertTrue(mask.detectors[SANSConstants.high_angle_bank].single_horizontal_strip_mask == [0])
        self.assertTrue(mask.detectors[SANSConstants.high_angle_bank].single_vertical_strip_mask == [0, 191])
        self.assertTrue(mask.detectors[SANSConstants.low_angle_bank].range_horizontal_strip_start == [190, 167])
        self.assertTrue(mask.detectors[SANSConstants.low_angle_bank].range_horizontal_strip_stop == [191, 172])
        self.assertTrue(mask.detectors[SANSConstants.high_angle_bank].range_horizontal_strip_start == [190, 156])
        self.assertTrue(mask.detectors[SANSConstants.high_angle_bank].range_horizontal_strip_stop == [191, 159])

    def _assert_reduction(self, state):
        reduction = state.reduction
        self.assertTrue(reduction.reduction_mode is ISISReductionMode.Lab)

    def _assert_wavelength(self, state):
        wavelength = state.wavelength
        self.assertTrue(wavelength.wavelength_low == 1.5)
        self.assertTrue(wavelength.wavelength_high == 12.5)
        self.assertTrue(wavelength.wavelength_step == 0.125)
        self.assertTrue(wavelength.wavelength_step_type is RangeStepType.Lin)

    def test_state_can_be_created_from_valid_user_file_with_data_information(self):
        # Arrange
        data_builder = get_data_builder(SANSFacility.ISIS)
        data_builder.set_sample_scatter("SANS2D00022024")
        data_builder.set_sample_scatter_period(3)
        data_state = data_builder.build()

        director = UserFileStateDirectorISIS(data_state)
        user_file_path = create_user_file(sample_user_file)

        director.set_user_file(user_file_path)
        # TODO: Add manual settings

        state = director.construct()

        # Assert
        self._assert_move(state)
        self._assert_mask(state)
        self._assert_reduction(state)
        self._assert_wavelength(state)

        # clean up
        if os.path.exists(user_file_path):
            os.remove(user_file_path)


if __name__ == "__main__":
    unittest.main()
