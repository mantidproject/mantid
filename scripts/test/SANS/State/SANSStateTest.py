import unittest
import mantid

from mantid.kernel import (PropertyManagerProperty, PropertyManager)
from mantid.api import Algorithm

from SANS2.State.SANSState import (SANSStateISIS, SANSState)
from SANS2.State.SANSStateData import (SANSStateDataISIS, SANSStateData)
from SANS2.State.SANSStateMove import (SANSStateMoveLOQ)
from SANS2.State.SANSStateReduction import (SANSStateReductionISIS)
from SANS2.State.SANSStateSliceEvent import (SANSStateSliceEventISIS)
from SANS2.State.SANSStateMask import (SANSStateMaskISIS)
from SANS2.State.SANSStateWavelength import (SANSStateWavelengthISIS)
from SANS2.State.SANSStateSave import (SANSStateSaveISIS)


from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSEnumerations import (ISISReductionMode, ReductionDimensionality, FitModeForMerge,
                                           RangeStepType, RebinType)


class SANSStateTest(unittest.TestCase):
    def test_that_is_sans_state_object(self):
        state = SANSStateISIS()
        self.assertTrue(isinstance(state, SANSState))

    def test_that_can_set_and_get_values(self):
        # Arrange
        state = SANSStateISIS()

        # Add the different descriptors of the SANSState here:
        data_state = SANSStateDataISIS()
        data_state.sample_scatter = "sample_scat"
        state.data = data_state

        # Move
        move_state = SANSStateMoveLOQ()
        move_state.detectors[SANSConstants.high_angle_bank].detector_name = "test"
        move_state.detectors[SANSConstants.high_angle_bank].detector_name_short = "test"
        move_state.detectors[SANSConstants.low_angle_bank].detector_name = "test"
        move_state.detectors[SANSConstants.low_angle_bank].detector_name_short = "test"
        state.move = move_state

        # Reduction
        reduction_state = SANSStateReductionISIS()
        reduction_state.reduction_mode = ISISReductionMode.Merged
        reduction_state.dimensionality = ReductionDimensionality.TwoDim
        reduction_state.merge_shift = 12.65
        reduction_state.merge_scale = 34.6
        reduction_state.merge_fit_mode = FitModeForMerge.ShiftOnly
        state.reduction = reduction_state

        # Event Slice
        slice_state = SANSStateSliceEventISIS()
        slice_state.start_time = [12.3, 123.4, 34345.0]
        slice_state.end_time = [12.5, 200., 40000.0]
        state.slice = slice_state

        # Mask
        mask_state = SANSStateMaskISIS()
        mask_state.radius_min = 10.0
        mask_state.radius_max = 20.0
        state.mask = mask_state

        # Wavelength conversion
        wavelength_state = SANSStateWavelengthISIS()
        wavelength_state.wavelength_low = 10.0
        wavelength_state.wavelength_high = 20.0
        wavelength_state.wavelength_step = 2.0
        wavelength_state.wavelength_step_type = RangeStepType.Lin
        wavelength_state.rebin_type = RebinType.Rebin
        state.wavelength = wavelength_state

        # Save state
        save_state = SANSStateSaveISIS()
        save_state.file_name = "test_file_name"

        # Assert
        try:
            state.validate()
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertTrue(is_valid)

    def test_that_invalid_types_for_parameters_raise_type_error(self):
        # Arrange
        state = SANSStateISIS()

        # Act + Assert
        try:
            state.data = ["sdf"]
            is_valid = True
        except TypeError:
            is_valid = False
        self.assertFalse(is_valid)

    def test_that_descriptor_validators_work(self):
        # Arrange
        state = SANSStateISIS()

        # We are not setting sample_scatter on the SANSStateDataISIS making it invalid
        data = SANSStateDataISIS()

        # Act + Assert
        try:
            state.data = data
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertFalse(is_valid)

    def test_that_sans_state_holds_a_copy_of_the_substates_and_not_only_a_reference(self):
        # Arrange
        state = SANSStateISIS()
        data = SANSStateDataISIS()
        ws_name_1 = "sample_scat"
        ws_name_2 = "sample_scat2"
        data.sample_scatter = ws_name_1
        state.data = data

        # Act
        data.sample_scatter = ws_name_2

        # Assert
        stored_name = state.data.sample_scatter
        self.assertTrue(stored_name == ws_name_1)

    def test_that_property_manager_can_be_generated_from_state_object(self):
        class FakeAlgorithm(Algorithm):
            def PyInit(self):
                self.declareProperty(PropertyManagerProperty("Args"))

            def PyExec(self):
                pass
        # Arrange
        state = SANSStateISIS()

        # Prepare state data
        data_state = SANSStateDataISIS()
        ws_name_sample = "SANS2D00001234"
        ws_name_can = "SANS2D00001234"
        period = 3

        data_state.sample_scatter = ws_name_sample
        data_state.sample_scatter_period = period
        data_state.can_scatter = ws_name_can
        data_state.can_scatter_period = period

        state.data = data_state

        # Prepare the move
        move_state = SANSStateMoveLOQ()
        test_value = 12.4
        test_name = "test_name"
        move_state.detectors[SANSConstants.low_angle_bank].x_translation_correction = test_value
        move_state.detectors[SANSConstants.high_angle_bank].y_translation_correction = test_value
        move_state.detectors[SANSConstants.high_angle_bank].detector_name = test_name
        move_state.detectors[SANSConstants.high_angle_bank].detector_name_short = test_name
        move_state.detectors[SANSConstants.low_angle_bank].detector_name = test_name
        move_state.detectors[SANSConstants.low_angle_bank].detector_name_short = test_name
        state.move = move_state

        # Prepare the reduction
        reduction_state = SANSStateReductionISIS()
        reduction_state.reduction_mode = ISISReductionMode.Merged
        reduction_state.dimensionality = ReductionDimensionality.TwoDim
        reduction_state.merge_shift = 12.65
        reduction_state.merge_scale = 34.6
        reduction_state.merge_fit_mode = FitModeForMerge.ShiftOnly
        state.reduction = reduction_state

        # Prepare the event Slice
        slice_state = SANSStateSliceEventISIS()
        slice_state.start_time = [12.3, 123.4, 34345.0]
        slice_state.end_time = [12.5, 200., 40000.0]
        state.slice = slice_state

        # Prepare the mask
        mask_state = SANSStateMaskISIS()
        mask_state.radius_min = 10.0
        mask_state.radius_max = 20.0
        state.mask = mask_state

        # Wavelength conversion
        wavelength_state = SANSStateWavelengthISIS()
        wavelength_state.wavelength_low = 10.0
        wavelength_state.wavelength_high = 20.0
        wavelength_state.wavelength_step = 2.0
        wavelength_state.wavelength_step_type = RangeStepType.Lin
        wavelength_state.rebin_type = RebinType.Rebin
        state.wavelength = wavelength_state

        # Save state
        save_state = SANSStateSaveISIS()
        save_state.file_name = "test_file_name"

        # Act
        serialized = state.property_manager

        fake = FakeAlgorithm()
        fake.initialize()
        fake.setProperty("Args", serialized)
        property_manager = fake.getProperty("Args").value

        # Assert
        state_2 = SANSStateISIS()
        state_2.property_manager = property_manager

        self.assertTrue(state_2.data.sample_scatter == ws_name_sample)
        self.assertTrue(state_2.data.sample_scatter_period == period)
        self.assertTrue(state_2.data.can_scatter == ws_name_can)
        self.assertTrue(state_2.data.can_scatter_period == period)

        self.assertTrue(state_2.move.detectors[SANSConstants.low_angle_bank].x_translation_correction == test_value)
        self.assertTrue(state_2.move.detectors[SANSConstants.high_angle_bank].y_translation_correction == test_value)
        self.assertTrue(state_2.move.detectors[SANSConstants.high_angle_bank].detector_name == test_name)
        self.assertTrue(state_2.move.detectors[SANSConstants.high_angle_bank].detector_name_short == test_name)

        self.assertTrue(state_2.reduction.reduction_mode is ISISReductionMode.Merged)
        self.assertTrue(state_2.reduction.dimensionality is ReductionDimensionality.TwoDim)
        self.assertTrue(state_2.reduction.merge_shift == 12.65)
        self.assertTrue(state_2.reduction.merge_scale == 34.6)
        self.assertTrue(state_2.reduction.merge_fit_mode == FitModeForMerge.ShiftOnly)

        self.assertTrue(len(state_2.slice.start_time) == 3)
        self.assertTrue(len(state_2.slice.end_time) == 3)
        self.assertTrue(state_2.slice.start_time[1] == 123.4)

        self.assertTrue(state_2.mask.radius_min == 10.)
        self.assertTrue(state_2.mask.radius_max == 20.)

        self.assertTrue(state_2.wavelength.wavelength_low == 10.0)
        self.assertTrue(state_2.wavelength.wavelength_high == 20.0)
        self.assertTrue(state_2.wavelength.wavelength_step == 2.0)
        self.assertTrue(state_2.wavelength.wavelength_step_type is RangeStepType.Lin)
        self.assertTrue(state_2.wavelength.rebin_type is RebinType.Rebin)

        self.assertTrue(state_2.save.file_name == "test_file_name")

if __name__ == '__main__':
    unittest.main()
