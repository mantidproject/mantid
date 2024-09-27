# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from sans_core.common.enums import ReductionMode, ReductionDimensionality, FitModeForMerge
from sans_core.state.StateObjects.StateReductionMode import StateReductionMode


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateReductionModeTest(unittest.TestCase):
    def test_that_converter_methods_work(self):
        # Arrange
        state = StateReductionMode()

        state.reduction_mode = ReductionMode.MERGED
        state.dimensionality = ReductionDimensionality.TWO_DIM
        state.merge_shift = 12.65
        state.merge_scale = 34.6
        state.merge_fit_mode = FitModeForMerge.SHIFT_ONLY

        state.merge_mask = True
        state.merge_min = 78.89
        state.merge_max = 56.4

        # Assert
        merge_strategy = state.get_merge_strategy()
        self.assertEqual(merge_strategy[0], ReductionMode.LAB)
        self.assertEqual(merge_strategy[1], ReductionMode.HAB)

        all_reductions = state.get_all_reduction_modes()
        self.assertEqual(len(all_reductions), 2)
        self.assertEqual(all_reductions[0], ReductionMode.LAB)
        self.assertEqual(all_reductions[1], ReductionMode.HAB)


if __name__ == "__main__":
    unittest.main()
