import unittest
import mantid

from SANS2.State.StateBuilder.SANSStateDataBuilder import get_data_builder
from SANS2.State.StateBuilder.SANSStateReductionBuilder import get_reduction_builder
from SANS2.Common.SANSType import (ISISReductionMode, ReductionDimensionality, FitModeForMerge)
from SANS2.Common.SANSConstants import SANSConstants
from SANS2.Common.SANSType import (SANSFacility, SANSInstrument)


class SANSStateReductionBuilderTest(unittest.TestCase):
    def test_that_reduction_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        data_builder = get_data_builder(facility)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_reduction_builder(data_info)
        self.assertTrue(builder)

        mode = ISISReductionMode.Merged
        dim = ReductionDimensionality.OneDim
        builder.set_reduction_mode(mode)
        builder.set_reduction_dimensionality(dim)

        merge_shift = 324.2
        merge_scale = 3420.98
        fit_mode = FitModeForMerge.Both
        builder.set_merge_fit_mode(fit_mode)
        builder.set_merge_shift(merge_shift)
        builder.set_merge_scale(merge_scale)

        state = builder.build()

        # Assert
        self.assertTrue(state.reduction_mode is mode)
        self.assertTrue(state.reduction_dimensionality is dim)
        self.assertTrue(state.merge_fit_mode == fit_mode)
        self.assertTrue(state.merge_shift == merge_shift)
        self.assertTrue(state.merge_scale == merge_scale)
        detector_names = state.detector_names
        self.assertTrue(detector_names[SANSConstants.low_angle_bank] == "main-detector-bank")

if __name__ == '__main__':
    unittest.main()
