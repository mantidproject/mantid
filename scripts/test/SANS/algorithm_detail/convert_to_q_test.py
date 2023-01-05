# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.simpleapi import DeleteWorkspace
from sans.algorithm_detail.convert_to_q import convert_workspace
from sans.common.enums import SANSFacility, ReductionDimensionality, RangeStepType, SANSInstrument
from sans.common.general_functions import create_unmanaged_algorithm
from sans.state.StateObjects.StateConvertToQ import get_convert_to_q_builder
from sans.state.StateObjects.StateData import get_data_builder
from sans.test_helper.file_information_mock import SANSFileInformationMock
from sans.test_helper.test_director import TestDirector


class ConvertToQTest(unittest.TestCase):
    @staticmethod
    def _get_workspace(is_adjustment):
        bank_pixel_width = 1 if is_adjustment else 2
        sample_name = "CreateSampleWorkspace"
        sample_options = {
            "WorkspaceType": "Histogram",
            "NumBanks": 1,
            "BankPixelWidth": bank_pixel_width,
            "OutputWorkspace": "test",
            "Function": "Flat background",
            "XUnit": "Wavelength",
            "XMin": 1.0,
            "XMax": 10.0,
            "BinWidth": 2.0,
        }
        sample_alg = create_unmanaged_algorithm(sample_name, **sample_options)
        sample_alg.execute()
        workspace = sample_alg.getProperty("OutputWorkspace").value
        return workspace

    @staticmethod
    def _get_sample_state(
        q_min=1.0,
        q_max=2.0,
        q_step=0.1,
        q_step_type=RangeStepType.LIN,
        q_xy_max=None,
        q_xy_step=None,
        use_gravity=False,
        dim=ReductionDimensionality.ONE_DIM,
    ):
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_state = data_builder.build()

        convert_to_q_builder = get_convert_to_q_builder(data_state)
        convert_to_q_builder.set_reduction_dimensionality(dim)
        convert_to_q_builder.set_use_gravity(use_gravity)
        convert_to_q_builder.set_radius_cutoff(0.002)
        convert_to_q_builder.set_wavelength_cutoff(2.0)
        convert_to_q_builder.set_q_min(q_min)
        convert_to_q_builder.set_q_max(q_max)
        prefix = 1.0 if q_step_type is RangeStepType.LIN else -1.0
        q_step *= prefix
        rebin_string = str(q_min) + "," + str(q_step) + "," + str(q_max)
        convert_to_q_builder.set_q_1d_rebin_string(rebin_string)
        if q_xy_max is not None:
            convert_to_q_builder.set_q_xy_max(q_xy_max)
        if q_xy_step is not None:
            convert_to_q_builder.set_q_xy_step(q_xy_step)

        convert_to_q_state = convert_to_q_builder.build()

        test_director = TestDirector()
        test_director.set_states(convert_to_q_state=convert_to_q_state, data_state=data_state)

        return test_director.construct()

    def test_that_converts_wavelength_workspace_to_q_for_1d_and_no_q_resolution(self):
        # Arrange
        workspace = self._get_workspace(is_adjustment=False)
        adj_workspace = self._get_workspace(is_adjustment=True)

        state = self._get_sample_state(q_min=1.0, q_max=2.0, q_step=0.1, q_step_type=RangeStepType.LIN)

        # Act
        output_dict = convert_workspace(
            workspace=workspace, output_summed_parts=True, state_convert_to_q=state.convert_to_q, wavelength_adj_workspace=adj_workspace
        )

        output_workspace = output_dict["output"]
        sum_of_counts = output_dict["counts_summed"]
        sum_of_norms = output_dict["norm_summed"]

        # Assert
        # We expect a q-based workspace with one histogram and 10 bins.
        self.assertEqual(output_workspace.getNumberHistograms(), 1)
        self.assertEqual(len(output_workspace.dataX(0)), 11)
        self.assertEqual(output_workspace.getAxis(0).getUnit().unitID(), "MomentumTransfer")
        self.assertFalse(output_workspace.hasDx(0))
        self.assertTrue(output_workspace.getAxis(0).isNumeric())
        self.assertTrue(output_workspace.getAxis(1).isSpectra())
        self.assertEqual(sum_of_counts.getAxis(0).getUnit().unitID(), "MomentumTransfer")
        self.assertEqual(sum_of_norms.getAxis(0).getUnit().unitID(), "MomentumTransfer")

        DeleteWorkspace(workspace)
        DeleteWorkspace(adj_workspace)
        DeleteWorkspace(output_workspace)
        DeleteWorkspace(sum_of_counts)
        DeleteWorkspace(sum_of_norms)

    def test_that_converts_wavelength_workspace_to_q_for_2d(self):
        workspace = self._get_workspace(is_adjustment=False)
        adj_workspace = self._get_workspace(is_adjustment=True)

        state = self._get_sample_state(q_xy_max=2.0, q_xy_step=0.5, dim=ReductionDimensionality.TWO_DIM)

        output_dict = convert_workspace(
            workspace=workspace, output_summed_parts=True, state_convert_to_q=state.convert_to_q, wavelength_adj_workspace=adj_workspace
        )

        output_workspace = output_dict["output"]
        sum_of_counts = output_dict["counts_summed"]
        sum_of_norms = output_dict["norm_summed"]

        # We expect a q-based workspace with 8 histograms and 8 bins
        self.assertEqual(output_workspace.getNumberHistograms(), 8)
        self.assertEqual(len(output_workspace.dataX(0)), 9)
        self.assertEqual(output_workspace.getAxis(0).getUnit().unitID(), "MomentumTransfer")
        self.assertEqual(output_workspace.getAxis(1).getUnit().unitID(), "MomentumTransfer")
        self.assertFalse(output_workspace.hasDx(0))
        self.assertTrue(output_workspace.getAxis(0).isNumeric())
        self.assertTrue(output_workspace.getAxis(1).isNumeric())
        self.assertEqual(sum_of_counts.getAxis(0).getUnit().unitID(), "MomentumTransfer")
        self.assertEqual(sum_of_norms.getAxis(0).getUnit().unitID(), "MomentumTransfer")

        DeleteWorkspace(workspace)
        DeleteWorkspace(adj_workspace)
        DeleteWorkspace(output_workspace)
        DeleteWorkspace(sum_of_counts)
        DeleteWorkspace(sum_of_norms)


if __name__ == "__main__":
    unittest.main()
