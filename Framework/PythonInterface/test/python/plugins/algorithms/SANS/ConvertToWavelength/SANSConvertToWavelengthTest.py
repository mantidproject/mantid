import unittest
import mantid

from mantid.dataobjects import EventWorkspace
from SANS2.Common.SANSFunctions import (create_unmanaged_algorithm)
from SANS2.Common.SANSConstants import SANSConstants


def provide_workspace(is_event=True):
    sample_name = "CreateSampleWorkspace"
    sample_options = {"OutputWorkspace": "dummy",
                      "NumBanks": 1,
                      "BankPixelWidth": 2}
    if is_event:
        sample_options.update({"WorkspaceType": "Event"})
    else:
        sample_options.update({"WorkspaceType": "Histogram"})

    sample_alg = create_unmanaged_algorithm(sample_name, **sample_options)
    sample_alg.execute()
    return sample_alg.getProperty("OutputWorkspace").value


class SANSSConvertToWavelengthTest(unittest.TestCase):
    # def test_that_event_workspace_and_interpolating_rebin_raises(self):
    #     workspace = provide_workspace(is_event=True)
    #     convert_options = {SANSConstants.input_workspace: workspace,
    #                        SANSConstants.output_workspace: SANSConstants.dummy,
    #                        "RebinMode": "InterpolatingRebin",
    #                        "WavelengthLow": 1.0,
    #                        "WavelengthHigh": 3.0,
    #                        "WavelengthStep": 1.5,
    #                        "WavelengthStepType": "LOG"}
    #     convert_alg = create_unmanaged_algorithm("SANSConvertToWavelength", **convert_options)
    #     had_run_time_error = False
    #     try:
    #         convert_alg.execute()
    #     except RuntimeError:
    #         had_run_time_error = True
    #     self.assertTrue(had_run_time_error)
    #
    # def test_that_negative_wavelength_values_raise(self):
    #     workspace = provide_workspace(is_event=True)
    #     convert_options = {SANSConstants.input_workspace: workspace,
    #                        SANSConstants.output_workspace: SANSConstants.dummy,
    #                        "RebinMode": "Rebin",
    #                        "WavelengthLow": -1.0,
    #                        "WavelengthHigh": 3.0,
    #                        "WavelengthStep": 1.5,
    #                        "WavelengthStepType": "LOG"}
    #     convert_alg = create_unmanaged_algorithm("SANSConvertToWavelength", **convert_options)
    #     had_run_time_error = False
    #     try:
    #         convert_alg.execute()
    #     except RuntimeError:
    #         had_run_time_error = True
    #     self.assertTrue(had_run_time_error)
    #
    # def test_that_lower_wavelength_larger_than_higher_wavelength_raises(self):
    #     workspace = provide_workspace(is_event=True)
    #     convert_options = {SANSConstants.input_workspace: workspace,
    #                        SANSConstants.output_workspace: SANSConstants.dummy,
    #                        "RebinMode": "Rebin",
    #                        "WavelengthLow":  4.0,
    #                        "WavelengthHigh": 3.0,
    #                        "WavelengthStep": 1.5,
    #                        "WavelengthStepType": "LOG"}
    #     convert_alg = create_unmanaged_algorithm("SANSConvertToWavelength", **convert_options)
    #     had_run_time_error = False
    #     try:
    #         convert_alg.execute()
    #     except RuntimeError:
    #         had_run_time_error = True
    #     self.assertTrue(had_run_time_error)
    #
    # def test_that_event_workspace_with_conversion_is_still_event_workspace(self):
    #     workspace = provide_workspace(is_event=True)
    #     convert_options = {SANSConstants.input_workspace: workspace,
    #                        SANSConstants.output_workspace: SANSConstants.dummy,
    #                        "RebinMode": "Rebin",
    #                        "WavelengthLow": 1.0,
    #                        "WavelengthHigh": 10.0,
    #                        "WavelengthStep": 1.0,
    #                        "WavelengthStepType": "LIN"}
    #     convert_alg = create_unmanaged_algorithm("SANSConvertToWavelength", **convert_options)
    #     convert_alg.execute()
    #     self.assertTrue(convert_alg.isExecuted())
    #     output_workspace = convert_alg.getProperty(SANSConstants.output_workspace).value
    #     self.assertTrue(isinstance(output_workspace, EventWorkspace))
    #     # Check the rebinning part
    #     dataX0 = output_workspace.dataX(0)
    #     self.assertTrue(len(dataX0) == 10)
    #     self.assertTrue(dataX0[0] == 1.0)
    #     self.assertTrue(dataX0[-1] == 10.0)
    #     # Check the units part
    #     axis0 = output_workspace.getAxis(0)
    #     unit = axis0.getUnit()
    #     self.assertTrue(unit.unitID() == "Wavelength")

    def test_that_not_setting_upper_bound_takes_it_from_original_value(self):
        workspace = provide_workspace(is_event=True)
        convert_options = {SANSConstants.input_workspace: workspace,
                           SANSConstants.output_workspace: SANSConstants.dummy,
                           "RebinMode": "Rebin",
                           "WavelengthLow": 1.0,
                           "WavelengthStep": 1.0,
                           "WavelengthStepType": "LIN"}
        convert_alg = create_unmanaged_algorithm("SANSConvertToWavelength", **convert_options)
        convert_alg.execute()
        self.assertTrue(convert_alg.isExecuted())
        output_workspace = convert_alg.getProperty(SANSConstants.output_workspace).value
        self.assertTrue(isinstance(output_workspace, EventWorkspace))

        # Check the rebinning part
        dataX0 = output_workspace.dataX(0)
        self.assertTrue(dataX0[0] == 1.0)
        expected_upper_bound = 5.27471197274
        self.assertTrue(dataX0[-1] == expected_upper_bound)

        # Check the units part
        axis0 = output_workspace.getAxis(0)
        unit = axis0.getUnit()
        self.assertTrue(unit.unitID() == "Wavelength")


if __name__ == '__main__':
    unittest.main()
