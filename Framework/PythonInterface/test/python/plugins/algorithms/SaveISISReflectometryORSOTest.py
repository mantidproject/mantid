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

    def setUp(self):
        self._rb_number = str(123456)
        self._resolution = 0.05
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
        self._temp_dir.cleanup()

    def test_create_file_with_resolution_provided(self):
        ws = self._create_sample_workspace()
        SaveISISReflectometryORSO(InputWorkspace=ws, Resolution=self._resolution, Filename=self._output_filename)

        self._check_file_contents(self._header_entries_for_sample_ws, ws, self._resolution)

    def test_create_file_from_workspace_with_reduction_history(self):
        # Check that relevant information is extracted from the history produced by the ISIS Reflectometry reduction
        resolution = 0.02
        reduced_ws = self._get_ws_from_reduction("INTER13460", resolution, "IvsQ_binned_13460")
        SaveISISReflectometryORSO(InputWorkspace=reduced_ws, Filename=self._output_filename)

        header_values_to_check = self._get_header_values_expected_from_reduced_ws_history(reduced_ws)

        self._check_file_contents(header_values_to_check, reduced_ws, resolution)

    def test_create_file_for_period_data_workspace_with_reduction_history(self):
        # The history produced for the ISIS Reflectometry reduction is slightly different for period data, so
        # check that we can extract relevant information from this
        resolution = 0.02
        reduced_ws = self._get_ws_from_reduction("POLREF14966", resolution, "IvsQ_binned_14966_1")

        SaveISISReflectometryORSO(InputWorkspace=reduced_ws, Filename=self._output_filename)

        header_values_to_check = self._get_header_values_expected_from_reduced_ws_history(reduced_ws)

        self._check_file_contents(header_values_to_check, reduced_ws, resolution)

    def test_create_file_from_workspace_with_no_reduction_history(self):
        ws = LoadNexus("INTER13460")

        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        self._check_data_in_file(ws, None)

    def test_create_file_for_workspace_with_empty_history(self):
        # The TOF workspace created from the ISIS Reflectometry reduction does not have any history
        input_ws = self._get_ws_from_reduction("INTER13460", 0.02, "TOF_13460")
        assert input_ws.getHistory().empty()

        SaveISISReflectometryORSO(InputWorkspace=input_ws, Filename=self._output_filename)

        self._check_data_in_file(input_ws, None)

    def test_create_file_with_write_resolution_set_to_false_omits_resolution_column(self):
        ws = self._create_sample_workspace()
        SaveISISReflectometryORSO(InputWorkspace=ws, WriteResolution=False, Resolution=self._resolution, Filename=self._output_filename)

        self._check_data_in_file(ws, None)

    def test_file_excludes_proposal_id_and_doi_if_logs_missing(self):
        ws = self._create_sample_workspace(rb_num_log_name="")
        SaveISISReflectometryORSO(InputWorkspace=ws, Resolution=self._resolution, Filename=self._output_filename)

        excluded_values = [self._header_entries_for_sample_ws[0], self._header_entries_for_sample_ws[1]]

        self._check_file_header(None, excluded_values)

    def test_file_includes_proposal_id_and_doi_for_alternative_rb_num_log(self):
        ws = self._create_sample_workspace(rb_num_log_name=self._LOG_EXP_IDENTIFIER)
        SaveISISReflectometryORSO(InputWorkspace=ws, Resolution=self._resolution, Filename=self._output_filename)

        self._check_file_header(self._header_entries_for_sample_ws)

    def test_comment_added_to_top_of_file(self):
        # The comment is needed until we are able to output all the mandatory information required by the standard
        ws = self._create_sample_workspace()
        SaveISISReflectometryORSO(InputWorkspace=ws, WriteResolution=False, Filename=self._output_filename)

        self._check_file_header([self._INVALID_HEADER_COMMENT])

    def _create_sample_workspace(self, rb_num_log_name=_LOG_RB_NUMBER):
        ws = CreateSampleWorkspace()
        if rb_num_log_name:
            ws.mutableRun().addProperty(rb_num_log_name, self._rb_number, True)
        return ws

    def _get_ws_from_reduction(self, input_run, resolution, reduced_ws_name):
        args = {
            "InputRunList": input_run,
            "ProcessingInstructions": "4",
            "ThetaIn": 0.5,
            "WavelengthMin": 2,
            "WavelengthMax": 5,
            "I0MonitorIndex": 1,
            "MomentumTransferStep": resolution,
        }
        alg = create_algorithm("ReflectometryISISLoadAndProcess", **args)
        assertRaisesNothing(self, alg.execute)
        return AnalysisDataService.retrieve(reduced_ws_name)

    def _get_header_values_expected_from_reduced_ws_history(self, reduced_ws):
        expected_header_values = []
        history = self._get_reduction_history_for_reduced_ws(reduced_ws)
        self.assertIsNotNone(history)

        # Get the reduction timestamp from the history, in local time
        datetime_utc = datetime.strptime(history.executionDate().toISO8601String().split(".")[0], "%Y-%m-%dT%H:%M:%S")
        reduction_datetime = datetime_utc.replace(tzinfo=timezone.utc).astimezone(tz=None)
        expected_header_values.append(
            f"reduction:\n#   software: {{name: {MantidORSODataset.SOFTWARE_NAME}, version: {version()}}}\n#   timestamp: {reduction_datetime.isoformat()}\n#"
        )

        return expected_header_values

    def _get_reduction_history_for_reduced_ws(self, reduced_ws):
        for history in reversed(reduced_ws.getHistory().getAlgorithmHistories()):
            if history.name() == self._REDUCTION_ALG:
                return history

            if history.name() == self._REDUCTION_WORKFLOW_ALG:
                for child_history in reversed(history.getChildHistories()):
                    if child_history.name() == self._REDUCTION_ALG:
                        return child_history

        return None

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
