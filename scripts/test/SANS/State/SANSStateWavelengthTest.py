import unittest
import mantid

from SANS2.State.SANSStateWavelength import (SANSStateWavelength, SANSStateWavelengthISIS)
from StateTestHelper import assert_validate_error, assert_raises_nothing


class SANSStateWavelengthTest(unittest.TestCase):

    def test_that_is_sans_state_data_object(self):
        state = SANSStateWavelengthISIS()
        self.assertTrue(isinstance(state, SANSStateWavelength))

    def test_that_raises_when_wavelength_entry_is_missing(self):
        # Arrange
        state = SANSStateWavelengthISIS()
        assert_validate_error(self, ValueError, state)
        state.wavelength_low = 1.
        assert_validate_error(self, ValueError, state)
        state.wavelength_high = 2.
        assert_validate_error(self, ValueError, state)
        state.wavelength_step = 2.
        assert_raises_nothing(self, state)

    def test_that_raises_when_lower_wavelength_is_smaller_than_high_wavelength(self):
        state = SANSStateWavelengthISIS()
        state.wavelength_low = 2.
        state.wavelength_high = 1.
        state.wavelength_step = 2.
        assert_validate_error(self, ValueError, state)

if __name__ == '__main__':
    unittest.main()
