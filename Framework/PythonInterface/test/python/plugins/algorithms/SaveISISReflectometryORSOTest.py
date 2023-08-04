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

from mantid.simpleapi import (
    CreateSampleWorkspace,
    SaveISISReflectometryORSO,
    ConvertToPointData,
    NRCalculateSlitResolution,
    LoadNexus,
    DeleteLog,
)
from mantid.api import AnalysisDataService


class SaveISISReflectometryORSOTest(unittest.TestCase):
    """Tests for the ISIS Reflectometry implementation of the ORSO file format.
    These tests focus on the file contents that are specific to ISIS.
    See mantid/utils/reflectometry/orso_helper_test.py for tests covering the shared aspects of the ORSO file output.
    """

    _LOG_RB_NUMBER = "rb_proposal"
    _LOG_EXP_IDENTIFIER = "experiment_identifier"
    _LOG_THETA = "Theta"
    _INVALID_HEADER_COMMENT = "Mantid@ISIS output may not be fully ORSO compliant"

    def setUp(self):
        """
        Create the workspace and then ad logs to it so that it has the appropriate properties for
        the SaveReflectometryORSO algorithm to read and save out.
        """
        self._rb_number = str(123456)
        self._resolution = 0.05
        self._filename = "ORSO_save_test.ort"
        self._temp_dir = tempfile.TemporaryDirectory()
        self._output_filename = os.path.join(self._temp_dir.name, self._filename)

    def tearDown(self):
        """
        Delete any workspaces created and remove the temp directory
        """
        AnalysisDataService.clear()
        self._temp_dir.cleanup()

    def test_create_file_with_resolution_provided(self):
        ws = self._create_sample_workspace()
        SaveISISReflectometryORSO(InputWorkspace=ws, Resolution=self._resolution, Filename=self._output_filename)

        header_values_to_check = [f"proposalID: '{self._rb_number}'", f"doi: 10.5286/ISIS.E.RB{self._rb_number}", "facility: ISIS"]

        self._check_file_contents(header_values_to_check, ws, self._resolution)

    def test_create_file_with_no_resolution_provided_calculates_default_using_theta_log(self):
        ws = LoadNexus("INTER13460")
        theta = ws.getRun().getProperty(self._LOG_THETA).lastValue()
        resolution = NRCalculateSlitResolution(Workspace=ws, TwoTheta=2 * theta)

        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        self._check_data_in_file(ws, resolution)

    def test_create_file_with_no_resolution_provided_calculates_default_using_theta_provided(self):
        ws = LoadNexus("INTER13460")
        DeleteLog(ws, self._LOG_THETA)
        theta = 2.3
        resolution = NRCalculateSlitResolution(Workspace=ws, TwoTheta=2 * theta)

        SaveISISReflectometryORSO(InputWorkspace=ws, ThetaIn=theta, Filename=self._output_filename)

        self._check_data_in_file(ws, resolution)

    def test_create_file_with_resolution_calculation_error_omits_resolution_column(self):
        # This workspace will cause NRCalculateSlitResolution to throw an error because it doesn't have the required
        # slit instrument components
        ws = self._create_sample_workspace()
        SaveISISReflectometryORSO(InputWorkspace=ws, Filename=self._output_filename)

        self._check_data_in_file(ws, None)

    def test_create_file_with_write_resolution_set_to_false_omits_resolution_column(self):
        ws = self._create_sample_workspace()
        SaveISISReflectometryORSO(InputWorkspace=ws, WriteResolution=False, Resolution=self._resolution, Filename=self._output_filename)

        self._check_data_in_file(ws, None)

    def test_file_excludes_proposal_id_and_doi_if_logs_missing(self):
        ws = self._create_sample_workspace(rb_num_log_name="")
        SaveISISReflectometryORSO(InputWorkspace=ws, Resolution=self._resolution, Filename=self._output_filename)

        excluded_values = [
            f"proposalID: '{self._rb_number}'",
            f"doi: 10.5286/ISIS.E.RB{self._rb_number}",
        ]

        self._check_file_header(None, excluded_values)

    def test_file_includes_proposal_id_and_doi_for_alternative_rb_num_log(self):
        ws = self._create_sample_workspace(rb_num_log_name=self._LOG_EXP_IDENTIFIER)
        SaveISISReflectometryORSO(InputWorkspace=ws, Resolution=self._resolution, Filename=self._output_filename)

        included_values = [f"proposalID: '{self._rb_number}'", f"doi: 10.5286/ISIS.E.RB{self._rb_number}", "facility: ISIS"]

        self._check_file_header(included_values)

    def test_comment_added_to_top_of_file(self):
        # The comment is needed until we are able to output all the mandatory information required by the standard
        ws = self._create_sample_workspace()
        SaveISISReflectometryORSO(InputWorkspace=ws, WriteResolution=False, Filename=self._output_filename)

        self._check_file_header([self._INVALID_HEADER_COMMENT])

    def _create_sample_workspace(self, rb_num_log_name=_LOG_RB_NUMBER, theta=None):
        ws = CreateSampleWorkspace()
        if rb_num_log_name:
            ws.mutableRun().addProperty(rb_num_log_name, self._rb_number, True)
        if theta is not None:
            ws.mutableRun().addProperty(self._LOG_THETA, theta, True)
        return ws

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
