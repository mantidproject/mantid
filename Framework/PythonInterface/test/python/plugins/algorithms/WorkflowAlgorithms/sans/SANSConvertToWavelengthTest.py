# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)
import unittest

from mantid.api import FrameworkManager
from mantid.dataobjects import EventWorkspace
from sans.common.general_functions import (create_unmanaged_algorithm)
from sans.common.constants import EMPTY_NAME
from sans.common.enums import (RangeStepType)


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


class SANSSConvertToWavelengthImplementationTest(unittest.TestCase):

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def test_that_event_workspace_and_interpolating_rebin_raises(self):
        workspace = provide_workspace(is_event=True)
        convert_options = {"InputWorkspace": workspace,
                           "OutputWorkspace": EMPTY_NAME,
                           "RebinMode": "InterpolatingRebin",
                           "WavelengthLow": 1.0,
                           "WavelengthHigh": 3.0,
                           "WavelengthStep": 1.5,
                           "WavelengthStepType":  RangeStepType.to_string(RangeStepType.Lin)}
        convert_alg = create_unmanaged_algorithm("SANSConvertToWavelengthAndRebin", **convert_options)
        had_run_time_error = False
        try:
            convert_alg.execute()
        except RuntimeError:
            had_run_time_error = True
        self.assertTrue(had_run_time_error)

    def test_that_negative_wavelength_values_raise(self):
        workspace = provide_workspace(is_event=True)
        convert_options = {"InputWorkspace": workspace,
                           "OutputWorkspace": EMPTY_NAME,
                           "RebinMode": "Rebin",
                           "WavelengthLow": -1.0,
                           "WavelengthHigh": 3.0,
                           "WavelengthStep": 1.5,
                           "WavelengthStepType":  RangeStepType.to_string(RangeStepType.Log)}
        convert_alg = create_unmanaged_algorithm("SANSConvertToWavelengthAndRebin", **convert_options)
        had_run_time_error = False
        try:
            convert_alg.execute()
        except RuntimeError:
            had_run_time_error = True
        self.assertTrue(had_run_time_error)

    def test_that_lower_wavelength_larger_than_higher_wavelength_raises(self):
        workspace = provide_workspace(is_event=True)
        convert_options = {"InputWorkspace": workspace,
                           "OutputWorkspace": EMPTY_NAME,
                           "RebinMode": "Rebin",
                           "WavelengthLow":  4.0,
                           "WavelengthHigh": 3.0,
                           "WavelengthStep": 1.5,
                           "WavelengthStepType":  RangeStepType.to_string(RangeStepType.Log)}
        convert_alg = create_unmanaged_algorithm("SANSConvertToWavelengthAndRebin", **convert_options)
        had_run_time_error = False
        try:
            convert_alg.execute()
        except RuntimeError:
            had_run_time_error = True
        self.assertTrue(had_run_time_error)

    def test_that_event_workspace_with_conversion_is_still_event_workspace(self):
        workspace = provide_workspace(is_event=True)
        convert_options = {"InputWorkspace": workspace,
                           "OutputWorkspace": EMPTY_NAME,
                           "RebinMode": "Rebin",
                           "WavelengthLow": 1.0,
                           "WavelengthHigh": 10.0,
                           "WavelengthStep": 1.0,
                           "WavelengthStepType": RangeStepType.to_string(RangeStepType.Lin)}
        convert_alg = create_unmanaged_algorithm("SANSConvertToWavelengthAndRebin", **convert_options)
        convert_alg.execute()
        self.assertTrue(convert_alg.isExecuted())
        output_workspace = convert_alg.getProperty("OutputWorkspace").value
        self.assertTrue(isinstance(output_workspace, EventWorkspace))
        # Check the rebinning part
        data_x0 = output_workspace.dataX(0)
        self.assertTrue(len(data_x0) == 10)
        self.assertTrue(data_x0[0] == 1.0)
        self.assertTrue(data_x0[-1] == 10.0)
        # Check the units part
        axis0 = output_workspace.getAxis(0)
        unit = axis0.getUnit()
        self.assertTrue(unit.unitID() == "Wavelength")

    def test_that_not_setting_upper_bound_takes_it_from_original_value(self):
        workspace = provide_workspace(is_event=True)
        convert_options = {"InputWorkspace": workspace,
                           "OutputWorkspace": EMPTY_NAME,
                           "RebinMode": "Rebin",
                           "WavelengthLow": 1.0,
                           "WavelengthStep": 1.0,
                           "WavelengthStepType": RangeStepType.to_string(RangeStepType.Lin)}
        convert_alg = create_unmanaged_algorithm("SANSConvertToWavelengthAndRebin", **convert_options)
        convert_alg.execute()
        self.assertTrue(convert_alg.isExecuted())
        output_workspace = convert_alg.getProperty("OutputWorkspace").value
        self.assertTrue(isinstance(output_workspace, EventWorkspace))

        # Check the rebinning part
        data_x0 = output_workspace.dataX(0)
        self.assertTrue(data_x0[0] == 1.0)
        expected_upper_bound = 5.27471197274
        self.assertEqual(round(data_x0[-1],11), expected_upper_bound)

        # Check the units part
        axis0 = output_workspace.getAxis(0)
        unit = axis0.getUnit()
        self.assertTrue(unit.unitID() == "Wavelength")


if __name__ == '__main__':
    unittest.main()
