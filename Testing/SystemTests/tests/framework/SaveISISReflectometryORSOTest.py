# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2024 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import systemtesting
import os
import tempfile
import numpy as np
from mantid import config, FileFinder
from mantid.api import AnalysisDataService
from mantid.simpleapi import SaveISISReflectometryORSO, LoadNexus

from testhelpers import assertRaisesNothing, create_algorithm


# These tests check the ISIS ORSO file output by running the ISIS Reflectometry reduction rather than using a reduced
# input Nexus file. This is so that we can pick up any changes to the reduction that affect the file output.
class SaveISISReflectometryORSOTest(systemtesting.MantidSystemTest):
    def setUp(self):
        self._oldFacility = config["default.facility"]
        if self._oldFacility.strip() == "":
            self._oldFacility = "TEST_LIVE"
        self._oldInstrument = config["default.instrument"]
        config["default.facility"] = "ISIS"
        config["default.instrument"] = "INTER"

        self._filename = "ORSO_save_test.ort"
        self._temp_dir = tempfile.TemporaryDirectory()
        self._output_filename = os.path.join(self._temp_dir.name, self._filename)

    def tearDown(self):
        AnalysisDataService.clear()
        config["default.facility"] = self._oldFacility
        config["default.instrument"] = self._oldInstrument
        self._temp_dir.cleanup()

    def runTest(self):
        pass

    def run_reduction_and_save_file(self, input_run_list, reduced_ws_name, reduction_args=None):
        reduced_ws = self._get_ws_from_reduction(input_run_list, reduced_ws_name, reduction_args)
        self._call_save_algorithm(reduced_ws)

    def run_stitched_reduction_and_save_file(self, input_runs_and_theta, reduction_args=None):
        reduced_ws = self._get_ws_from_stitched_reduction(input_runs_and_theta, reduction_args)
        self._call_save_algorithm(reduced_ws)

    def check_output_against_ref_file(self, ref_filepath):
        saved_file_lines = self._read_lines_from_file(self._output_filename)
        ref_file_lines = self._read_lines_from_file(ref_filepath)

        for i, ref_file_line in enumerate(ref_file_lines):
            if self._should_check_line(ref_file_line):
                self._check_line(saved_file_lines[i], ref_file_line)

    def _get_ws_from_reduction(self, input_runs, reduced_ws_name, additional_args, theta=0.5):
        args = {
            "InputRunList": input_runs,
            "ProcessingInstructions": "4",
            "ThetaIn": theta,
            "WavelengthMin": 2,
            "WavelengthMax": 5,
            "I0MonitorIndex": 1,
            "MomentumTransferStep": 0.02,
        }
        if additional_args:
            args.update(additional_args)
        alg = create_algorithm("ReflectometryISISLoadAndProcess", **args)
        alg.setRethrows(True)
        assertRaisesNothing(self, alg.execute)
        return AnalysisDataService.retrieve(reduced_ws_name)

    def _get_ws_from_stitched_reduction(self, input_runs_and_theta, additional_args):
        reduced_workspaces = []
        for run, theta in input_runs_and_theta:
            reduced_ws = self._get_ws_from_reduction(run, f"IvsQ_binned_{run}", additional_args, theta)
            reduced_workspaces.append(reduced_ws)

        stitched_ws_name = "stitched_ws"
        args = {
            "InputWorkspaces": reduced_workspaces,
            "OutputWorkspace": stitched_ws_name,
            "Params": 0.002,
        }
        alg = create_algorithm("Stitch1DMany", **args)
        alg.setRethrows(True)
        assertRaisesNothing(self, alg.execute)
        return AnalysisDataService.retrieve(stitched_ws_name)

    def _call_save_algorithm(self, reduced_ws):
        SaveISISReflectometryORSO(InputWorkspace=reduced_ws, WriteResolution=True, Filename=self._output_filename)

    @staticmethod
    def _read_lines_from_file(filepath):
        with open(filepath, "r") as file:
            return file.readlines()

    @staticmethod
    def _should_check_line(line):
        # Some lines in the file will change each time the test runs and are checked in the unit tests instead.
        # Additionally, the first line in the file may change when the orsopy version changes and is not necessary to
        # check.
        lines_to_skip = ["#   software: {name: Mantid", "#   timestamp: 2", "# # ORSO reflectivity data file"]
        for skip_line in lines_to_skip:
            if line.startswith(skip_line):
                return False
        return True

    def _check_line(self, line, ref_line):
        if ref_line.startswith("#"):
            # Check the header metadata
            self.assertEqual(ref_line, line)
        else:
            # Check the data values
            ref_values = np.fromstring(ref_line, dtype=float, sep=" ")
            actual_values = np.fromstring(line, dtype=float, sep=" ")
            self.assertTrue(np.allclose(actual_values, ref_values, atol=1e-10, equal_nan=True))


class SaveISISReflectometryORSOReducedWorkspaceTest(SaveISISReflectometryORSOTest):
    _REF_FILE = FileFinder.getFullPath("ISISReducedWorkspaceORSOFile.ort")

    def runTest(self):
        # One of the input runs should be an already loaded workspace as this appears differently in the history
        input_ws_name = "Test_ws"
        LoadNexus("13462", OutputWorkspace=input_ws_name)
        input_runs = f"13460, {input_ws_name}, inter13463.nxs"
        args = {"FirstTransmissionRunList": "13464", "SecondTransmissionRunList": "13464, 13469"}
        self.run_reduction_and_save_file(input_runs, f"IvsQ_binned_13460+{input_ws_name}+13463", args)
        self.check_output_against_ref_file(self._REF_FILE)


class SaveISISReflectometryORSOReducedPeriodDataWorkspaceTest(SaveISISReflectometryORSOTest):
    _REF_FILE = FileFinder.getFullPath("ISISReducedPeriodDataWorkspaceORSOFile.ort")

    def runTest(self):
        self.run_reduction_and_save_file("POLREF14966", "IvsQ_binned_14966_1")
        self.check_output_against_ref_file(self._REF_FILE)


class SaveISISReflectometryORSOStitchedWorkspaceTest(SaveISISReflectometryORSOTest):
    _REF_FILE = FileFinder.getFullPath("ISISStitchedWorkspaceORSOFile.ort")

    def runTest(self):
        args = {"FirstTransmissionRunList": "13463", "SecondTransmissionRunList": "13464, 13469"}
        self.run_stitched_reduction_and_save_file([("13460", 0.5), ("13462", 1.0)], reduction_args=args)
        self.check_output_against_ref_file(self._REF_FILE)
