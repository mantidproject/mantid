import unittest
import mantid

from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.State.StateBuilder.SANSStateConvertToQBuilder import get_convert_to_q_builder
from SANS2.Common.SANSType import (RangeStepType, ReductionDimensionality, SANSFacility)


class SANSStateConvertToQBuilderTest(unittest.TestCase):
    def test_that_reduction_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        data_builder = get_data_builder(facility)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_convert_to_q_builder(data_info)
        self.assertTrue(builder)

        builder.set_q_min(12.0)
        builder.set_q_max(17.0)
        builder.set_q_step(1.)
        builder.set_q_step_type(RangeStepType.Lin)
        builder.set_reduction_dimensionality(ReductionDimensionality.OneDim)

        state = builder.build()

        # Assert
        self.assertTrue(state.q_min == 12.0)
        self.assertTrue(state.q_max == 17.0)
        self.assertTrue(state.q_step == 1.)
        self.assertTrue(state.q_step_type is RangeStepType.Lin)
        self.assertTrue(state.reduction_dimensionality is ReductionDimensionality.OneDim)


if __name__ == '__main__':
    unittest.main()
