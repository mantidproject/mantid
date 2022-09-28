# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest
from unittest import mock

from mantidqtinterfaces.Muon.GUI.ElementalAnalysis.LoadWidget import load_utils as lutils

import mantid.simpleapi as mantid
from mantid import config
import numpy as np


class LoadUtilsTest(unittest.TestCase):
    def setUp(self):
        self.test_path = r"test\path\to\ral012345.rooth2020.dat"
        self.bad_path = r"test\path\to\ral012345.rooth2042"
        self.test_run = 5
        self.var_ws_name = "{}_Delayed_{}"
        self.test_ws_name = self.var_ws_name.format(1, self.test_run)
        self.test_ws_names = [self.var_ws_name.format(i, self.test_run) for i in range(1, 9)]

    def test_pad_run(self):
        tests = {123: "00123", 0: "00000", 12345: "12345", 123456: "123456"}
        for run, padded_run in tests.items():
            self.assertEqual(lutils.pad_run(run), padded_run)

    @mock.patch('mantidqtinterfaces.Muon.GUI.ElementalAnalysis.LoadWidget.load_utils.glob.iglob')
    @mock.patch('mantidqtinterfaces.Muon.GUI.ElementalAnalysis.LoadWidget.load_utils.os.path.join')
    def test_that_search_user_dirs_explores_all_directories(self, mock_join, mock_iglob):
        expected_len = len(config["datasearch.directories"].split(";"))
        lutils.search_user_dirs(self.test_run)

        self.assertEqual(mock_join.call_count, expected_len)

    def test_get_detector_num_from_ws(self):
        self.assertEqual(lutils.get_detector_num_from_ws(self.test_ws_name), "1")

    def test_get_detectors_num(self):
        self.assertEqual(lutils.get_detectors_num(self.test_path), "1")

    def test_get_detectors_num_throws_with_bad_input(self):
        input = ['only.one_separator', 'one.much_longer32123.three']
        for value in input:
            with self.assertRaises(Exception):
                lutils.get_detectors_num(value)

    def test_get_end_num(self):
        self.assertEqual(lutils.get_end_num(self.test_path), "rooth2020")

    def test_get_run_type(self):
        self.assertEqual(lutils.get_run_type(self.test_path), "Delayed")
        self.assertEqual(lutils.get_run_type('a.b10.c'), "Prompt")
        self.assertEqual(lutils.get_run_type('a.b99.c'), "Total")
        with self.assertRaises(KeyError):
            lutils.get_run_type(self.bad_path)

    def test_get_filename(self):
        self.assertEqual(lutils.get_filename(self.test_path, self.test_run), self.test_ws_name)

    def test_get_filename_returns_none_if_given_bad_path(self):
        self.assertEqual(lutils.get_filename(self.bad_path, self.test_run), None)

    def test_hyphenise(self):
        tests = {"1-5": [1, 2, 3, 4, 5], "1, 4-5": [1, 4, 5], "1-3, 5": [1, 3, 2, 5], "1, 3, 5": [1, 5, 3]}
        for out, arg in tests.items():
            self.assertEqual(lutils.hyphenise(arg), out)

    def test_merge_workspaces(self):
        expected_output, workspaces = [], []
        detectors = range(1, 5)
        for detector in detectors:
            workspace = self.var_ws_name.format(detector, self.test_run)
            workspaces.append(workspace)
            mantid.CreateSampleWorkspace(OutputWorkspace=workspace)
            expected_output.append("{}; Detector {}".format(self.test_run, detector))

        self.assertEqual(lutils.merge_workspaces(self.test_run, workspaces), sorted(expected_output))
        # check call works with empty workspace list
        self.assertEqual(lutils.merge_workspaces(self.test_run, []), sorted(expected_output))

    def test_create_merged_workspace(self):
        workspace_list = []
        num_workspaces = lutils.num_files_per_detector
        workspace_names = []
        num_bins = 100
        X_data = np.linspace(0, 400, num_bins)
        # create data in each workspace based on y = mx + specNumber
        m = 0.1

        def Yfunc(x, specNo):
            return m * x + specNo

        def Efunc(x, specNo):
            return 2 * m * x + specNo

        for i in range(0, num_workspaces):
            name = "test_" + str(i)
            workspace_names.append(name)
            ws = mantid.WorkspaceFactory.create("Workspace2D", NVectors=1,
                                                XLength=num_bins, YLength=num_bins)
            mantid.mtd.add(name, ws)
            Y_data = Yfunc(X_data, i)
            E_data = Efunc(X_data, i)
            ws.setY(0, Y_data)
            ws.setX(0, X_data)
            ws.setE(0, E_data)
            workspace_list.append(ws.name())

        merged_ws = lutils.create_merged_workspace(workspace_list)
        # check number of bins, and number of histograms is correct
        self.assertEqual(merged_ws.getNumberHistograms(), num_workspaces)
        self.assertEqual(merged_ws.blocksize(), num_bins)
        # check that data has been copied over correctly into the new merged workspace
        for i in range(0, num_workspaces):
            self.assertTrue(np.array_equal(merged_ws.readX(i), X_data))
            self.assertTrue(np.array_equal(merged_ws.readY(i), Yfunc(X_data, i)))
            self.assertTrue(np.array_equal(merged_ws.readE(i), Efunc(X_data, i)))

    def test_merge_workspaces_returns_correctly_if_delayed_data_missing(self):
        num_files_per_detector = lutils.num_files_per_detector
        workspace_list = [None] * num_files_per_detector
        num_bins = 100
        X_data = np.linspace(0, 400, num_bins)
        names = ["Total", "Prompt"]
        # create data in each workspace based on y = mx + specNumber

        def Yfunc(x, specNo):
            return 0.1 * x + specNo

        def Efunc(x, specNo):
            return 2 * 0.1 * x + specNo

        for i in range(0, 2):
            name = names[i]
            ws = mantid.WorkspaceFactory.create("Workspace2D", NVectors=1,
                                                XLength=num_bins, YLength=num_bins)
            mantid.mtd.add(name, ws)
            Y_data = Yfunc(X_data, i)
            E_data = Efunc(X_data, i)
            ws.setY(0, Y_data)
            ws.setX(0, X_data)
            ws.setE(0, E_data)
            input_index = lutils.spectrum_index[name] - 1
            workspace_list[input_index] = name

        merged_ws = lutils.create_merged_workspace(workspace_list)
        self.assertEqual(merged_ws.getNumberHistograms(), num_files_per_detector)
        # check that total and prompt data were copied to the correct place
        for i in range(0, 2):
            name = names[i]
            input_index = lutils.spectrum_index[name] - 1
            self.assertTrue(np.array_equal(merged_ws.readX(input_index), X_data))
            self.assertTrue(np.array_equal(merged_ws.readY(input_index), Yfunc(X_data, i)))
            self.assertTrue(np.array_equal(merged_ws.readE(input_index), Efunc(X_data, i)))
        # check that the y data for delayed response is all zeros
        self.assertTrue(not np.any(merged_ws.readY(lutils.spectrum_index["Delayed"] - 1)))

    def test_flatten_run_data(self):
        test_1 = ["1_det_1", "1_det_2"]
        test_2 = ["2_det_1", "2_det_2"]
        self.assertEqual(lutils.flatten_run_data(test_1, test_2), [test_1, test_2])

    def test_replace_workspace_name_suffix(self):
        tests = {self.test_ws_name: "suffix", "_".join([self.test_ws_name, "test"]): "suffix"}

        for workspace_name, suffix in tests.items():
            self.assertEqual(lutils.replace_workspace_name_suffix(workspace_name, suffix),
                              self.var_ws_name.format(1, suffix))


if __name__ == "__main__":
    unittest.main()
