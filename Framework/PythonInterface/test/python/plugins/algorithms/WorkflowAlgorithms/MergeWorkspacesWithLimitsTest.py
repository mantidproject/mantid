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
from mantid.simpleapi import CreateWorkspace, MergeWorkspacesWithLimits, GroupWorkspaces, DeleteWorkspace


class MergeWorkspacesWithLimitsTest(unittest.TestCase):

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

    def test_MergeWorkspacesWithLimits_executes(self):
        x_min = np.array([0, 5, 10, 15, 20])
        x_max = np.array([10, 20, 30, 40, 50])
        ws_merged = MergeWorkspacesWithLimits(WorkspaceGroup='ws_group', XMin=x_min, XMax=x_max)
        self.assertIsInstance(ws_merged, MatrixWorkspace)
        self.assertEqual(ws_merged.getNumberHistograms(), 1)
        self.assertAlmostEqual(min(ws_merged.dataX(0)), 0, places=0)
        self.assertAlmostEqual(max(ws_merged.dataX(0)), 50, places=0)

    def test_MergeWorkspacesWithLimits_produces_correct_range(self):
        x_min = np.array([2, 5, 10, 15, 20])
        x_max = np.array([10, 20, 30, 40, 45])
        ws_merged = MergeWorkspacesWithLimits(WorkspaceGroup='ws_group', XMin=x_min, XMax=x_max)
        self.assertIsInstance(ws_merged, MatrixWorkspace)
        self.assertEqual(ws_merged.getNumberHistograms(), 1)
        self.assertAlmostEqual(min(ws_merged.dataX(0)), 2, places=0)
        self.assertAlmostEqual(max(ws_merged.dataX(0)), 45, places=0)


if __name__ == "__main__":
    unittest.main()
