# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import os
import unittest
import tempfile
import numpy as np
from datetime import datetime, timezone

from mantid import config
from mantid.simpleapi import (
    CreateSampleWorkspace,
    SaveISISReflectometryORSO,
    ConvertToPointData,
    NRCalculateSlitResolution,
    LoadNexus,
    DeleteLog,
)
from mantid.api import AnalysisDataService
from mantid.kernel import version
from mantid.utils.reflectometry.orso_helper import MantidORSODataset
from testhelpers import assertRaisesNothing, create_algorithm


class SaveISISReflectometryORSOTest(unittest.TestCase):
    """Tests for the ISIS Reflectometry implementation of the ORSO file format.
    These tests focus on the file contents that are specific to ISIS.
    See mantid/utils/reflectometry/orso_helper_test.py for tests covering the shared aspects of the ORSO file output.
    """

    _LOG_RB_NUMBER = "rb_proposal"
    _LOG_EXP_IDENTIFIER = "experiment_identifier"
    _INVALID_HEADER_COMMENT = "Mantid@ISIS output may not be fully ORSO compliant"
    _SAVE_ALG = "SaveISISReflectometryORSO"
    _REDUCTION_ALG = "ReflectometryReductionOneAuto"
    _REDUCTION_WORKFLOW_ALG = "ReflectometryISISLoadAndProcess"
    _Q_UNIT = "MomentumTransfer"
    _FIRST_TRANS_COMMENT = "First transmission run"
    _SECOND_TRANS_COMMENT = "Second transmission run"

    def setUp(self):
        self._oldFacility = config["default.facility"]
        if self._oldFacility.strip() == "":
            self._oldFacility = "TEST_LIVE"
        self._oldInstrument = config["default.instrument"]
        config["default.facility"] = "ISIS"
        config["default.instrument"] = "INTER"

        self._rb_number = str(123456)
        self._filename = "ORSO_save_test.ort"
        self._temp_dir = tempfile.TemporaryDirectory()
        self._output_filename = os.path.join(self._temp_dir.name, self._filename)
        # The optional ISIS header entries that are not taken from the ws history
        # and that will usually be saved out from the test workspace produced by _create_sample_workspace
        self._header_entries_for_sample_ws = [
            f"proposalID: '{self._rb_number}'",
            f"doi: 10.5286/ISIS.E.RB{self._rb_number}",
            "facility: ISIS",
            f"creator:\n#     name: {self._SAVE_ALG}\n#     affiliation: {MantidORSODataset.SOFTWARE_NAME}\n#",
        ]
        self._runs_and_angles = {
            "INTER00013460.nxs": "0.5",
            "INTER00013461.nxs": "0.5",
            "INTER00013462.nxs": "0.5",
        }

    def tearDown(self):
        """
        Delete any workspaces created and remove the temp directory
        """
        AnalysisDataService.clear()
        config["default.facility"] = self._oldFacility
        config["default.instrument"] = self._oldInstrument
        self._temp_dir.cleanup()

    def test_create_file_from_workspace_with_reduction_history(self):
        # Check that relevant information is extracted from the history produced by the ISIS Reflectometry reduction
        resolution = 0.02
        reduced_ws = self._get_ws_from_reduction("INTER13460, 13461, 13462.nxs", resolution, "IvsQ_13460+13461+13462")
        SaveISISReflectometryORSO(InputWorkspace=reduced_ws, Filename=self._output_filename)

        expected_data_files = ["INTER00013460.nxs", "INTER00013461.nxs", "INTER00013462.nxs"]
        header_values_to_check = self._get_header_values_expected_from_reduced_ws(reduced_ws, expected_data_files)
        excluded_values = [f"#     additional_files:\n"]

        self._check_file_contents(header_values_to_check, reduced_ws, resolution, excluded_values)

    def test_file_metadata_matches_reduced_workspace_instrument_when_save_after_default_instrument_changed(self):
        self.assertTrue(config["default.instrument"] == "INTER")
        resolution = 0.02
        reduced_ws = self._get_ws_from_reduction("13460", resolution, "IvsQ_binned_13460")

        config["default.instrument"] = "OFFSPEC"
        SaveISISReflectometryORSO(InputWorkspace=reduced_ws, Filename=self._output_filename)

        expected_data_files = ["INTER00013460.nxs"]
        header_values_to_check = self._get_header_values_expected_from_reduced_ws(reduced_ws, expected_data_files)

        self._check_file_contents(header_values_to_check, reduced_ws, resolution)

    def test_create_file_from_workspace_with_reduction_history_and_transmission_runs(self):
        resolution = 0.02
        reduced_ws = self._get_ws_from_reduction("13460", resolution, "IvsQ_binned_13460", "13463", "13464, 13462")
        SaveISISReflectometryORSO(InputWorkspace=reduced_ws, Filename=self._output_filename)

        expected_data_files = ["INTER00013460.nxs"]
        expected_additional_file_entries = {
            "INTER00013463.nxs": self._FIRST_TRANS_COMMENT,
            "INTER00013464.nxs": self._SECOND_TRANS_COMMENT,
            "INTER00013462.nxs": self._SECOND_TRANS_COMMENT,
        }
        header_values_to_check = self._get_header_values_expected_from_reduced_ws(reduced_ws, expected_data_files)
        header_values_to_check.append(self._expected_additional_file_header_values(expected_additional_file_entries))

        self._check_file_contents(header_values_to_check, reduced_ws, resolution)

    def test_create_file_for_period_data_workspace_with_reduction_history(self):
        # The history produced for the ISIS Reflectometry reduction is slightly different for period data, so
        # check that we can extract relevant information from this
        resolution = 0.02
        reduced_ws = self._get_ws_from_reduction("POLREF14966", resolution, "IvsQ_binned_14966_1")

        SaveISISReflectometryORSO(InputWorkspace=reduced_ws, Filename=self._output_filename)

        header_values_to_check = self._get_header_values_expected_from_reduced_ws(reduced_ws)

        self._check_file_contents(header_values_to_check, reduced_ws, resolution)

    def test_create_file_from_workspace_with_no_reduction_history(self):
        ws = LoadNexus("INTER13460")
        self._set_units_as_momentum_transfer(ws)

        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        self._check_data_in_file(ws, None)

    def test_create_file_for_workspace_with_empty_history(self):
        # The TOF workspace created from the ISIS Reflectometry reduction does not have any history
        input_ws = self._get_ws_from_reduction("INTER13460", 0.02, "TOF_13460")
        assert input_ws.getHistory().empty()
        self._set_units_as_momentum_transfer(input_ws)

        SaveISISReflectometryORSO(InputWorkspace=input_ws, Filename=self._output_filename)

        self._check_data_in_file(input_ws, None)

    def test_create_file_with_write_resolution_set_to_false_omits_resolution_column(self):
        ws = self._create_sample_workspace()
        SaveISISReflectometryORSO(InputWorkspace=ws, WriteResolution=False, Filename=self._output_filename)

        self._check_data_in_file(ws, None)

    def test_file_excludes_proposal_id_and_doi_if_logs_missing(self):
        ws = self._create_sample_workspace(rb_num_log_name="")
        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        excluded_values = [self._header_entries_for_sample_ws[0], self._header_entries_for_sample_ws[1]]

        self._check_file_header(None, excluded_values)

    def test_file_includes_proposal_id_and_doi_for_alternative_rb_num_log(self):
        ws = self._create_sample_workspace(rb_num_log_name=self._LOG_EXP_IDENTIFIER)
        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        self._check_file_header(self._header_entries_for_sample_ws)

    def test_input_ws_must_be_in_correct_units(self):
        ws = CreateSampleWorkspace()
        with self.assertRaisesRegex(RuntimeError, "must have units of"):
            SaveISISReflectometryORSO(InputWorkspace=ws, WriteResolution=False, Filename=self._output_filename)

    def test_comment_added_to_top_of_file(self):
        # The comment is needed until we are able to output all the mandatory information required by the standard
        ws = self._create_sample_workspace()
        SaveISISReflectometryORSO(InputWorkspace=ws, WriteResolution=False, Filename=self._output_filename)

        self._check_file_header([self._INVALID_HEADER_COMMENT])

    def _create_sample_workspace(self, rb_num_log_name=_LOG_RB_NUMBER):
        ws = CreateSampleWorkspace()
        self._set_units_as_momentum_transfer(ws)
        if rb_num_log_name:
            ws.mutableRun().addProperty(rb_num_log_name, self._rb_number, True)
        return ws

    def _set_units_as_momentum_transfer(self, ws):
        ws.getAxis(0).setUnit(self._Q_UNIT)

    def _get_ws_from_reduction(self, input_runs, resolution, reduced_ws_name, first_trans_runs=None, second_trans_runs=None):
        args = {
            "InputRunList": input_runs,
            "ProcessingInstructions": "4",
            "ThetaIn": 0.5,
            "WavelengthMin": 2,
            "WavelengthMax": 5,
            "I0MonitorIndex": 1,
            "MomentumTransferStep": resolution,
        }
        if first_trans_runs:
            args["FirstTransmissionRunList"] = first_trans_runs
        if second_trans_runs:
            args["SecondTransmissionRunList"] = second_trans_runs
        alg = create_algorithm("ReflectometryISISLoadAndProcess", **args)
        alg.setRethrows(True)
        assertRaisesNothing(self, alg.execute)
        return AnalysisDataService.retrieve(reduced_ws_name)

    def _get_header_values_expected_from_reduced_ws(self, reduced_ws, expected_data_files=None):
        expected_header_values = []
        history = self._get_reduction_history_for_reduced_ws(reduced_ws)
        self.assertIsNotNone(history)

        # Get the reduction timestamp from the history, in local time
        datetime_utc = datetime.strptime(history.executionDate().toISO8601String().split(".")[0], "%Y-%m-%dT%H:%M:%S")
        reduction_datetime = datetime_utc.replace(tzinfo=timezone.utc).astimezone(tz=None)
        expected_header_values.append(
            f"reduction:\n#   software: {{name: {MantidORSODataset.SOFTWARE_NAME}, version: {version()}}}\n#   timestamp: {reduction_datetime.isoformat()}\n#"
        )

        expected_header_values.append(f"data_set: {reduced_ws.name()}")

        if expected_data_files:
            expected_header_values.append(self._expected_data_file_header_values(expected_data_files))

        return expected_header_values

    def _expected_data_file_header_values(self, expected_entries):
        files_entry = [f"#     data_files:\n"]

        for run_file in expected_entries:
            files_entry.append(f"#     - file: {run_file}\n")
            files_entry.append(f"#       comment: Incident angle {self._runs_and_angles[run_file]}\n")

        return "".join(files_entry)

    @staticmethod
    def _expected_additional_file_header_values(expected_entries):
        files_entry = [f"#     additional_files:\n"]

        for run_file, comment in expected_entries.items():
            files_entry.append(f"#     - file: {run_file}\n")
            files_entry.append(f"#       comment: {comment}\n")

        return "".join(files_entry)

    def _get_reduction_history_for_reduced_ws(self, reduced_ws):
        for history in reversed(reduced_ws.getHistory().getAlgorithmHistories()):
            if history.name() == self._REDUCTION_ALG:
                return history

            if history.name() == self._REDUCTION_WORKFLOW_ALG:
                for child_history in reversed(history.getChildHistories()):
                    if child_history.name() == self._REDUCTION_ALG:
                        return child_history

        return None

    def _get_reduction_workflow_histories_for_reduced_ws(self, reduced_ws):
        workflow_histories = []
        for history in reduced_ws.getHistory().getAlgorithmHistories():
            if history.name() == self._REDUCTION_WORKFLOW_ALG:
                workflow_histories.append(history)

        return workflow_histories

    def _check_file_contents(self, header_values_to_check, ws, resolution, excluded_header_values=None):
        self._check_file_header(header_values_to_check, excluded_header_values)
        self._check_data_in_file(ws, resolution, check_file_exists=False)

    def _check_file_header(self, included_header_values=None, excluded_header_values=None):
        # Check that file is created with the correct name
        self.assertTrue(os.path.exists(self._output_filename))

        # Check inserted header items are all in the file
        with open(self._output_filename, "r") as orso_file:
            orso_header_and_data = orso_file.read()

        if included_header_values:
            for header_item in included_header_values:
                self.assertIn(header_item, orso_header_and_data)

        if excluded_header_values:
            assert orso_header_and_data
            for excluded_item in excluded_header_values:
                self.assertNotIn(excluded_item, orso_header_and_data)

    def _check_data_in_file(self, ws, resolution, check_file_exists=True):
        if check_file_exists:
            self.assertTrue(os.path.exists(self._output_filename))

        expected_data = ConvertToPointData(InputWorkspace=ws)

        orso_data = np.loadtxt(self._output_filename)

        expected_num_columns = 4 if resolution is not None else 3
        self.assertEqual(expected_num_columns, len(orso_data[0]))

        # Check the data in the columns
        q_data = orso_data[:, 0]
        r_data = orso_data[:, 1]
        sr_data = orso_data[:, 2]
        self.assertTrue(np.allclose(q_data, expected_data.readX(0), atol=1e-10, equal_nan=True))
        self.assertTrue(np.allclose(r_data, expected_data.readY(0), atol=1e-10, equal_nan=True))
        self.assertTrue(np.allclose(sr_data, expected_data.readE(0), atol=1e-10, equal_nan=True))
        if resolution is not None:
            sq_data = orso_data[:, 3]
            self.assertTrue(np.allclose(sq_data, q_data * resolution, atol=1e-10, equal_nan=True))


if __name__ == "__main__":
    unittest.main()
