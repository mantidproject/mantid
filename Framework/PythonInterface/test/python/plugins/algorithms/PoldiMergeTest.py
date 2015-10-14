import unittest

from mantid.kernel import *
from mantid.api import *
from mantid.simpleapi import *

import numpy as np


class PoldiMergeTest(unittest.TestCase):
    def __init__(self, *args):
        unittest.TestCase.__init__(self, *args)
        properties = ["TablePositionX", "TablePositionY", "TablePositionZ", "ChopperSpeed"]

        leftData = [0.0, 1.0, 2.0, 3.0]
        rightDataGood = [0.0, 1.0, 2.0, 3.0]

        rightDataBadOffset = [1.0, 2.0, 3.0, 4.0]
        rightDataBadDelta = [0.0, 2.0, 4.0, 6.0]

        ydata = np.ones(len(leftData))

        self.base = CreateWorkspace(leftData, ydata, OutputWorkspace="Base")
        self.goodTiming = CreateWorkspace(rightDataGood, ydata, OutputWorkspace="GoodTiming")
        self.goodTimingBadProperties = CreateWorkspace(rightDataGood, ydata, OutputWorkspace="GoodTimingBadProperties")
        self.badTimingOffset = CreateWorkspace(rightDataBadOffset, ydata, OutputWorkspace="BadTimingOffset")
        self.badTimingDelta = CreateWorkspace(rightDataBadDelta, ydata, OutputWorkspace="BadTimingDelta")

        self.groupGood = GroupWorkspaces(["Base", "GoodTiming"], OutputWorkspace="GoodGroup")

        goodProperty = 10.0
        badProperty = 20.0

        for p in properties:
            self.base.getRun().addProperty(p, goodProperty, True)
            self.goodTiming.getRun().addProperty(p, goodProperty, True)
            self.badTimingOffset.getRun().addProperty(p, goodProperty, True)
            self.badTimingDelta.getRun().addProperty(p, goodProperty, True)

            self.goodTimingBadProperties.getRun().addProperty(p, badProperty, True)

    def __runMerge__(self, workspaceNames, checkInstruments=False):
        return PoldiMerge(WorkspaceNames=workspaceNames, OutputWorkspace="PoldiMergeOutput",
                          CheckInstruments=checkInstruments)

    def test_happyCase(self):
        output = self.__runMerge__("Base,GoodTiming")

        self.assertTrue(isinstance(output, MatrixWorkspace))

        dataX = output.dataX(0)
        self.assertEqual(dataX[0], 0.0)
        self.assertEqual(dataX[-1], 3.0)
        self.assertEqual(len(dataX), 4)

        dataY = output.dataY(0)
        self.assertEqual(dataY[0], 2.0)
        self.assertEqual(dataY[1], 2.0)
        self.assertEqual(len(dataY), 4)

        DeleteWorkspace("PoldiMergeOutput")

    def test_workspaceGroup(self):
        output = self.__runMerge__("GoodGroup")

        self.assertTrue(isinstance(output, MatrixWorkspace))

        dataX = output.dataX(0)
        self.assertEqual(dataX[0], 0.0)
        self.assertEqual(dataX[-1], 3.0)
        self.assertEqual(len(dataX), 4)

        dataY = output.dataY(0)
        self.assertEqual(dataY[0], 2.0)
        self.assertEqual(dataY[1], 2.0)
        self.assertEqual(len(dataY), 4)

        DeleteWorkspace("PoldiMergeOutput")

    def test_timingDelta(self):
        self.assertRaises(RuntimeError, lambda: self.__runMerge__("Base,BadTimingDelta"))
        self.assertFalse(AnalysisDataService.doesExist("PoldiMergeOutput"))

    def test_timingOffset(self):
        self.assertRaises(RuntimeError, lambda: self.__runMerge__("Base,BadTimingOffset"))
        self.assertFalse(AnalysisDataService.doesExist("PoldiMergeOutput"))

    def test_badProperties(self):
        self.assertRaises(RuntimeError, lambda: self.__runMerge__("Base,GoodTimingBadProperties", True))
        self.assertFalse(AnalysisDataService.doesExist("PoldiMergeOutput"))

    def test_badName(self):
        self.assertRaises(RuntimeError, lambda: self.__runMerge__("Base,NotExisting"))
        self.assertFalse(AnalysisDataService.doesExist("PoldiMergeOutput"))


if __name__ == '__main__':
    unittest.main()