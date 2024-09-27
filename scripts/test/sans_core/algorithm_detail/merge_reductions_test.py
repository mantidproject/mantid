# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from sans_core.algorithm_detail.bundles import OutputPartsBundle, ReducedSlice
from sans_core.algorithm_detail.merge_reductions import MergeFactory, ISIS1DMerger
from sans_core.common.constants import EMPTY_NAME
from sans_core.common.enums import DataType, ReductionMode
from sans_core.common.enums import ReductionDimensionality, FitModeForMerge
from sans_core.common.general_functions import create_unmanaged_algorithm
from sans_core.state.StateObjects.StateReductionMode import StateReductionMode
from sans_core.test_helper.test_director import TestDirector


# Test merge reductions
# This fits the high angle workspace to the low angle workspace
# By construction of the data, the HAB is shifted up by 4, with a gradient of 2x the LAB
# Results in the following data:
# HAB y[x=0] = 5.0
# LAB y[x=1] = 1.0
# HAB y[x=2] = 13
# LAB y[x=2] = 5
# .......
class MergeReductionsTest(unittest.TestCase):
    @staticmethod
    def create_1D_workspace(data_x, data_y):
        create_name = "CreateWorkspace"
        create_options = {"DataX": data_x, "DataY": data_y, "NSpec": 1, "UnitX": "MomentumTransfer", "OutputWorkspace": EMPTY_NAME}
        create_alg = create_unmanaged_algorithm(create_name, **create_options)
        create_alg.execute()
        return create_alg.getProperty("OutputWorkspace").value

    @staticmethod
    def _get_simple_state(fit_type=FitModeForMerge.NO_FIT, scale=1.0, shift=0.0):
        # Set the reduction parameters
        reduction_info = StateReductionMode()
        reduction_info.reduction_mode = ReductionMode.MERGED
        reduction_info.dimensionality = ReductionDimensionality.TWO_DIM
        reduction_info.merge_shift = shift
        reduction_info.merge_scale = scale
        reduction_info.merge_fit_mode = fit_type

        # Get the sample state
        test_director = TestDirector()
        test_director.set_states(reduction_state=reduction_info)
        return test_director.construct()

    @staticmethod
    def _create_workspaces(state, data_type, data_x_lab, data_y_lab_count, data_y_lab_norm, data_x_hab, data_y_hab_count, data_y_hab_norm):
        lab_count = MergeReductionsTest.create_1D_workspace(data_x_lab, data_y_lab_count)
        lab_norm = MergeReductionsTest.create_1D_workspace(data_x_lab, data_y_lab_norm)
        lab_bundle = OutputPartsBundle(
            state=state,
            data_type=data_type,
            reduction_mode=ReductionMode.LAB,
            output_workspace_count=lab_count,
            output_workspace_norm=lab_norm,
        )

        mock_out_bundle = mock.Mock()
        mock_out_bundle.state = state
        lab_slice = ReducedSlice(parts_bundle=lab_bundle, wav_range=None, transmission_bundle=None, output_bundle=mock_out_bundle)

        hab_count = MergeReductionsTest.create_1D_workspace(data_x_hab, data_y_hab_count)
        hab_norm = MergeReductionsTest.create_1D_workspace(data_x_hab, data_y_hab_norm)
        hab_bundle = OutputPartsBundle(
            state=state,
            data_type=data_type,
            reduction_mode=ReductionMode.HAB,
            output_workspace_count=hab_count,
            output_workspace_norm=hab_norm,
        )
        hab_slice = ReducedSlice(parts_bundle=hab_bundle, wav_range=None, transmission_bundle=None, output_bundle=mock_out_bundle)
        return lab_slice, hab_slice

    @staticmethod
    def _provide_data(state):
        # Generate data for the tests, we generate data with the following rules
        # sample_lab : Linear data y = 2 + 2x
        # sample_lab : uniform data y = 1
        # sample_hab : Linear data y = 6 + 4*x
        # sample_norm : uniform data y = 1
        # can_lab, can_hab : uniform data y = 1

        # Create data for sample
        data_x_lab = list(range(0, 10))
        data_y_lab_count = [2 + 2 * x for x in data_x_lab]
        data_y_lab_norm = [1] * 10

        data_x_hab = list(range(0, 10))
        data_y_hab_count = [6 + 4 * x for x in data_x_hab]
        data_y_hab_norm = [1] * 10

        sample_lab, sample_hab = MergeReductionsTest._create_workspaces(
            state, DataType.SAMPLE, data_x_lab, data_y_lab_count, data_y_lab_norm, data_x_hab, data_y_hab_count, data_y_hab_norm
        )
        # Create data for can
        data_x_lab = list(range(0, 10))
        data_y_lab_count = [1] * 10
        data_y_lab_norm = [1] * 10

        data_x_hab = list(range(0, 10))
        data_y_hab_count = [1] * 10
        data_y_hab_norm = [1] * 10

        can_lab, can_hab = MergeReductionsTest._create_workspaces(
            state, DataType.CAN, data_x_lab, data_y_lab_count, data_y_lab_norm, data_x_hab, data_y_hab_count, data_y_hab_norm
        )

        def _set_sample(mock_bundle, data_type: DataType):
            mock_bundle.output_bundle.data_type = data_type

        _set_sample(sample_lab, DataType.SAMPLE)
        _set_sample(sample_hab, DataType.SAMPLE)
        _set_sample(can_lab, DataType.CAN)
        _set_sample(can_hab, DataType.CAN)
        return sample_lab, sample_hab, can_lab, can_hab

    def test_that_correct_merger_is_generated(self):
        # Arrange
        state = self._get_simple_state()
        merge_factory = MergeFactory()

        # Act
        merger = merge_factory.create_merger(state)

        # Assert
        self.assertTrue(isinstance(merger, ISIS1DMerger))

    def test_that_can_merge_without_fitting(self):
        # Without fitting, the scale and shift returned will be identical to the input
        fit_type = FitModeForMerge.NO_FIT
        scale_input = 0.5
        shift_input = -1.5
        state = self._get_simple_state(fit_type, scale_input, shift_input)
        merge_factory = MergeFactory()
        merger = merge_factory.create_merger(state)

        sample_lab, sample_hab, can_lab, can_hab = self._provide_data(state)

        bundles = {ReductionMode.LAB: [sample_lab, can_lab], ReductionMode.HAB: [sample_hab, can_hab]}

        # Act
        result = merger.merge(bundles)
        merged_workspace = result.merged_workspace

        scale = result.scale
        shift = result.shift
        self.assertTrue(abs(scale - scale_input) < 1e-4)
        self.assertTrue(abs(shift - shift_input) < 1e-4)

        # There is an overlap of two bins between HAB and LAB, the values are tested in SANSStitch
        self.assertEqual(merged_workspace.blocksize(), 10)

    def test_that_can_merge_fitting(self):
        # If we fit both, we required that
        # 1 = f0.A0 + scaling*5    : @ x  = 0
        # 5 = f0.A0 + scaling*13   : @ x  = 2
        # This is a linear problem, in 2 variables, i.e. it has 1 unique solution: scaling = 0.5 : f0.A0 = -1.5
        # returning shift is f0.A0/scaling = -3
        fit_type = FitModeForMerge.BOTH
        scale_input = 0.4
        shift_input = -1
        expected_scale = 0.5
        expected_shift = -1.5 / 0.5
        state = self._get_simple_state(fit_type, scale_input, shift_input)
        merge_factory = MergeFactory()
        merger = merge_factory.create_merger(state)

        sample_lab, sample_hab, can_lab, can_hab = self._provide_data(state)

        bundles = {ReductionMode.LAB: [sample_lab, can_lab], ReductionMode.HAB: [sample_hab, can_hab]}

        # Act
        result = merger.merge(bundles)
        merged_workspace = result.merged_workspace

        self.assertEqual(merged_workspace.blocksize(), 10)

        scale = result.scale
        shift = result.shift
        self.assertNotEqual(scale, scale_input)
        self.assertNotEqual(shift, shift_input)
        self.assertTrue(abs(scale - expected_scale) < 1e-4)
        self.assertTrue(abs(shift - expected_shift) < 1e-4)

    def test_that_can_merge_with_shift_only_fitting(self):
        # Same argument as above, tie scale to 0.5 and fit shift : should result in shift of -3
        fit_type = FitModeForMerge.SHIFT_ONLY
        scale_input = 0.5
        shift_input = -1.0
        expected_shift = -3
        state = self._get_simple_state(fit_type, scale_input, shift_input)
        merge_factory = MergeFactory()
        merger = merge_factory.create_merger(state)

        sample_lab, sample_hab, can_lab, can_hab = self._provide_data(state)
        bundles = {ReductionMode.LAB: [sample_lab, can_lab], ReductionMode.HAB: [sample_hab, can_hab]}

        # Act
        result = merger.merge(bundles)
        merged_workspace = result.merged_workspace

        self.assertEqual(merged_workspace.blocksize(), 10)

        scale = result.scale
        shift = result.shift

        self.assertNotEqual(shift, shift_input)
        self.assertTrue(abs(scale - scale_input) < 1e-4)
        self.assertTrue(abs(shift - expected_shift) < 1e-4)

    def test_that_can_merge_with_scale_only_fitting(self):
        # Same arguments as above, tie shift to -3 and fit scale : should result in scale of 0.5
        fit_type = FitModeForMerge.SCALE_ONLY
        scale_input = 0.1
        shift_input = -3
        expected_scale = 0.5
        state = self._get_simple_state(fit_type, scale_input, shift_input)
        merge_factory = MergeFactory()
        merger = merge_factory.create_merger(state)

        sample_lab, sample_hab, can_lab, can_hab = self._provide_data(state)
        bundles = {ReductionMode.LAB: [sample_lab, can_lab], ReductionMode.HAB: [sample_hab, can_hab]}

        # Act
        result = merger.merge(bundles)
        merged_workspace = result.merged_workspace

        self.assertEqual(merged_workspace.blocksize(), 10)

        scale = result.scale
        shift = result.shift

        self.assertNotEqual(scale, scale_input)
        self.assertLess(abs(scale - expected_scale), 1e-4)
        self.assertLess(abs(shift - shift_input), 1e-4)


if __name__ == "__main__":
    unittest.main()
