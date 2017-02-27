from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from sans.state.scale import get_scale_builder
from sans.state.data import get_data_builder
from sans.common.enums import (SANSFacility, SANSInstrument, SampleShape)


# ----------------------------------------------------------------------------------------------------------------------
#  State
# No tests required for the current states
# ----------------------------------------------------------------------------------------------------------------------

# ----------------------------------------------------------------------------------------------------------------------
#  Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateSliceEventBuilderTest(unittest.TestCase):
    def test_that_slice_event_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        data_builder = get_data_builder(facility)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_scale_builder(data_info)
        self.assertTrue(builder)

        builder.set_scale(1.0)
        builder.set_shape(SampleShape.Cuboid)
        builder.set_thickness(3.6)
        builder.set_width(3.7)
        builder.set_height(5.8)

        # Assert
        state = builder.build()
        self.assertTrue(state.shape is SampleShape.Cuboid)
        self.assertTrue(state.scale == 1.0)
        self.assertTrue(state.thickness == 3.6)
        self.assertTrue(state.width == 3.7)
        self.assertTrue(state.height == 5.8)


if __name__ == '__main__':
    unittest.main()
