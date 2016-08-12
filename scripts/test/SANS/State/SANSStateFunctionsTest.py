import unittest
import mantid

from SANS2.State.SANSStateFunctions import get_output_workspace_name
from SANS2.State.StateDirector.TestDirector import TestDirector
from SANS2.State.SANSStateData import SANSStateData
from SANS2.Common.SANSEnumerations import (ReductionDimensionality, ISISReductionMode)


class SANSStateFunctionsTest(unittest.TestCase):
    @staticmethod
    def _get_state():
        test_director = TestDirector()
        state = test_director.construct()

        state.data.sample_scatter_run_number = 12345
        state.data.sample_scatter_period = SANSStateData.ALL_PERIODS

        state.reduction.dimensionality = ReductionDimensionality.OneDim

        state.wavelength.wavelength_low = 12.0
        state.wavelength.wavelength_high = 34.0

        state.mask.phi_min = 12.0
        state.mask.phi_max = 56.0

        state.slice.start_time = [4.56778]
        state.slice.end_time = [12.373938]
        return state

    def test_that_unknown_reduction_mode_raises(self):
        # Arrange
        state = SANSStateFunctionsTest._get_state()

        # Act + Assert
        try:
            get_output_workspace_name(state, ISISReductionMode.All)
            did_raise = False
        except RuntimeError:
            did_raise = True
        self.assertTrue(did_raise)
    #
    # def test_that_creates_correct_workspace_name_for_1D(self):
    #     # Arrange
    #     state = SANSStateFunctionsTest._get_state()
    #     # Act
    #     output_workspace = get_output_workspace_name(state, ISISReductionMode.Lab)
    #     # Assert
    #     self.assertTrue("12345rear_1D12.0_34.0Phi12.0_56.0_t4.57_T12.37" == output_workspace)


if __name__ == '__main__':
    unittest.main()
