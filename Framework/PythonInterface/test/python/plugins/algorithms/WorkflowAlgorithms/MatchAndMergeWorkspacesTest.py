# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2019 ISIS Rutherford Appleton Laboratory UKRI,
#     NScD Oak Ridge National Laboratory, European Spallation Source
#     & Institut Laue - Langevin
# SPDX - License - Identifier: GPL - 3.0 +
from __future__ import (absolute_import, division, print_function)

import unittest
import numpy as np
from mantid.api import MatrixWorkspace
from mantid.simpleapi import (AnalysisDataService, ConjoinWorkspaces, CreateWorkspace, MatchAndMergeWorkspaces,
                              GroupWorkspaces, DeleteWorkspace)


class MatchAndMergeWorkspacesTest(unittest.TestCase):

    def setUp(self):
        ws_list = []
        for i in range(5):
            ws_name = 'ws_' + str(i+1)
            data_x = np.arange(i, (i+1)*10+0.1, 0.1)
            data_y = np.arange(i, (i+1)*10, 0.1)
            data_e = np.arange(i, (i+1)*10, 0.1)
            CreateWorkspace(OutputWorkspace=ws_name, DataX=data_x, DataY=data_y, DataE=data_e)
            ws_list.append(ws_name)
        GroupWorkspaces(InputWorkspaces=ws_list, OutputWorkspace='ws_group')

    def test_MatchAndMergeWorkspaces_executes(self):
        x_min = np.array([0, 5, 10, 15, 20])
        x_max = np.array([10, 20, 30, 40, 50])
        ws_merged = MatchAndMergeWorkspaces(InputWorkspaces='ws_group', XMin=x_min, XMax=x_max)
        self.assertIsInstance(ws_merged, MatrixWorkspace)
        self.assertEqual(ws_merged.getNumberHistograms(), 1)
        self.assertAlmostEqual(min(ws_merged.dataX(0)), 0, places=0)
        self.assertAlmostEqual(max(ws_merged.dataX(0)), 50, places=0)

    def test_MatchAndMergeWorkspaces_produces_correct_range(self):
        x_min = np.array([2, 5, 10, 15, 20])
        x_max = np.array([10, 20, 30, 40, 45])
        ws_merged = MatchAndMergeWorkspaces(InputWorkspaces='ws_group', XMin=x_min, XMax=x_max)
        self.assertIsInstance(ws_merged, MatrixWorkspace)
        self.assertEqual(ws_merged.getNumberHistograms(), 1)
        self.assertAlmostEqual(min(ws_merged.dataX(0)), 2, places=0)
        self.assertAlmostEqual(max(ws_merged.dataX(0)), 45, places=0)

    def test_MatchAndMergeWorkspaces_accepts_a_list_of_workspaces(self):
        x_min = np.array([2, 5, 10])
        x_max = np.array([10, 20, 30])
        ws_group = AnalysisDataService.retrieve('ws_group')
        ws_list = [ws_group[0], ws_group[1], ws_group[2]]
        ws_merged = MatchAndMergeWorkspaces(InputWorkspaces=ws_list, XMin=x_min, XMax=x_max)
        self.assertIsInstance(ws_merged, MatrixWorkspace)
        self.assertEqual(ws_merged.getNumberHistograms(), 1)
        self.assertAlmostEqual(min(ws_merged.dataX(0)), 2, places=0)
        self.assertAlmostEqual(max(ws_merged.dataX(0)), 30, places=0)

    def test_MatchAndMergeWorkspaces_accepts_a_mixture_of_ws_size(self):
        x_min = np.array([2, 5, 10, 15, 20])
        x_max = np.array([10, 20, 30, 40, 45])
        ws_group = AnalysisDataService.retrieve('ws_group')
        ConjoinWorkspaces(InputWorkspace1=ws_group[3],
                          InputWorkspace2=ws_group[4],
                          CheckOverlapping=False)
        ws_list = [ws_group[0], ws_group[1], ws_group[2], ws_group[3]]
        ws_merged = MatchAndMergeWorkspaces(InputWorkspaces=ws_list, XMin=x_min, XMax=x_max)
        self.assertIsInstance(ws_merged, MatrixWorkspace)
        self.assertEqual(ws_merged.getNumberHistograms(), 1)
        self.assertAlmostEqual(min(ws_merged.dataX(0)), 2, places=0)
        self.assertAlmostEqual(max(ws_merged.dataX(0)), 45, places=0)

    def test_MatchAndMergeWorkspaces_fails_with_wrong_number_min_limits(self):
        x_min = np.array([0])
        x_max = np.array([10, 20, 30, 40, 50])
        self.assertRaises(RuntimeError, MatchAndMergeWorkspaces, InputWorkspaces='ws_group', XMin=x_min, XMax=x_max)

    def test_MatchAndMergeWorkspaces_fails_with_wrong_number_max_limits(self):
        x_min = np.array([0, 5, 10, 15, 20])
        x_max = np.array([10])
        self.assertRaises(RuntimeError, MatchAndMergeWorkspaces, InputWorkspaces='ws_group', XMin=x_min, XMax=x_max)

    def test_MatchAndMergeWorkspaces_fails_with_wrong_ws_input(self):
        x_min = np.array([0, 5, 10, 15, 20])
        x_max = np.array([10, 20, 30, 40, 50])
        self.assertRaises(ValueError, MatchAndMergeWorkspaces, InputWorkspaces='fake_group', XMin=x_min, XMax=x_max)

    def test_MatchAndMergeWorkspaces_fails_with_min_larger_than_max(self):
        x_min = np.array([10, 20, 30, 40, 50])
        x_max = np.array([0, 5, 10, 15, 20])
        self.assertRaises(ValueError, MatchAndMergeWorkspaces, InputWorkspaces='fake_group', XMin=x_min, XMax=x_max)


if __name__ == "__main__":
    unittest.main()
