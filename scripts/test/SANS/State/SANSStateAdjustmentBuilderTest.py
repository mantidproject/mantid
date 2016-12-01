import unittest
import mantid

from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.State.StateBuilder.SANSStateAdjustmentBuilder import get_adjustment_builder
from SANS2.State.StateDirector.TestDirector import TestDirector
from SANS2.Common.SANSType import (RebinType, RangeStepType, DataType, convert_reduction_data_type_to_string)
from SANS2.Common.SANSType import (SANSFacility, SANSInstrument, FitType)


class SANSStateAdjustmentBuilderTest(unittest.TestCase):
    def test_that_reduction_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        data_builder = get_data_builder(facility)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        test_director = TestDirector()
        sample_state = test_director.construct()
        calculate_transmission_state = sample_state.adjustment.calculate_transmission
        normalize_to_monitor_state = sample_state.adjustment.normalize_to_monitor
        wavelength_and_pixel_adjustment_state = sample_state.adjustment.wavelength_and_pixel_adjustment

        # Act
        builder = get_adjustment_builder(data_info)
        self.assertTrue(builder)

        builder.set_calculate_transmission(calculate_transmission_state)
        builder.set_normalize_to_monitor(normalize_to_monitor_state)
        builder.set_wavelength_and_pixel_adjustment(wavelength_and_pixel_adjustment_state)
        builder.set_wide_angle_correction(False)
        state = builder.build()

        # # Assert
        self.assertTrue(not state.wide_angle_correction)

        try:
            state.validate()
            is_valid = True
        except ValueError:
            is_valid = False
        self.assertTrue(is_valid)


if __name__ == '__main__':
    unittest.main()
