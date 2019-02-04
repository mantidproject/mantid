# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest
from mantid.api import FrameworkManager

from sans.common.general_functions import (create_unmanaged_algorithm)
from sans.common.constants import EMPTY_NAME
from sans.common.enums import (SANSFacility, SampleShape, ReductionDimensionality, RangeStepType, SANSInstrument)
from sans.test_helper.test_director import TestDirector
from sans.state.convert_to_q import get_convert_to_q_builder
from sans.state.data import get_data_builder
from sans.test_helper.file_information_mock import SANSFileInformationMock


class SANSConvertToQTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    @staticmethod
    def _get_workspace(x_unit="Wavelength", is_adjustment=False):
        bank_pixel_width = 1 if is_adjustment else 2
        sample_name = "CreateSampleWorkspace"
        sample_options = {"WorkspaceType": "Histogram",
                          "NumBanks": 1,
                          "BankPixelWidth": bank_pixel_width,
                          "OutputWorkspace": "test",
                          "Function": "Flat background",
                          "XUnit": x_unit,
                          "XMin": 1.,
                          "XMax": 10.,
                          "BinWidth": 2.}
        sample_alg = create_unmanaged_algorithm(sample_name, **sample_options)
        sample_alg.execute()
        workspace = sample_alg.getProperty("OutputWorkspace").value
        return workspace

    @staticmethod
    def _get_sample_state(q_min=1., q_max=2., q_step=0.1, q_step_type=RangeStepType.Lin,
                          q_xy_max=None, q_xy_step=None, q_xy_step_type=None,
                          use_gravity=False, dim=ReductionDimensionality.OneDim):
        # Arrange
        facility = SANSFacility.ISIS
        file_information = SANSFileInformationMock(instrument=SANSInstrument.LOQ, run_number=74044)
        data_builder = get_data_builder(facility, file_information)
        data_builder.set_sample_scatter("LOQ74044")
        data_state = data_builder.build()

        convert_to_q_builder = get_convert_to_q_builder(data_state)
        convert_to_q_builder.set_reduction_dimensionality(dim)
        convert_to_q_builder.set_use_gravity(use_gravity)
        convert_to_q_builder.set_radius_cutoff(0.002)
        convert_to_q_builder.set_wavelength_cutoff(2.)
        convert_to_q_builder.set_q_min(q_min)
        convert_to_q_builder.set_q_max(q_max)
        prefix = 1. if q_step_type is RangeStepType.Lin else -1.
        q_step *= prefix
        rebin_string = str(q_min) + "," + str(q_step) + "," + str(q_max)
        convert_to_q_builder.set_q_1d_rebin_string(rebin_string)
        if q_xy_max is not None:
            convert_to_q_builder.set_q_xy_max(q_xy_max)
        if q_xy_step is not None:
            convert_to_q_builder.set_q_xy_step(q_xy_step)
        if q_xy_step_type is not None:
            convert_to_q_builder.set_q_xy_step_type(q_xy_step_type)

        convert_to_q_state = convert_to_q_builder.build()

        test_director = TestDirector()
        test_director.set_states(convert_to_q_state=convert_to_q_state, data_state=data_state)

        return test_director.construct().property_manager

    @staticmethod
    def _do_run_convert_to_q(state, data_workspace, wavelength_adjustment_workspace=None,
                             pixel_adjustment_workspace=None, wavelength_and_pixel_adjustment_workspace=None):
        convert_name = "SANSConvertToQ"
        convert_options = {"InputWorkspace": data_workspace,
                           "OutputWorkspace": EMPTY_NAME,
                           "SANSState": state,
                           "OutputParts": True}
        if wavelength_adjustment_workspace:
            convert_options.update({"InputWorkspaceWavelengthAdjustment": wavelength_adjustment_workspace})
        if pixel_adjustment_workspace:
            convert_options.update({"InputWorkspacePixelAdjustment": pixel_adjustment_workspace})
        if wavelength_and_pixel_adjustment_workspace:
            convert_options.update({"InputWorkspaceWavelengthAndPixelAdjustment":
                                        wavelength_and_pixel_adjustment_workspace})
        convert_alg = create_unmanaged_algorithm(convert_name, **convert_options)
        convert_alg.execute()
        data_workspace = convert_alg.getProperty("OutputWorkspace").value
        sum_of_counts = convert_alg.getProperty("SumOfCounts").value
        sum_of_norms = convert_alg.getProperty("SumOfNormFactors").value
        return data_workspace, sum_of_counts, sum_of_norms

    def test_that_only_accepts_wavelength_based_workspaces(self):
        # Arrange
        workspace = SANSConvertToQTest._get_workspace("TOF")
        state = self._get_sample_state(q_min=1., q_max=2., q_step=0.1, q_step_type=RangeStepType.Lin)
        # Act + Assert
        args = []
        kwargs = {"state": state, "data_workspace": workspace}
        self.assertRaises(ValueError, SANSConvertToQTest._do_run_convert_to_q, *args, **kwargs)

    def test_that_converts_wavelength_workspace_to_q_for_1d_and_no_q_resolution(self):
        # Arrange
        workspace = SANSConvertToQTest._get_workspace("Wavelength")
        workspace2 = SANSConvertToQTest._get_workspace("Wavelength", is_adjustment=True)

        state = self._get_sample_state(q_min=1., q_max=2., q_step=0.1, q_step_type=RangeStepType.Lin)

        # Act
        output_workspace, sum_of_counts, sum_of_norms = SANSConvertToQTest._do_run_convert_to_q(state=state,
                                         data_workspace=workspace, wavelength_adjustment_workspace=workspace2)

        # Assert
        # We expect a q-based workspace with one histogram and 10 bins.
        self.assertTrue(output_workspace.getNumberHistograms() == 1)
        self.assertTrue(len(output_workspace.dataX(0)) == 11)
        self.assertTrue(output_workspace.getAxis(0).getUnit().unitID() == "MomentumTransfer")
        self.assertFalse(output_workspace.hasDx(0))
        self.assertTrue(output_workspace.getAxis(0).isNumeric())
        self.assertTrue(output_workspace.getAxis(1).isSpectra())
        self.assertTrue(sum_of_counts.getAxis(0).getUnit().unitID() == "MomentumTransfer")
        self.assertTrue(sum_of_norms.getAxis(0).getUnit().unitID() == "MomentumTransfer")

    def test_that_converts_wavelength_workspace_to_q_for_2d(self):
        # Arrange
        workspace = SANSConvertToQTest._get_workspace("Wavelength")
        workspace2 = SANSConvertToQTest._get_workspace("Wavelength", is_adjustment=True)

        state = self._get_sample_state(q_xy_max=2., q_xy_step=0.5, q_xy_step_type=RangeStepType.Lin,
                                       dim=ReductionDimensionality.TwoDim)

        # Act
        output_workspace, sum_of_counts, sum_of_norms = SANSConvertToQTest._do_run_convert_to_q(state=state,
                                         data_workspace=workspace, wavelength_adjustment_workspace=workspace2)

        # Assert
        # We expect a q-based workspace with 8 histograms and 8 bins
        self.assertTrue(output_workspace.getNumberHistograms() == 8)
        self.assertTrue(len(output_workspace.dataX(0)) == 9)
        self.assertTrue(output_workspace.getAxis(0).getUnit().unitID() == "MomentumTransfer")
        self.assertTrue(output_workspace.getAxis(1).getUnit().unitID() == "MomentumTransfer")
        self.assertFalse(output_workspace.hasDx(0))
        self.assertTrue(output_workspace.getAxis(0).isNumeric())
        self.assertTrue(output_workspace.getAxis(1).isNumeric())
        self.assertTrue(sum_of_counts.getAxis(0).getUnit().unitID() == "MomentumTransfer")
        self.assertTrue(sum_of_norms.getAxis(0).getUnit().unitID() == "MomentumTransfer")


if __name__ == '__main__':
    unittest.main()
