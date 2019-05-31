# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
import mantid

from sans.state.reduction_mode import (StateReductionMode, get_reduction_mode_builder)
from sans.state.data import get_data_builder
from sans.common.enums import (ISISReductionMode, ReductionDimensionality, FitModeForMerge,
                               SANSFacility, SANSInstrument, DetectorType)
from sans.test_helper.file_information_mock import SANSFileInformationMock


# ----------------------------------------------------------------------------------------------------------------------
# State
# ----------------------------------------------------------------------------------------------------------------------
class StateReductionModeTest(unittest.TestCase):
    def test_that_converter_methods_work(self):
        # Arrange
        state = StateReductionMode()

        state.reduction_mode = ISISReductionMode.Merged
        state.dimensionality = ReductionDimensionality.TwoDim
        state.merge_shift = 12.65
        state.merge_scale = 34.6
        state.merge_fit_mode = FitModeForMerge.ShiftOnly

        state.detector_names[DetectorType.to_string(DetectorType.LAB)] = "Test1"
        state.detector_names[DetectorType.to_string(DetectorType.HAB)] = "Test2"

        state.merge_mask = True
        state.merge_min = 78.89
        state.merge_max = 56.4


        # Assert
        merge_strategy = state.get_merge_strategy()
        self.assertTrue(merge_strategy[0] is ISISReductionMode.LAB)
        self.assertTrue(merge_strategy[1] is ISISReductionMode.HAB)

        all_reductions = state.get_all_reduction_modes()
        self.assertEqual(len(all_reductions),  2)
        self.assertTrue(all_reductions[0] is ISISReductionMode.LAB)
        self.assertTrue(all_reductions[1] is ISISReductionMode.HAB)

        result_lab = state.get_detector_name_for_reduction_mode(ISISReductionMode.LAB)
        self.assertEqual(result_lab,  "Test1")
        result_hab = state.get_detector_name_for_reduction_mode(ISISReductionMode.HAB)
        self.assertEqual(result_hab,  "Test2")

        self.assertRaises(RuntimeError, state.get_detector_name_for_reduction_mode, "non_sense")


# ----------------------------------------------------------------------------------------------------------------------
# Builder
# ----------------------------------------------------------------------------------------------------------------------
class StateReductionModeBuilderTest(unittest.TestCase):
    def test_that_reduction_state_can_be_built(self):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_info = data_builder.build()

        # Act
        builder = get_reduction_mode_builder(data_info)
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

        merge_mask = True
        merge_min = 12.23
        merge_max = 45.89
        builder.set_merge_mask(merge_mask)
        builder.set_merge_min(merge_min)
        builder.set_merge_max(merge_max)

        state = builder.build()

        # Assert
        self.assertTrue(state.reduction_mode is mode)
        self.assertTrue(state.reduction_dimensionality is dim)
        self.assertEqual(state.merge_fit_mode,  fit_mode)
        self.assertEqual(state.merge_shift,  merge_shift)
        self.assertEqual(state.merge_scale,  merge_scale)
        detector_names = state.detector_names
        self.assertEqual(detector_names[DetectorType.to_string(DetectorType.LAB)],  "main-detector-bank")
        self.assertTrue(state.merge_mask)
        self.assertEqual(state.merge_min,  merge_min)
        self.assertEqual(state.merge_max,  merge_max)


if __name__ == '__main__':
    unittest.main()
