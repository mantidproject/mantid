# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2023 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +

import unittest
from mantid.simpleapi import DeleteWorkspace, InterpolateBackground, CreateWorkspace, GroupWorkspaces


class InterpolateBackgroundTest(unittest.TestCase):
    def setUp(self):
        self.dataX = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
        self.dataY1 = [5, 10, 15, 20, 25, 30, 35, 40, 45, 50]
        self.dataY2 = [10, 20, 30, 40, 50, 60, 70, 80, 90, 100]
        self.ws1 = CreateWorkspace(dataX=self.dataX, dataY=self.dataY1, nspec=1, OutputWorkspace="ws1")
        self.ws2 = CreateWorkspace(dataX=self.dataX, dataY=self.dataY2, nspec=1, OutputWorkspace="ws2")
        self.wsGroup = GroupWorkspaces("ws1,ws2", OutputWorkspace="wsGroup")
        self.interpo = "300"

    def tearDown(self):
        DeleteWorkspace(self.wsGroup)

    def test_nominal(self):
        self.ws1.getRun().addProperty("SampleTemp", "100", False)
        self.ws2.getRun().addProperty("SampleTemp", "400", False)
        outputWS = InterpolateBackground(self.wsGroup, self.interpo)
        expected = [15, 30, 45, 60, 75, 90, 105, 120, 135, 150]
        self.assertListEqual(list(outputWS.readY(0)), expected)

    def test_bad_input(self):
        # Test raises Runtime error if a workspace is missing SampleTemp property
        with self.assertRaisesRegex(TypeError, "invalid Properties"):
            InterpolateBackground(self.wsGroup, self.interpo)
        self.ws2 = CreateWorkspace(dataX=self.dataX, dataY=self.dataY2, nspec=2, OutputWorkspace="ws2")
        self.ws1.getRun().addProperty("SampleTemp", "100", False)
        self.ws2.getRun().addProperty("SampleTemp", "400", False)
        # Test raises Runtime error if workspaces have a different number of bins
        with self.assertRaisesRegex(TypeError, "invalid Properties"):
            InterpolateBackground(self.wsGroup, self.interpo)


if __name__ == "__main__":
    unittest.main()
