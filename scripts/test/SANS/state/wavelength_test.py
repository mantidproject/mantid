import unittest
import mantid

from sans.state.wavelength import (StateWavelength, get_wavelength_builder)
from sans.state.data import get_data_builder
from sans.common.enums import (SANSFacility, SANSInstrument, RebinType, RangeStepType)
from state_test_helper import assert_validate_error, assert_raises_nothing


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateWavelengthTest(unittest.TestCase):

    def test_that_is_sans_state_data_object(self):
        state = StateWavelength()
        self.assertTrue(isinstance(state, StateWavelength))

    def test_that_raises_when_wavelength_entry_is_missing(self):
        # Arrange
        state = StateWavelength()
        assert_validate_error(self, ValueError, state)
        state.wavelength_low = 1.
        assert_validate_error(self, ValueError, state)
        state.wavelength_high = 2.
        assert_validate_error(self, ValueError, state)
        state.wavelength_step = 2.
        assert_raises_nothing(self, state)

    def test_that_raises_when_lower_wavelength_is_smaller_than_high_wavelength(self):
        state = StateWavelength()
        state.wavelength_low = 2.
        state.wavelength_high = 1.
        state.wavelength_step = 2.
        assert_validate_error(self, ValueError, state)


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------

class StateSliceEventBuilderTest(unittest.TestCase):
    def test_that_slice_event_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        data_builder = get_data_builder(facility)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_wavelength_builder(data_info)
        self.assertTrue(builder)

        builder.set_wavelength_low(10.0)
        builder.set_wavelength_high(20.0)
        builder.set_wavelength_step(3.0)
        builder.set_wavelength_step_type(RangeStepType.Lin)
        builder.set_rebin_type(RebinType.Rebin)

        # Assert
        state = builder.build()

        self.assertTrue(state.wavelength_low == 10.0)
        self.assertTrue(state.wavelength_high == 20.0)
        self.assertTrue(state.wavelength_step_type is RangeStepType.Lin)
        self.assertTrue(state.rebin_type is RebinType.Rebin)


if __name__ == '__main__':
    unittest.main()
