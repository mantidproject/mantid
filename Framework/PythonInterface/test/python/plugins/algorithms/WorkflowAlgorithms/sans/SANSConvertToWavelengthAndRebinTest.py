# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import json
import unittest

import numpy

from mantid.api import FrameworkManager, WorkspaceGroup
from mantid.dataobjects import EventWorkspace
from sans_core.common.general_functions import create_unmanaged_algorithm
from sans_core.common.constants import EMPTY_NAME
from sans_core.common.enums import RangeStepType


def provide_workspace(is_event=True):
    sample_name = "CreateSampleWorkspace"
    sample_options = {"OutputWorkspace": "dummy", "NumBanks": 1, "BankPixelWidth": 2}
    if is_event:
        sample_options.update({"WorkspaceType": "Event"})
    else:
        sample_options.update({"WorkspaceType": "Histogram"})

    sample_alg = create_unmanaged_algorithm(sample_name, **sample_options)
    sample_alg.execute()
    return sample_alg.getProperty("OutputWorkspace").value


class SANSSConvertToWavelengthImplementationTest(unittest.TestCase):
    WAV_PAIRS = "WavelengthPairs"

    @classmethod
    def setUpClass(cls):
        FrameworkManager.Instance()

    def test_invalid_json_raises(self):
        workspace = provide_workspace(is_event=True)
        convert_options = {
            "InputWorkspace": workspace,
            "OutputWorkspace": EMPTY_NAME,
            "RebinMode": "InterpolatingRebin",
            self.WAV_PAIRS: "invalidJSON",
            "WavelengthStep": 1.5,
            "WavelengthStepType": RangeStepType.LIN.value,
        }
        convert_alg = create_unmanaged_algorithm("SANSConvertToWavelengthAndRebin", **convert_options)
        with self.assertRaisesRegex(RuntimeError, "WavelengthPairs"):
            convert_alg.execute()

    def test_single_nested_json_raises(self):
        convert_options = {
            "InputWorkspace": provide_workspace(),
            "OutputWorkspace": EMPTY_NAME,
            "RebinMode": "InterpolatingRebin",
            self.WAV_PAIRS: json.dumps([1.0, 2.0]),
            "WavelengthStep": 1.5,
            "WavelengthStepType": RangeStepType.LIN.value,
        }
        convert_alg = create_unmanaged_algorithm("SANSConvertToWavelengthAndRebin", **convert_options)
        with self.assertRaisesRegex(RuntimeError, "WavelengthPairs"):
            convert_alg.execute()

    def test_that_event_workspace_and_interpolating_rebin_raises(self):
        workspace = provide_workspace(is_event=True)
        convert_options = {
            "InputWorkspace": workspace,
            "OutputWorkspace": EMPTY_NAME,
            "RebinMode": "InterpolatingRebin",
            self.WAV_PAIRS: json.dumps([[1.0, 3.0]]),
            "WavelengthStep": 1.5,
            "WavelengthStepType": RangeStepType.LIN.value,
        }
        convert_alg = create_unmanaged_algorithm("SANSConvertToWavelengthAndRebin", **convert_options)
        with self.assertRaisesRegex(RuntimeError, "RebinMode"):
            convert_alg.execute()

    def test_that_negative_wavelength_values_raise(self):
        workspace = provide_workspace(is_event=True)
        convert_options = {
            "InputWorkspace": workspace,
            "OutputWorkspace": EMPTY_NAME,
            "RebinMode": "Rebin",
            self.WAV_PAIRS: json.dumps([[-1.0, 3.0]]),
            "WavelengthStep": 1.5,
            "WavelengthStepType": RangeStepType.LOG.value,
        }
        convert_alg = create_unmanaged_algorithm("SANSConvertToWavelengthAndRebin", **convert_options)
        with self.assertRaisesRegex(RuntimeError, self.WAV_PAIRS):
            convert_alg.execute()

    def test_that_lower_wavelength_larger_than_higher_wavelength_raises(self):
        workspace = provide_workspace(is_event=True)
        convert_options = {
            "InputWorkspace": workspace,
            "OutputWorkspace": EMPTY_NAME,
            "RebinMode": "Rebin",
            self.WAV_PAIRS: json.dumps([[4.0, 3.0]]),
            "WavelengthStep": 1.5,
            "WavelengthStepType": RangeStepType.LOG.value,
        }
        convert_alg = create_unmanaged_algorithm("SANSConvertToWavelengthAndRebin", **convert_options)
        with self.assertRaisesRegex(RuntimeError, self.WAV_PAIRS):
            convert_alg.execute()

    def test_that_event_workspace_with_conversion_is_still_event_workspace(self):
        workspace = provide_workspace(is_event=True)
        convert_options = {
            "InputWorkspace": workspace,
            "OutputWorkspace": EMPTY_NAME,
            "RebinMode": "Rebin",
            self.WAV_PAIRS: json.dumps([[1.0, 10.0]]),
            "WavelengthStep": 1.0,
            "WavelengthStepType": RangeStepType.LIN.value,
        }
        convert_alg = create_unmanaged_algorithm("SANSConvertToWavelengthAndRebin", **convert_options)
        convert_alg.execute()
        self.assertTrue(convert_alg.isExecuted())
        output_workspaces = convert_alg.getProperty("OutputWorkspace").value
        self.assertTrue(isinstance(output_workspaces, WorkspaceGroup))
        self.assertEqual(1, len(output_workspaces))
        output_workspace = output_workspaces.getItem(0)
        self.assertTrue(isinstance(output_workspace, EventWorkspace))
        # Check the rebinning part
        data_x0 = output_workspace.dataX(0)
        self.assertEqual(len(data_x0), 10)
        self.assertEqual(data_x0[0], 1.0)
        self.assertEqual(data_x0[-1], 10.0)
        # Check the units part
        axis0 = output_workspace.getAxis(0)
        unit = axis0.getUnit()
        self.assertEqual(unit.unitID(), "Wavelength")

    def test_that_not_setting_upper_bound_takes_it_from_original_value(self):
        workspace = provide_workspace(is_event=True)
        convert_options = {
            "InputWorkspace": workspace,
            "OutputWorkspace": EMPTY_NAME,
            "RebinMode": "Rebin",
            self.WAV_PAIRS: json.dumps([[1.0, None]]),
            "WavelengthStep": 1.0,
            "WavelengthStepType": RangeStepType.LIN.value,
        }
        convert_alg = create_unmanaged_algorithm("SANSConvertToWavelengthAndRebin", **convert_options)
        convert_alg.execute()
        self.assertTrue(convert_alg.isExecuted())
        output_workspaces = convert_alg.getProperty("OutputWorkspace").value
        self.assertTrue(isinstance(output_workspaces, WorkspaceGroup))
        output_workspace = output_workspaces.getItem(0)
        self.assertTrue(isinstance(output_workspace, EventWorkspace))

        # Check the rebinning part
        data_x0 = output_workspace.dataX(0)
        self.assertEqual(data_x0[0], 1.0)
        expected_upper_bound = 5.27471197274
        self.assertEqual(round(data_x0[-1], 11), expected_upper_bound)

        # Check the units part
        axis0 = output_workspace.getAxis(0)
        unit = axis0.getUnit()
        self.assertEqual(unit.unitID(), "Wavelength")

    def test_multiple_pairs(self):
        workspace = provide_workspace(is_event=True)
        expected_x_data = [[1.0, 2.0], [2.0, 3.0], [1.0, 3.0]]
        convert_options = {
            "InputWorkspace": workspace,
            "OutputWorkspace": EMPTY_NAME,
            "RebinMode": "Rebin",
            self.WAV_PAIRS: json.dumps(expected_x_data),
            "WavelengthStep": 1.0,
            "WavelengthStepType": RangeStepType.LIN.value,
        }
        convert_alg = create_unmanaged_algorithm("SANSConvertToWavelengthAndRebin", **convert_options)
        convert_alg.execute()
        self.assertTrue(convert_alg.isExecuted())
        output_workspaces = convert_alg.getProperty("OutputWorkspace").value

        self.assertTrue(isinstance(output_workspaces, WorkspaceGroup))
        self.assertEqual(len(expected_x_data), len(output_workspaces))

        for i, (expected, ws) in enumerate(zip(expected_x_data, output_workspaces)):
            x_expected_bins = numpy.arange(expected[0], expected[1] + 1, step=1)  # Emulate x bin data with interval 1
            self.assertTrue(numpy.array_equal(x_expected_bins, ws.dataX(0)), f"Ws index {i} was not equal")


if __name__ == "__main__":
    unittest.main()
