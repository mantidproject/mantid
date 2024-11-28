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
from unittest.mock import Mock, patch
from pathlib import Path
from datetime import datetime, timezone, date, time

from mantid import config
from mantid.simpleapi import CreateSampleWorkspace, SaveISISReflectometryORSO, ConvertToPointData, GroupWorkspaces, AddSampleLog
from mantid.api import AnalysisDataService
from mantid.kernel import version, DateAndTime
from mantid.utils.reflectometry.orso_helper import MantidORSODataset, MantidORSOSaver


class SaveISISReflectometryORSOTest(unittest.TestCase):
    """Tests for the ISIS Reflectometry implementation of the ORSO file format.
    These tests focus on the file contents that are specific to ISIS.
    See mantid/utils/reflectometry/orso_helper_test.py for tests covering the shared aspects of the ORSO file output.
    """

    _LOG_RB_NUMBER = "rb_proposal"
    _LOG_EXP_IDENTIFIER = "experiment_identifier"
    _LOG_RUN_NUM = "run_number"

    _INVALID_HEADER_COMMENT = "Mantid@ISIS output may not be fully ORSO compliant"
    _Q_UNIT = "MomentumTransfer"
    _FIRST_TRANS_COMMENT = "First transmission run"
    _SECOND_TRANS_COMMENT = "Second transmission run"
    _FLOOD_WS_COMMENT = "Flood correction workspace or file"
    _FLOOD_RUN_COMMENT = "Flood correction run file"
    _CALIB_FILE_COMMENT = "Calibration file"
    _STITCHED_DATASET_NAME = "Stitched"

    _NUM_COLS_BASIC = 3
    _NUM_COLS_BASIC_WITH_RES = 4
    _NUM_COLS_EXTENDED = 7
    _NUM_COLS_EXTENDED_WITH_RES = 8

    # Algorithm names
    _SAVE_ALG = "SaveISISReflectometryORSO"
    _RRO_ALG = "ReflectometryReductionOne"
    _REDUCTION_ALG = "ReflectometryReductionOneAuto"
    _REDUCTION_WORKFLOW_ALG = "ReflectometryISISLoadAndProcess"
    _STITCH_ALG = "Stitch1DMany"
    _CREATE_FLOOD_ALG = "CreateFloodWorkspace"
    _REF_ROI = "RefRoi"
    _CONVERT_UNITS = "ConvertUnits"
    _REBIN_ALG = "Rebin"

    # Metadata headings
    _DATA_FILES_HEADING = "#     data_files:"
    _ADDITIONAL_FILES_HEADING = "#     additional_files:\n"
    _REDUCTION_HEADING = "# reduction:\n"
    _REDUCTION_CALL_HEADING = "#   call:"
    _DATA_SET_HEADING = "# data_set:"

    # Error messages
    _WS_UNITS_ERROR = "must have units of"
    _WS_NUM_SPECTRA_ERROR = "must contain only one spectrum"
    _WS_NOT_FOUND_ERROR = "Cannot find workspace"
    _FILE_EXT_ERROR = "must end with a supported ORSO extension"

    def setUp(self):
        self._oldFacility = config["default.facility"]
        if self._oldFacility.strip() == "":
            self._oldFacility = "TEST_LIVE"
        self._oldInstrument = config["default.instrument"]
        config["default.facility"] = "ISIS"
        config["default.instrument"] = "INTER"

        self._rb_number = str(123456)
        self._filename = f"ORSO_save_test{MantidORSOSaver.ASCII_FILE_EXT}"
        self._temp_dir = tempfile.TemporaryDirectory()
        self._output_filename = os.path.join(self._temp_dir.name, self._filename)
        # The optional ISIS header entries that are not taken from the ws history
        # and that will usually be saved out from the test workspace produced by _create_sample_workspace
        self._header_entries_for_sample_ws = [
            f"proposalID: '{self._rb_number}'",
            f"doi: 10.5286/ISIS.E.RB{self._rb_number}",
            "facility: ISIS",
            f"creator:\n#     name: {self._SAVE_ALG}\n{self._get_affiliation_entry()}\n#",
        ]

    def tearDown(self):
        """
        Delete any workspaces created and remove the temp directory
        """
        AnalysisDataService.clear()
        config["default.facility"] = self._oldFacility
        config["default.instrument"] = self._oldInstrument
        self._temp_dir.cleanup()

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_file_populates_software_version_and_reduction_timestamp(self, mock_alg_histories):
        input_ws = self._create_sample_workspace()
        history = self._create_mock_alg_history(self._REDUCTION_ALG, {"InputWorkspace": "input_ws"}, [Mock()])
        expected_value = datetime.combine(date(2024, 2, 13), time(12, 14, 36)).replace(tzinfo=timezone.utc).astimezone(tz=None)
        history.executionDate = Mock(return_value=DateAndTime("2024-02-13T12:14:36.073814000"))
        mock_alg_histories.return_value = [history]

        self._run_save_alg(input_ws)

        self._check_file_header(
            [
                f"reduction:\n#   software: {{name: {MantidORSODataset.SOFTWARE_NAME}, version: {version()}}}\n"
                f"#   timestamp: {expected_value.isoformat()}\n#"
            ]
        )

    def test_create_file_from_workspace_with_no_reduction_history(self):
        ws = self._create_sample_workspace()

        self._run_save_alg(ws)

        self._check_data_in_file(ws, None)

    @patch("mantid.api.WorkspaceHistory.empty")
    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_create_file_for_workspace_with_empty_history(self, mock_alg_histories, mock_history_empty):
        input_ws = self._create_sample_workspace()
        self._configure_mock_alg_history(mock_alg_histories, [(self._STITCH_ALG, {"Params": "0.02"})])
        mock_history_empty.return_value = True

        self._run_save_alg(input_ws)

        excluded_values = [self._REDUCTION_CALL_HEADING]

        self._check_file_contents(None, input_ws, None, excluded_values)

    def test_create_file_with_write_resolution_set_to_false_omits_resolution_column(self):
        ws = self._create_sample_workspace()
        self._run_save_alg(ws, write_resolution=False)

        self._check_data_in_file(ws, None)

    def test_file_excludes_proposal_id_and_doi_if_logs_missing(self):
        ws = self._create_sample_workspace(rb_num_log_name="")
        self._run_save_alg(ws)

        excluded_metadata = [self._header_entries_for_sample_ws[0], self._header_entries_for_sample_ws[1]]

        self._check_file_header(None, excluded_metadata)

    def test_file_includes_proposal_id_and_doi_for_alternative_rb_num_log(self):
        ws = self._create_sample_workspace(rb_num_log_name=self._LOG_EXP_IDENTIFIER)
        self._run_save_alg(ws)

        self._check_file_header(self._header_entries_for_sample_ws)

    def test_all_input_workspaces_must_be_in_correct_units(self):
        ws_correct = self._create_sample_workspace()
        ws_invalid = CreateSampleWorkspace()
        with self.assertRaisesRegex(RuntimeError, self._WS_UNITS_ERROR):
            self._run_save_alg([ws_correct, ws_invalid], write_resolution=False)

    def test_all_workspaces_in_group_must_be_in_correct_units(self):
        ws_correct = self._create_sample_workspace()
        ws_invalid = CreateSampleWorkspace()
        grp = GroupWorkspaces(InputWorkspaces=[ws_correct.name(), ws_invalid.name()], OutputWorkspace="test_ws_group")
        with self.assertRaisesRegex(RuntimeError, self._WS_UNITS_ERROR):
            self._run_save_alg(grp, write_resolution=False)

    def test_all_input_workspaces_must_have_a_single_spectrum(self):
        ws_correct = self._create_sample_workspace()
        ws_invalid = CreateSampleWorkspace(XUnit=self._Q_UNIT, NumBanks=1, BankPixelWidth=2)
        with self.assertRaisesRegex(RuntimeError, self._WS_NUM_SPECTRA_ERROR):
            self._run_save_alg([ws_correct, ws_invalid], write_resolution=False)

    def test_all_workspaces_in_group_must_have_a_single_spectrum(self):
        ws_correct = self._create_sample_workspace()
        ws_invalid = CreateSampleWorkspace(XUnit=self._Q_UNIT, NumBanks=1, BankPixelWidth=2)
        grp = GroupWorkspaces(InputWorkspaces=[ws_correct.name(), ws_invalid.name()], OutputWorkspace="test_ws_group")
        with self.assertRaisesRegex(RuntimeError, self._WS_NUM_SPECTRA_ERROR):
            self._run_save_alg(grp, write_resolution=False)

    def test_all_input_workspaces_must_exist_in_ADS(self):
        ws = self._create_sample_workspace()
        with self.assertRaisesRegex(RuntimeError, self._WS_NOT_FOUND_ERROR):
            self._run_save_alg([ws, "invalid"], write_resolution=False)

    def test_comment_added_to_top_of_file(self):
        # The comment is needed until we are able to output all the mandatory information required by the standard
        ws = self._create_sample_workspace()
        self._run_save_alg(ws, write_resolution=False)

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

                self._run_save_alg(ws)

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

                self._run_save_alg(ws)

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

        self._run_save_alg(ws)

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

        self._run_save_alg(ws)

        self._check_file_header([self._get_expected_data_file_metadata(expected_file_names, self._REDUCTION_HEADING)])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_input_run_is_ws_name_retrieves_run_number_from_log(self, mock_alg_histories):
        ws = self._create_sample_workspace(instrument_name="INTER")
        ws.mutableRun().addProperty(self._LOG_RUN_NUM, "12345", True)
        theta = "0.5"
        expected_file_names = [("INTER00012345", theta)]
        property_values = {
            "InputRunList": "ws",
            "ThetaIn": theta,
        }
        self._configure_mock_alg_history(mock_alg_histories, [(self._REDUCTION_WORKFLOW_ALG, property_values)])

        self._run_save_alg(ws)

        self._check_file_header([self._get_expected_data_file_metadata(expected_file_names, self._REDUCTION_HEADING)])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_input_run_is_ws_name_without_run_num_log_omits_datafiles(self, mock_alg_histories):
        ws = self._create_sample_workspace(instrument_name="INTER")
        property_values = {
            "InputRunList": "ws",
            "ThetaIn": "0.5",
        }
        self._configure_mock_alg_history(mock_alg_histories, [(self._REDUCTION_WORKFLOW_ALG, property_values)])

        self._run_save_alg(ws)

        self._check_file_header([f"{self._DATA_FILES_HEADING} []\n{self._REDUCTION_HEADING}"])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_input_run_cannot_convert_to_filename_omits_datafiles(self, mock_alg_histories):
        ws = self._create_sample_workspace(instrument_name="INTER")
        property_values = {
            "InputRunList": "test",
            "ThetaIn": "0.5",
        }
        self._configure_mock_alg_history(mock_alg_histories, [(self._REDUCTION_WORKFLOW_ALG, property_values)])

        self._run_save_alg(ws)

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

        self._run_save_alg(ws)

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

        self._run_save_alg(ws)

        expected_additional_file_entries = {
            "INTER00013463": self._FIRST_TRANS_COMMENT,
            "INTER00013464": self._SECOND_TRANS_COMMENT,
            "INTER00038415": self._SECOND_TRANS_COMMENT,
        }

        self._check_file_header([self._get_expected_additional_file_metadata(expected_additional_file_entries, self._REDUCTION_HEADING)])

    def test_file_populates_reduction_call_from_ws_history(self):
        ws = self._create_sample_workspace(instrument_name="INTER")

        self._run_save_alg(ws)

        self._check_file_header(
            [f"{self._get_affiliation_entry()}\n{self._REDUCTION_CALL_HEADING} CreateSampleWorkspace(OutputWorkspace='ws'"]
        )

    def test_file_excludes_reduction_call_for_ws_groups(self):
        ws = self._create_sample_workspace_group(["ws_1", "ws_2"], instrument_name="INTER")

        self._run_save_alg(ws)

        self._check_file_header(excluded_header_values=[f"{self._get_affiliation_entry()}\n{self._REDUCTION_CALL_HEADING}"])

    def test_file_excludes_reduction_call_for_ws_group_member(self):
        ws_name = "ws_1"
        self._create_sample_workspace_group([ws_name, "ws_2"], instrument_name="INTER")

        self._run_save_alg(ws_name)

        self._check_file_header(excluded_header_values=[f"{self._get_affiliation_entry()}\n{self._REDUCTION_CALL_HEADING}"])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_flood_correction_ws_included_in_additional_files(self, mock_alg_histories):
        test_cases = [
            (Path("test/path_to/flood_file.nxs"), "flood_file.nxs"),
            ("ws_name", "ws_name"),
        ]
        for flood_ws_param, expected_entry in test_cases:
            with self.subTest(flood_ws_param=flood_ws_param, expected_entry=expected_entry):
                ws = self._create_sample_workspace()
                property_values = {
                    "FloodCorrection": "Workspace",
                    "FloodWorkspace": str(flood_ws_param),
                }
                self._configure_mock_alg_history(mock_alg_histories, [(self._REDUCTION_WORKFLOW_ALG, property_values)])

                self._run_save_alg(ws)

                self._check_file_header(
                    [self._get_expected_additional_file_metadata({expected_entry: self._FLOOD_WS_COMMENT}, self._REDUCTION_HEADING)]
                )

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_flood_correction_from_parameter_file_included_in_additional_files(self, mock_alg_histories):
        flood_run_file = "flood_run.nxs"
        test_path = Path(f"path/to/{flood_run_file}")
        flood_history = self._create_mock_alg_history(self._CREATE_FLOOD_ALG, {"Filename": str(test_path)})
        red_history = self._create_mock_alg_history(self._REDUCTION_ALG, {}, [flood_history])
        property_values = {
            "FloodCorrection": "ParameterFile",
            "FloodWorkspace": "",
        }
        history = self._create_mock_alg_history(self._REDUCTION_WORKFLOW_ALG, property_values, [red_history])
        mock_alg_histories.return_value = [history]

        ws = self._create_sample_workspace()

        self._run_save_alg(ws)

        self._check_file_header(
            [self._get_expected_additional_file_metadata({flood_run_file: self._FLOOD_RUN_COMMENT}, self._REDUCTION_HEADING)]
        )

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_flood_correction_excluded_if_not_in_history(self, mock_alg_histories):
        filename = "test_file.nxs"
        test_path = Path(f"path/to/{filename}")
        child_history = self._create_mock_alg_history("test_alg", {"Filename": str(test_path)})
        red_history = self._create_mock_alg_history(self._REDUCTION_ALG, {}, [child_history])
        property_values = {
            "FloodCorrection": "ParameterFile",
            "FloodWorkspace": "",
        }
        history = self._create_mock_alg_history(self._REDUCTION_WORKFLOW_ALG, property_values, [red_history])
        mock_alg_histories.return_value = [history]

        ws = self._create_sample_workspace()

        self._run_save_alg(ws)

        self._check_file_header(None, [filename])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_calibration_file_included_in_additional_files(self, mock_alg_histories):
        filename = "calib_file.nxs"
        test_path = Path(f"path/to/{filename}")
        histories_to_create = [(self._REDUCTION_WORKFLOW_ALG, {"CalibrationFile": str(test_path)})]
        ws = self._create_sample_workspace()

        self._configure_mock_alg_history(mock_alg_histories, histories_to_create)

        self._run_save_alg(ws)

        self._check_file_header(
            [self._get_expected_additional_file_metadata({filename: self._CALIB_FILE_COMMENT}, self._REDUCTION_HEADING)]
        )

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_calibration_file_excluded_if_not_in_history(self, mock_alg_histories):
        histories_to_create = [(self._REDUCTION_WORKFLOW_ALG, {"CalibrationFile": ""})]
        ws = self._create_sample_workspace()
        self._configure_mock_alg_history(mock_alg_histories, histories_to_create)

        self._run_save_alg(ws)

        self._check_file_header(None, [self._CALIB_FILE_COMMENT])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_stitched_data_has_correct_dataset_name(self, mock_alg_histories):
        ws = self._create_sample_workspace()
        self._configure_mock_alg_history(mock_alg_histories, [(self._STITCH_ALG, {})])

        self._run_save_alg(ws, write_resolution=False)

        self._check_file_header([self._get_dataset_name_entry(self._STITCHED_DATASET_NAME)])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_unstitched_data_gets_dataset_name_from_ref_roi(self, mock_alg_histories):
        angle = "2.300"
        ws = self._create_sample_workspace()
        self._configure_q_conversion_alg_mock_history(mock_alg_histories, self._REF_ROI, {"ScatteringAngle": angle})

        self._run_save_alg(ws, write_resolution=False)

        self._check_file_header([self._get_dataset_name_entry(f"'{angle}'")])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_unstitched_data_gets_dataset_name_from_convert_units(self, mock_alg_histories):
        ws = self._create_sample_workspace()
        self._configure_q_conversion_alg_mock_history(mock_alg_histories, self._CONVERT_UNITS, {})

        self._run_save_alg(ws, write_resolution=False)

        self._check_file_header([self._get_dataset_name_entry(f"'{self._get_theta_value(ws):.3f}'")])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_unstitched_data_sets_dataset_name_to_ws_name_as_default(self, mock_alg_histories):
        ws = self._create_sample_workspace()
        self._configure_q_conversion_alg_mock_history(mock_alg_histories, "UnsupportedAlgorithm", {})

        self._run_save_alg(ws, write_resolution=False)

        self._check_file_header([self._get_dataset_name_entry(ws.name())])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_workspace_group_data_generates_unique_dataset_names(self, mock_alg_histories):
        ws_names = ["ws_1", "ws_2"]
        ws_grp = self._create_sample_workspace_group(ws_names)
        self._configure_mock_alg_history(mock_alg_histories, [(self._STITCH_ALG, {})])

        self._run_save_alg([ws_grp], write_resolution=False)

        self._check_file_header([self._get_dataset_name_entry(f"{ws_name} {self._STITCHED_DATASET_NAME}") for ws_name in ws_names])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_workspace_group_sets_dataset_name_to_ws_name_as_default(self, mock_alg_histories):
        ws_names = ["ws_1", "ws_2"]
        ws_grp = self._create_sample_workspace_group(ws_names)
        self._configure_q_conversion_alg_mock_history(mock_alg_histories, "UnsupportedAlgorithm", {})

        self._run_save_alg(ws_grp, write_resolution=False)

        self._check_file_header([self._get_dataset_name_entry(ws_name) for ws_name in ws_names])

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_additional_columns_included_if_requested_and_is_unstitched_data_no_resolution(self, mock_alg_histories):
        angle = "2.3"
        ws = self._create_sample_workspace()
        self._configure_q_conversion_alg_mock_history(mock_alg_histories, self._REF_ROI, {"ScatteringAngle": angle})
        self._run_save_alg(ws, write_resolution=False, include_extra_cols=True)
        self._check_num_columns_in_file(self._NUM_COLS_EXTENDED)

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_additional_columns_included_if_requested_and_is_unstitched_data_with_resolution(self, mock_alg_histories):
        angle = "2.3"
        ws = self._create_sample_workspace()
        self._configure_q_conversion_alg_mock_history(mock_alg_histories, self._REF_ROI, {"ScatteringAngle": angle}, True)
        self._run_save_alg(ws, write_resolution=False, include_extra_cols=True)
        self._check_num_columns_in_file(self._NUM_COLS_EXTENDED_WITH_RES)

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_additional_columns_excluded_if_requested_but_is_stitched_data(self, mock_alg_histories):
        ws = self._create_sample_workspace()
        self._configure_mock_alg_history(mock_alg_histories, [(self._STITCH_ALG, {"Params": 0.02})])
        self._run_save_alg(ws, write_resolution=False, include_extra_cols=True)
        self._check_num_columns_in_file(self._NUM_COLS_BASIC_WITH_RES)

    def test_additional_columns_excluded_if_no_conversion_history(self):
        ws = self._create_sample_workspace()
        self._run_save_alg(ws, write_resolution=True, include_extra_cols=True)
        self._check_num_columns_in_file(self._NUM_COLS_BASIC)

    @patch("mantid.api.WorkspaceHistory.getAlgorithmHistories")
    def test_additional_columns_excluded_if_no_supported_conversion_alg_in_history(self, mock_alg_histories):
        ws = self._create_sample_workspace()
        self._configure_q_conversion_alg_mock_history(mock_alg_histories, "UnsupportedAlgorithm", {})

        self._run_save_alg(ws, write_resolution=True, include_extra_cols=True)

        self._check_num_columns_in_file(self._NUM_COLS_BASIC)

    def test_filename_must_have_supported_extension(self):
        ws = self._create_sample_workspace()
        with self.assertRaisesRegex(RuntimeError, self._FILE_EXT_ERROR):
            self._run_save_alg([ws], write_resolution=False, filename="file_extension.invalid")

    @patch("plugins.algorithms.SaveISISReflectometryORSO.MantidORSOSaver.save_orso_ascii")
    @patch("plugins.algorithms.SaveISISReflectometryORSO.MantidORSOSaver.save_orso_nexus")
    def test_saved_as_nexus_if_relevant_filename_extension(self, mock_save_orso_nexus, mock_save_orso_ascii):
        ws = self._create_sample_workspace()
        self._run_save_alg(ws, filename=f"test_nexus_save{MantidORSOSaver.NEXUS_FILE_EXT}")
        mock_save_orso_nexus.assert_called_once()
        mock_save_orso_ascii.assert_not_called()

    def test_data_with_spin_state_logs_adds_polarization_metadata_in_instrument_settings_header(self):
        spin_states = ["pp", "mm", "mp", "pm"]
        ws_grp = self._create_sample_workspace_group_with_spin_state(spin_states, instrument_name="POLREF")

        self._run_save_alg(ws_grp, write_resolution=False, include_extra_cols=False)

        self._check_file_header(["instrument_settings:\n" "#       incident_angle: null\n" "#       wavelength: null"])
        for state in spin_states:
            self._check_file_header([f"#       polarization: {state}"])

    def _create_sample_workspace(self, rb_num_log_name=_LOG_RB_NUMBER, instrument_name="", ws_name="ws"):
        # Create a single spectrum workspace in units of momentum transfer
        ws = CreateSampleWorkspace(
            InstrumentName=instrument_name, OutputWorkspace=ws_name, XUnit=self._Q_UNIT, NumBanks=1, BankPixelWidth=1
        )
        if rb_num_log_name:
            ws.mutableRun().addProperty(rb_num_log_name, self._rb_number, True)
        return ws

    def _create_sample_workspace_group(self, member_ws_names, group_name="sample_group", instrument_name=""):
        for ws_name in member_ws_names:
            self._create_sample_workspace(ws_name=ws_name, instrument_name=instrument_name)
        return GroupWorkspaces(InputWorkspaces=",".join(member_ws_names), OutputWorkspace=group_name)

    def _create_sample_workspace_group_with_spin_state(self, spin_states, instrument_name=""):
        group_member_names = []
        for state in spin_states:
            ws_name = "ws_" + state
            group_member_names.append(ws_name)
            self._create_sample_workspace(ws_name=ws_name, instrument_name=instrument_name)
            AddSampleLog(Workspace=ws_name, LogName="spin_state_ORSO", LogText=state)
        return GroupWorkspaces(InputWorkspaces=",".join(group_member_names), OutputWorkspace="group_pol")

    def _get_expected_data_file_metadata(self, expected_entries, expected_section_end):
        files_entry = [f"{self._DATA_FILES_HEADING}\n"]

        for run_file, theta in expected_entries:
            files_entry.append(f"#     - file: {run_file}\n")
            files_entry.append(f"#       comment: Reduction input angle {theta}\n")

        files_entry.append(expected_section_end)
        return "".join(files_entry)

    def _get_expected_additional_file_metadata(self, expected_entries, expected_section_end):
        files_entry = [self._ADDITIONAL_FILES_HEADING]

        for run_file, comment in expected_entries.items():
            files_entry.append(f"#     - file: {run_file}\n")
            files_entry.append(f"#       comment: {comment}\n")

        files_entry.append(expected_section_end)
        return "".join(files_entry)

    def _get_dataset_name_entry(self, dataset_name):
        return f"{self._DATA_SET_HEADING} {dataset_name}\n#"

    @staticmethod
    def _get_affiliation_entry():
        return f"#     affiliation: {MantidORSODataset.SOFTWARE_NAME}"

    def _configure_mock_alg_history(self, mock_alg_histories, histories_to_create):
        histories = []
        for alg_name, property_values in histories_to_create:
            histories.append(self._create_mock_alg_history(alg_name, property_values))

        mock_alg_histories.return_value = histories

    def _create_mock_alg_history(self, alg_name, property_values, child_histories=None):
        history = Mock()
        history.name = Mock(return_value=alg_name)
        history.getChildHistories = Mock(return_value=[]) if child_histories is None else Mock(return_value=child_histories)
        if alg_name == self._REDUCTION_ALG:
            history.executionDate = Mock(return_value=DateAndTime("2024-03-05T12:30:30.000000000"))

        def mock_get_property_value(key):
            try:
                return property_values[key]
            except KeyError:
                return ""

        history.getPropertyValue = Mock(side_effect=mock_get_property_value)
        return history

    def _configure_q_conversion_alg_mock_history(self, mock_alg_histories, q_convert_alg_name, property_values, has_resolution=False):
        convert_history = self._create_mock_alg_history(q_convert_alg_name, property_values)
        rro_history = self._create_mock_alg_history(self._RRO_ALG, {}, [convert_history])
        if has_resolution:
            rebin_history = self._create_mock_alg_history(self._REBIN_ALG, {"Params": "0.1,0.02,0.3"})
            red_history = self._create_mock_alg_history(self._REDUCTION_ALG, {}, [rro_history, rebin_history])
        else:
            red_history = self._create_mock_alg_history(self._REDUCTION_ALG, {}, [rro_history])
        mock_alg_histories.return_value = [red_history]

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

        expected_num_columns = self._NUM_COLS_BASIC_WITH_RES if resolution is not None else self._NUM_COLS_BASIC
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

    def _check_num_columns_in_file(self, expected_num_columns):
        self.assertTrue(os.path.exists(self._output_filename))
        orso_data = np.loadtxt(self._output_filename)
        self.assertEqual(expected_num_columns, len(orso_data[0]))
        return orso_data

    def _get_theta_value(self, ws):
        spectrum_info = ws.spectrumInfo()
        self.assertEqual(1, spectrum_info.size())
        return float(np.rad2deg(spectrum_info.signedTwoTheta(0))) / 2.0

    def _run_save_alg(self, ws_list, write_resolution=True, include_extra_cols=False, filename=None):
        SaveISISReflectometryORSO(
            WorkspaceList=ws_list,
            WriteResolution=write_resolution,
            IncludeAdditionalColumns=include_extra_cols,
            Filename=self._output_filename if filename is None else os.path.join(self._temp_dir.name, filename),
        )


if __name__ == "__main__":
    unittest.main()
