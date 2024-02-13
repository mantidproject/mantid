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
from unittest.mock import Mock, patch

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
    _LOG_RUN_NUM = "run_number"
    _INVALID_HEADER_COMMENT = "Mantid@ISIS output may not be fully ORSO compliant"
    _SAVE_ALG = "SaveISISReflectometryORSO"
    _REDUCTION_ALG = "ReflectometryReductionOneAuto"
    _REDUCTION_WORKFLOW_ALG = "ReflectometryISISLoadAndProcess"
    _STITCH_ALG = "Stitch1DMany"
    _Q_UNIT = "MomentumTransfer"
    _FIRST_TRANS_COMMENT = "First transmission run"
    _SECOND_TRANS_COMMENT = "Second transmission run"

    # Metadata headings
    _DATA_FILES_HEADING = "#     data_files:"
    _ADDITIONAL_FILES_HEADING = "#     additional_files:\n"
    _REDUCTION_HEADING = "# reduction:\n"
    _REDUCTION_CALL_HEADING = "#   call:"
    _DATA_SET_HEADING = "# data_set:"

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

    def tearDown(self):
        """
        Delete any workspaces created and remove the temp directory
        """
        AnalysisDataService.clear()
        config["default.facility"] = self._oldFacility
        config["default.instrument"] = self._oldInstrument
        self._temp_dir.cleanup()

    def test_create_file_from_workspace_with_reduction_history(self):
        # Check that relevant information is extracted from the history produced by the ISIS Reflectometry GUI reduction
        resolution = 0.02
        # One of the input runs should be an already loaded workspace as this appears differently in the history
        input_ws_name = "Test_ws"
        LoadNexus("38393", OutputWorkspace=input_ws_name)
        input_runs = f"0000013460, {input_ws_name}, inter38415.nxs"
        reduced_ws = self._get_ws_from_reduction(input_runs, resolution, f"IvsQ_13460+{input_ws_name}+38415", "13463", "13464, 38415")
        SaveISISReflectometryORSO(InputWorkspace=reduced_ws, Filename=self._output_filename)

        expected_data_files = [("INTER00013460", "0.5"), ("INTER00038393", "0.5"), ("INTER00038415", "0.5")]
        expected_additional_file_entries = {
            "INTER00013463": self._FIRST_TRANS_COMMENT,
            "INTER00013464": self._SECOND_TRANS_COMMENT,
            "INTER00038415": self._SECOND_TRANS_COMMENT,
        }
        metadata_to_check = self._get_basic_metadata_expected_from_reduced_ws(reduced_ws)
        metadata_to_check.append(self._get_expected_data_file_metadata(expected_data_files, self._ADDITIONAL_FILES_HEADING))
        metadata_to_check.append(self._get_expected_additional_file_metadata(expected_additional_file_entries, self._REDUCTION_HEADING))
        metadata_to_check.append(
            f"{self._REDUCTION_CALL_HEADING} {self._REDUCTION_WORKFLOW_ALG}(InputRunList='{input_runs.replace(' ', '')}'"
        )

        self._check_file_contents(metadata_to_check, reduced_ws, resolution)

    def test_create_file_from_workspace_with_stitched_reduction_history(self):
        # Testing the stitched reduction output helps to ensure that we're not getting duplicates in the metadata
        # because there will be multiple calls to ReflectometryISISLoadAndProcess in the history
        resolution = 0.02
        reduced_ws = self._get_ws_from_stitched_reduction(["13460", "38393"], resolution, "13463", "13464, 38415")
        SaveISISReflectometryORSO(InputWorkspace=reduced_ws, Filename=self._output_filename)

        expected_data_files = [("INTER00013460", "0.5"), ("INTER00038393", "0.5")]
        expected_additional_file_entries = {
            "INTER00013463": self._FIRST_TRANS_COMMENT,
            "INTER00013464": self._SECOND_TRANS_COMMENT,
            "INTER00038415": self._SECOND_TRANS_COMMENT,
        }
        metadata_to_check = self._get_basic_metadata_expected_from_reduced_ws(reduced_ws)
        metadata_to_check.append(self._get_expected_data_file_metadata(expected_data_files, self._ADDITIONAL_FILES_HEADING))
        metadata_to_check.append(self._get_expected_additional_file_metadata(expected_additional_file_entries, self._REDUCTION_HEADING))
        metadata_to_check.append(f"{self._REDUCTION_CALL_HEADING} '{self._REDUCTION_WORKFLOW_ALG}(InputRunList=''13460''")

        self._check_file_contents(metadata_to_check, reduced_ws, resolution)

    def test_create_file_for_period_data_workspace_with_reduction_history(self):
        # The history produced for the ISIS Reflectometry reduction is slightly different for period data, so
        # check that we can extract relevant information from this
        resolution = 0.02
        reduced_ws = self._get_ws_from_reduction("POLREF14966", resolution, "IvsQ_binned_14966_1")

        SaveISISReflectometryORSO(InputWorkspace=reduced_ws, Filename=self._output_filename)

        metadata_to_check = self._get_basic_metadata_expected_from_reduced_ws(reduced_ws)
        metadata_to_check.append(f"{self._REDUCTION_CALL_HEADING} '{self._REDUCTION_ALG}(InputWorkspace=''TOF_14966_1''")

        self._check_file_contents(metadata_to_check, reduced_ws, resolution)

    def test_create_file_from_workspace_with_no_reduction_history(self):
        ws = self._create_sample_workspace()

        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        self._check_data_in_file(ws, None)

    @patch("mantid.api.WorkspaceHistory.empty")
    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_create_file_for_workspace_with_empty_history(self, mock_alg_histories, mock_history_empty):
        input_ws = self._create_sample_workspace()
        self._configure_mock_alg_history(mock_alg_histories, [(self._STITCH_ALG, {"Params": "0.02"})])
        mock_history_empty.return_value = True

        SaveISISReflectometryORSO(InputWorkspace=input_ws, Filename=self._output_filename)

        excluded_values = [self._REDUCTION_CALL_HEADING]

        self._check_file_contents(None, input_ws, None, excluded_values)

    def test_create_file_with_write_resolution_set_to_false_omits_resolution_column(self):
        ws = self._create_sample_workspace()
        SaveISISReflectometryORSO(InputWorkspace=ws, WriteResolution=False, Filename=self._output_filename)

        self._check_data_in_file(ws, None)

    def test_file_excludes_proposal_id_and_doi_if_logs_missing(self):
        ws = self._create_sample_workspace(rb_num_log_name="")
        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        excluded_metadata = [self._header_entries_for_sample_ws[0], self._header_entries_for_sample_ws[1]]

        self._check_file_header(None, excluded_metadata)

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

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_input_run_data_files_are_formatted_correctly(self, mock_alg_histories):
        test_cases = [
            "0000013460.nxs",
            "INTER00013460.nxs",
            "INTER13460.nxs",
            "13460.nxs",
            "0000013460",
            "INTER00013460",
            "INTER13460",
            "13460",
        ]
        for test_case in test_cases:
            with self.subTest(test_case=test_case):
                theta = "0.5"
                expected_file_name = ("INTER00013460", theta)
                ws = self._create_sample_workspace(instrument_name="INTER")
                property_values = {
                    "InputRunList": test_case,
                    "ThetaIn": theta,
                }
                self._configure_mock_alg_history(mock_alg_histories, [(self._REDUCTION_WORKFLOW_ALG, property_values)])

                SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

                self._check_file_header([self._get_expected_data_file_metadata([expected_file_name], self._REDUCTION_HEADING)])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_transmission_run_additional_files_are_formatted_correctly(self, mock_alg_histories):
        test_cases = [
            ("0000013460.nxs", "0000038393.nxs"),
            ("INTER00013460.nxs", "INTER00038393.nxs"),
            ("INTER13460.nxs", "INTER00038393.nxs"),
            ("13460.nxs", "38393.nxs"),
            ("0000013460", "38393.nxs"),
            ("INTER00013460", "INTER00038393"),
            ("INTER13460", "INTER38393"),
            ("13460", "38393"),
        ]
        for first_test, second_test in test_cases:
            with self.subTest(first_test=first_test, second_test=second_test):
                expected_additional_file_entries = {
                    "INTER00013460": self._FIRST_TRANS_COMMENT,
                    "INTER00038393": self._SECOND_TRANS_COMMENT,
                }
                ws = self._create_sample_workspace(instrument_name="INTER")
                property_values = {
                    "FirstTransmissionRunList": first_test,
                    "SecondTransmissionRunList": second_test,
                }
                self._configure_mock_alg_history(mock_alg_histories, [(self._REDUCTION_WORKFLOW_ALG, property_values)])

                SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

                self._check_file_header(
                    [self._get_expected_additional_file_metadata(expected_additional_file_entries, self._REDUCTION_HEADING)]
                )

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_merged_runs_are_formatted_correctly(self, mock_alg_histories):
        theta = "0.5"
        expected_file_names = [("INTER00013460", theta), ("INTER00038393", theta)]
        ws = self._create_sample_workspace(instrument_name="INTER")
        property_values = {
            "InputRunList": "13460,38393",
            "ThetaIn": theta,
        }
        self._configure_mock_alg_history(mock_alg_histories, [(self._REDUCTION_WORKFLOW_ALG, property_values)])

        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        self._check_file_header([self._get_expected_data_file_metadata(expected_file_names, self._REDUCTION_HEADING)])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_old_SURF_runs_are_formatted_correctly(self, mock_alg_histories):
        theta = "0.5"
        expected_file_names = [("SRF79239", theta)]
        ws = self._create_sample_workspace(instrument_name="SURF")
        property_values = {
            "InputRunList": "79239",
            "ThetaIn": theta,
        }
        self._configure_mock_alg_history(mock_alg_histories, [(self._REDUCTION_WORKFLOW_ALG, property_values)])

        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        self._check_file_header([self._get_expected_data_file_metadata(expected_file_names, self._REDUCTION_HEADING)])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_run_is_ws_name_retrieves_run_number_from_log(self, mock_alg_histories):
        ws = self._create_sample_workspace(instrument_name="INTER")
        ws.mutableRun().addProperty(self._LOG_RUN_NUM, "12345", True)
        theta = "0.5"
        expected_file_names = [("INTER00012345", theta)]
        property_values = {
            "InputRunList": "ws",
            "ThetaIn": theta,
        }
        self._configure_mock_alg_history(mock_alg_histories, [(self._REDUCTION_WORKFLOW_ALG, property_values)])

        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        self._check_file_header([self._get_expected_data_file_metadata(expected_file_names, self._REDUCTION_HEADING)])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_run_is_ws_name_without_run_num_log_omits_datafiles(self, mock_alg_histories):
        ws = self._create_sample_workspace(instrument_name="INTER")
        property_values = {
            "InputRunList": "ws",
            "ThetaIn": "0.5",
        }
        self._configure_mock_alg_history(mock_alg_histories, [(self._REDUCTION_WORKFLOW_ALG, property_values)])

        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        self._check_file_header([f"{self._DATA_FILES_HEADING} []\n{self._REDUCTION_HEADING}"])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_input_run_cannot_convert_to_filename_omits_datafiles(self, mock_alg_histories):
        ws = self._create_sample_workspace(instrument_name="INTER")
        property_values = {
            "InputRunList": "test",
            "ThetaIn": "0.5",
        }
        self._configure_mock_alg_history(mock_alg_histories, [(self._REDUCTION_WORKFLOW_ALG, property_values)])

        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        self._check_file_header([f"{self._DATA_FILES_HEADING} []\n{self._REDUCTION_HEADING}"])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_stitched_reduction_history_creates_correct_input_file_entries(self, mock_alg_histories):
        ws = self._create_sample_workspace(instrument_name="INTER")
        histories_to_create = [
            (self._REDUCTION_WORKFLOW_ALG, {"InputRunList": "13460", "ThetaIn": "0.5"}),
            (self._REDUCTION_WORKFLOW_ALG, {"InputRunList": "13460", "ThetaIn": "1.0"}),
            (self._REDUCTION_WORKFLOW_ALG, {"InputRunList": "38393", "ThetaIn": "2.3"}),
        ]
        self._configure_mock_alg_history(mock_alg_histories, histories_to_create)

        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        expected_data_files = [("INTER00013460", "0.5"), ("INTER00013460", "1.0"), ("INTER00038393", "2.3")]

        self._check_file_header([self._get_expected_data_file_metadata(expected_data_files, self._REDUCTION_HEADING)])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_stitched_reduction_history_creates_correct_transmission_file_entries(self, mock_alg_histories):
        ws = self._create_sample_workspace(instrument_name="INTER")
        histories_to_create = [
            (self._REDUCTION_WORKFLOW_ALG, {"FirstTransmissionRunList": "13463", "SecondTransmissionRunList": "13464"}),
            (self._REDUCTION_WORKFLOW_ALG, {"FirstTransmissionRunList": "13463", "SecondTransmissionRunList": "13464"}),
            (self._REDUCTION_WORKFLOW_ALG, {"FirstTransmissionRunList": "13463", "SecondTransmissionRunList": "38415"}),
        ]
        self._configure_mock_alg_history(mock_alg_histories, histories_to_create)

        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        expected_additional_file_entries = {
            "INTER00013463": self._FIRST_TRANS_COMMENT,
            "INTER00013464": self._SECOND_TRANS_COMMENT,
            "INTER00038415": self._SECOND_TRANS_COMMENT,
        }

        self._check_file_header([self._get_expected_additional_file_metadata(expected_additional_file_entries, self._REDUCTION_HEADING)])

    def test_file_populates_reduction_call_from_ws_history(self):
        ws = self._create_sample_workspace(instrument_name="INTER")

        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        self._check_file_header(
            [
                f"{self._REDUCTION_CALL_HEADING} CreateSampleWorkspace(OutputWorkspace='ws', InstrumentName='INTER')\n{self._DATA_SET_HEADING}"
            ]
        )

    def _create_sample_workspace(self, rb_num_log_name=_LOG_RB_NUMBER, instrument_name=""):
        ws = CreateSampleWorkspace(InstrumentName=instrument_name)
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

    def _get_ws_from_stitched_reduction(self, input_runs, resolution, first_trans_runs=None, second_trans_runs=None):
        reduced_workspaces = []
        for run in input_runs:
            reduced_ws = self._get_ws_from_reduction(run, resolution, f"IvsQ_binned_{run}", first_trans_runs, second_trans_runs)
            reduced_workspaces.append(reduced_ws)

        stitched_ws_name = "stitched_ws"
        args = {
            "InputWorkspaces": reduced_workspaces,
            "OutputWorkspace": stitched_ws_name,
            "Params": resolution,
        }
        alg = create_algorithm("Stitch1DMany", **args)
        alg.setRethrows(True)
        assertRaisesNothing(self, alg.execute)
        return AnalysisDataService.retrieve(stitched_ws_name)

    def _get_basic_metadata_expected_from_reduced_ws(self, reduced_ws):
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

        # We currently want the resolution unit to be left blank. This will be populated at some point in the future.
        expected_header_values.append("- {error_of: Qz, error_type: resolution}\n")

        return expected_header_values

    def _get_expected_data_file_metadata(self, expected_entries, expected_section_end):
        files_entry = [f"{self._DATA_FILES_HEADING}\n"]

        for run_file, theta in expected_entries:
            files_entry.append(f"#     - file: {run_file}\n")
            files_entry.append(f"#       comment: Incident angle {theta}\n")

        files_entry.append(expected_section_end)
        return "".join(files_entry)

    def _get_expected_additional_file_metadata(self, expected_entries, expected_section_end):
        files_entry = [self._ADDITIONAL_FILES_HEADING]

        for run_file, comment in expected_entries.items():
            files_entry.append(f"#     - file: {run_file}\n")
            files_entry.append(f"#       comment: {comment}\n")

        files_entry.append(expected_section_end)
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

    def _configure_mock_alg_history(self, mock_alg_histories, histories_to_create):
        histories = []
        for alg_name, property_values in histories_to_create:
            histories.append(self._create_mock_alg_history(alg_name, property_values))

        mock_alg_histories.return_value = histories

    @staticmethod
    def _create_mock_alg_history(alg_name, property_values):
        history = Mock()
        history.name = Mock(return_value=alg_name)
        history.getChildHistories = Mock(return_value=[])

        def mock_get_property_value(key):
            try:
                return property_values[key]
            except KeyError:
                return ""

        history.getPropertyValue = Mock(side_effect=mock_get_property_value)
        return history

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
                self.assertEqual(orso_header_and_data.count(header_item), 1, "Entry duplicated in ORSO file header")

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
