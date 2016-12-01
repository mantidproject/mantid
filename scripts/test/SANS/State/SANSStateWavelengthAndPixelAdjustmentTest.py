import unittest
import mantid


from SANS2.State.SANSStateWavelengthAndPixelAdjustment import (SANSStateWavelengthAndPixelAdjustment,
                                                               SANSStateWavelengthAndPixelAdjustmentISIS)
from StateTestHelper import assert_validate_error, assert_raises_nothing

from SANS2.Common.SANSType import (RebinType, RangeStepType, DetectorType, convert_detector_type_to_string)


class SANSStateWavelengthAndPixelAdjustmentTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateWavelengthAndPixelAdjustmentISIS()
        self.assertTrue(isinstance(state, SANSStateWavelengthAndPixelAdjustment))

    def test_that_raises_when_wavelength_entry_is_missing(self):
        # Arrange
        state = SANSStateWavelengthAndPixelAdjustmentISIS()
        assert_validate_error(self, ValueError, state)
        state.wavelength_low = 1.
        assert_validate_error(self, ValueError, state)
        state.wavelength_high = 2.
        assert_validate_error(self, ValueError, state)
        state.wavelength_step = 2.
        assert_validate_error(self, ValueError, state)
        state.wavelength_step_type = RangeStepType.Lin
        assert_raises_nothing(self, state)

    def test_that_raises_when_lower_wavelength_is_smaller_than_high_wavelength(self):
        state = SANSStateWavelengthAndPixelAdjustmentISIS()
        state.wavelength_low = 2.
        state.wavelength_high = 1.
        state.wavelength_step = 2.
        state.wavelength_step_type = RangeStepType.Lin
        assert_validate_error(self, ValueError, state)


if __name__ == '__main__':
    unittest.main()
