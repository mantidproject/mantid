import unittest
import mantid

from SANS2.State.StateBuilder.SANSStateMaskBuilder import get_mask_builder
from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSType import (SANSFacility, SANSInstrument)


class SANSStateMaskBuilderTest(unittest.TestCase):
    def test_that_mask_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        data_builder = get_data_builder(facility)
        data_builder.set_sample_scatter("LOQ74044")
        data_builder.set_sample_scatter_period(3)
        data_info = data_builder.build()

        # Act
        builder = get_mask_builder(data_info)
        self.assertTrue(builder)

        start_time = [0.1, 1.3]
        end_time = [0.2, 1.6]
        builder.set_bin_mask_general_start(start_time)
        builder.set_bin_mask_general_stop(end_time)
        builder.set_LAB_single_vertical_strip_mask([1, 2, 3])

        # Assert
        state = builder.build()
        self.assertTrue(len(state.bin_mask_general_start) == 2)
        self.assertTrue(state.bin_mask_general_start[0] == start_time[0])
        self.assertTrue(state.bin_mask_general_start[1] == start_time[1])

        self.assertTrue(len(state.bin_mask_general_stop) == 2)
        self.assertTrue(state.bin_mask_general_stop[0] == end_time[0])
        self.assertTrue(state.bin_mask_general_stop[1] == end_time[1])

        strip_mask = state.detectors[SANSConstants.low_angle_bank].single_vertical_strip_mask
        self.assertTrue(len(strip_mask) == 3)
        self.assertTrue(strip_mask[2] == 3)


if __name__ == '__main__':
    unittest.main()
