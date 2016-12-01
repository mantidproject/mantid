import unittest
import mantid

from mantid.kernel import (PropertyManagerProperty, PropertyManager)
from mantid.api import Algorithm

from SANS2.State.SANSStateAdjustment import (SANSStateAdjustment, SANSStateAdjustmentISIS)
from SANS2.State.SANSStateCalculateTransmission import SANSStateCalculateTransmission
from SANS2.State.SANSStateNormalizeToMonitor import SANSStateNormalizeToMonitor
from SANS2.State.SANSStateWavelengthAndPixelAdjustment import SANSStateWavelengthAndPixelAdjustment


class MockStateNormalizeToMonitor(SANSStateNormalizeToMonitor):
    def validate(self):
        pass


class MockStateCalculateTransmission(SANSStateCalculateTransmission):
    def validate(self):
        pass


class MockStateWavelengthAndPixelAdjustment(SANSStateWavelengthAndPixelAdjustment):
    def validate(self):
        pass


class SANSStateReductionTest(unittest.TestCase):
    def test_that_is_sans_state_data_object(self):
        state = SANSStateAdjustmentISIS()
        self.assertTrue(isinstance(state, SANSStateAdjustment))

    def test_that_raises_when_calculate_transmission_is_not_set(self):
        state = SANSStateAdjustmentISIS()
        state.normalize_to_monitor = MockStateNormalizeToMonitor()
        state.wavelength_and_pixel_adjustment = MockStateWavelengthAndPixelAdjustment()
        self.assertRaises(ValueError, state.validate)
        state.calculate_transmission = MockStateCalculateTransmission()
        try:
            state.validate()
        except ValueError:
            self.fail()

    def test_that_raises_when_normalize_to_monitor_is_not_set(self):
        state = SANSStateAdjustmentISIS()
        state.calculate_transmission = MockStateCalculateTransmission()
        state.wavelength_and_pixel_adjustment = MockStateWavelengthAndPixelAdjustment()
        self.assertRaises(ValueError, state.validate)
        state.normalize_to_monitor = MockStateNormalizeToMonitor()
        try:
            state.validate()
        except ValueError:
            self.fail()

    def test_that_raises_when_wavelength_and_pixel_adjustment_is_not_set(self):
        state = SANSStateAdjustmentISIS()
        state.calculate_transmission = MockStateCalculateTransmission()
        state.normalize_to_monitor = MockStateNormalizeToMonitor()
        self.assertRaises(ValueError, state.validate)
        state.wavelength_and_pixel_adjustment = MockStateWavelengthAndPixelAdjustment()
        try:
            state.validate()
        except ValueError:
            self.fail()


if __name__ == '__main__':
    unittest.main()
