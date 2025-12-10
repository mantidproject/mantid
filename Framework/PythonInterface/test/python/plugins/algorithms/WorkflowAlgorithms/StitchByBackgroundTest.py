# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2025 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
import unittest

from mantid.simpleapi import StitchByBackground, CreateWorkspace, GroupWorkspaces
import numpy as np


class StitchByBackgroundTest(unittest.TestCase):
    ws_list = []

    @classmethod
    def setUpClass(cls):
        for i in range(5):
            ws_name = "ws_" + str(i + 1)
            data_x = np.arange(i, (i + 1) * 10 + 0.1, 0.1)
            data_y = np.arange(i, (i + 1) * 10, 0.1)
            data_e = np.arange(i, (i + 1) * 10, 0.1)
            CreateWorkspace(OutputWorkspace=ws_name, DataX=data_x, DataY=data_y, DataE=data_e)
            cls.ws_list.append(ws_name)
        GroupWorkspaces(InputWorkspaces=cls.ws_list, OutputWorkspace="ws_group")

    def test_overlap_width_must_be_positive(self):
        with self.assertRaisesRegex(ValueError, "-1 is < the lower bound"):
            StitchByBackground(InputWorkspaces="ws_group", OverlapWidth=-1)

    def test_stitch_and_ws_list_correctly_sized(self):
        with self.assertRaisesRegex(RuntimeError, r"There must be one less stitch point \(3\) than input workspaces \(5\)."):
            StitchByBackground(InputWorkspaces=self.ws_list, OverlapWidth=1, StitchPoints=[1.2, 2.3, 3.4], OutputWorkspace="out")


if __name__ == "__main__":
    unittest.main()
