import unittest
import mantid

from SANS2.State.StateBuilder.SANSStateWavelengthBuilder import get_wavelength_builder
from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.Common.SANSType import (SANSFacility, SANSInstrument, RebinType, RangeStepType)


class SANSStateSliceEventBuilderTest(unittest.TestCase):
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
