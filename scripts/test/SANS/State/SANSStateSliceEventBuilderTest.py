import unittest
import mantid

from SANS2.State.StateBuilder.SANSStateSliceEventBuilder import get_slice_event_builder
from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.Common.SANSType import (SANSFacility, SANSInstrument)


class SANSStateSliceEventBuilderTest(unittest.TestCase):
    def test_that_slice_event_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        data_builder = get_data_builder(facility)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_slice_event_builder(data_info)
        self.assertTrue(builder)

        start_time = [0.1, 1.3]
        end_time = [0.2, 1.6]
        builder.set_start_time(start_time)
        builder.set_end_time(end_time)

        # Assert
        state = builder.build()
        self.assertTrue(len(state.start_time) == 2)
        self.assertTrue(state.start_time[0] == start_time[0])
        self.assertTrue(state.start_time[1] == start_time[1])

        self.assertTrue(len(state.end_time) == 2)
        self.assertTrue(state.end_time[0] == end_time[0])
        self.assertTrue(state.end_time[1] == end_time[1])


if __name__ == '__main__':
    unittest.main()
