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
    REDUCED_WS_PREFIX = "IvsQ_binned_"

    _TIMESTAMP_METADATA = "#   timestamp: 2"
    _REDUCTION_METADATA = "# reduction:"

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
        self._run_reduction(input_run_list, reduction_args)
        self._call_save_algorithm(reduced_ws_name)

    def run_stitched_reduction_and_save_file(self, input_runs_and_theta, reduction_args=None, instrument_name=""):
        ws_names_list = self._run_stitched_reduction(input_runs_and_theta, reduction_args, instrument_name)
        self._call_save_algorithm(ws_names_list, include_extra_cols=True)

    def check_output_against_ref_file(self, ref_filepath, ref_file_excludes_reduction_call=False):
        saved_file_lines = self._read_lines_from_file(self._output_filename)
        ref_file_lines = self._read_lines_from_file(ref_filepath)

        saved_line_num = 0
        for ref_file_line in ref_file_lines:
            saved_line = saved_file_lines[saved_line_num]
            if self._should_check_line(ref_file_line, saved_line):
                # Depending on the precise timings of each individual reduction, we can get multiple reduction
                # timestamps in a multi-dataset ORSO file that aren't in our reference files, so we skip these
                if saved_line.startswith(self._TIMESTAMP_METADATA):
                    saved_line_num += 1
                    saved_line = saved_file_lines[saved_line_num]
                elif (
                    ref_file_excludes_reduction_call
                    and saved_line.startswith(self._REDUCTION_METADATA)
                    and saved_file_lines[saved_line_num + 1].startswith(self._TIMESTAMP_METADATA)
                ):
                    # When the reference file has no reduction call information then the extra reduction timestamps will
                    # also create an extra reduction header line that isn't in the reference file, so we skip two lines
                    saved_line_num += 2
                    saved_line = saved_file_lines[saved_line_num]
                self._check_line(saved_line, ref_file_line)
            saved_line_num += 1

    def _run_reduction(self, input_runs, additional_args, theta=0.5):
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

    def _run_stitched_reduction(self, input_runs_and_theta, additional_args, instrument_name=""):
        reduced_workspaces = []
        for run, theta in input_runs_and_theta:
            self._run_reduction(instrument_name + run if instrument_name else run, additional_args, theta)
            reduced_workspaces.append(f"{self.REDUCED_WS_PREFIX}{run}")

        stitched_ws_name = "stitched_ws"
        args = {
            "InputWorkspaces": reduced_workspaces,
            "OutputWorkspace": stitched_ws_name,
            "Params": 0.002,
        }
        alg = create_algorithm("Stitch1DMany", **args)
        alg.setRethrows(True)
        assertRaisesNothing(self, alg.execute)

        reduced_workspaces.append(stitched_ws_name)
        return reduced_workspaces

    def _call_save_algorithm(self, reduced_ws, include_extra_cols=False):
        SaveISISReflectometryORSO(
            WorkspaceList=reduced_ws, WriteResolution=True, IncludeAdditionalColumns=include_extra_cols, Filename=self._output_filename
        )

    @staticmethod
    def _read_lines_from_file(filepath):
        with open(filepath, "r") as file:
            return file.readlines()

    def _should_check_line(self, ref_line, saved_line):
        # Some lines in the file will change each time the test runs and are checked in the unit tests instead.
        # Additionally, the first line in the file may change when the orsopy version changes and is not necessary to
        # check.
        lines_to_skip = ["#   software: {name: Mantid", self._TIMESTAMP_METADATA, "# # ORSO reflectivity data file"]
        for skip_line in lines_to_skip:
            if ref_line.startswith(skip_line) and saved_line.startswith(skip_line):
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


class SaveISISReflectometryORSOReducedSingleDatasetFileTest(SaveISISReflectometryORSOTest):
    _REF_FILE = FileFinder.getFullPath("ISISReducedWorkspaceORSOFile.ort")

    def runTest(self):
        # One of the input runs should be an already loaded workspace as this appears differently in the history
        input_ws_name = "Test_ws"
        LoadNexus("13462", OutputWorkspace=input_ws_name)
        input_runs = f"13460, {input_ws_name}, inter13463.nxs"
        args = {"FirstTransmissionRunList": "13464", "SecondTransmissionRunList": "13464, 13469"}
        self.run_reduction_and_save_file(input_runs, f"{self.REDUCED_WS_PREFIX}13460+{input_ws_name}+13463", args)
        self.check_output_against_ref_file(self._REF_FILE)


class SaveISISReflectometryORSOPeriodDataMultiDatasetFileTest(SaveISISReflectometryORSOTest):
    _REF_FILE = FileFinder.getFullPath("ISISReducedPeriodDataWorkspaceORSOFile.ort")

    def runTest(self):
        self.run_stitched_reduction_and_save_file([("31832", 0.25), ("31833", 0.5)], instrument_name="POLREF")
        self.check_output_against_ref_file(self._REF_FILE, ref_file_excludes_reduction_call=True)


class SaveISISReflectometryORSOStitchedMultiDatasetFileTest(SaveISISReflectometryORSOTest):
    _REF_FILE = FileFinder.getFullPath("ISISStitchedWorkspaceORSOFile.ort")

    def runTest(self):
        args = {"FirstTransmissionRunList": "13463", "SecondTransmissionRunList": "13464, 13469"}
        self.run_stitched_reduction_and_save_file([("13460", 0.5), ("13462", 1.0)], reduction_args=args)
        self.check_output_against_ref_file(self._REF_FILE)
