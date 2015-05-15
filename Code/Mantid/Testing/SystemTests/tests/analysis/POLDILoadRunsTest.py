# pylint: disable=no-init,invalid-name,bare-except
import stresstesting
from mantid.simpleapi import *
from mantid.api import *
import numpy as np


class POLDILoadRunsTest(stresstesting.MantidStressTest):
    """This assembly of test cases checks that the behavior of PoldiLoadRuns is correct."""

    def runTest(self):
        self.loadSingleWorkspace()
        self.loadMultipleSingleWorkspaces()
        self.loadWorkspacesMergeTwo()
        self.loadWorkspacesMergeTwoReverse()
        self.loadWorkspacesNotFound()

        self.loadWorkspacesAddToGroup()
        self.loadWorkspacesOverwriteGroup()
        self.loadWorkspacesDontOverwriteOther()
        self.loadWorkspacesOverwriteOther()

        self.checkRemoveBadDetectors()
        self.check2015PoldiData()

    def loadSingleWorkspace(self):
        singleWs = PoldiLoadRuns(2013, 6904)

        self.assertTrue(issubclass(type(singleWs), WorkspaceGroup))
        self.assertTrue(singleWs.contains("singleWs_data_6904"))

        self.clearAnalysisDataService()

    def loadMultipleSingleWorkspaces(self):
        multipleSingleWs = PoldiLoadRuns(2013, 6903, 6904)

        self.assertTrue(issubclass(type(multipleSingleWs), WorkspaceGroup))
        self.assertEquals(len(multipleSingleWs.getNames()), 2)

        self.clearAnalysisDataService()

    def loadWorkspacesMergeTwo(self):
        twoWorkspacesMerged = PoldiLoadRuns(2013, 6903, 6904, 2)

        self.assertTrue(issubclass(type(twoWorkspacesMerged), WorkspaceGroup))

        wsNames = twoWorkspacesMerged.getNames()
        self.assertEquals(len(wsNames), 1)
        self.assertEquals(wsNames[0], "twoWorkspacesMerged_data_6904")

        self.clearAnalysisDataService()

    def loadWorkspacesMergeTwoReverse(self):
        twoWorkspacesMergedReversed = PoldiLoadRuns(2013, 6904, 6903, 2)

        self.assertTrue(issubclass(type(twoWorkspacesMergedReversed), WorkspaceGroup))

        wsNames = twoWorkspacesMergedReversed.getNames()
        self.assertEquals(len(wsNames), 1)
        self.assertEquals(wsNames[0], "twoWorkspacesMergedReversed_data_6904")

        PoldiLoadRuns(2013, 6903, 6904, 2, OutputWorkspace="twoWorkspacesMerged")

        wsMergedReversed = AnalysisDataService.retrieve("twoWorkspacesMergedReversed_data_6904")
        wsMerged = AnalysisDataService.retrieve("twoWorkspacesMerged_data_6904")

        self.compareWorkspaces(wsMergedReversed, wsMerged)

        self.clearAnalysisDataService()

    def loadWorkspacesMergeThreeNotWorking(self):
        try:
            PoldiLoadRuns(2013, 6903, 6904, 3, OutputWorkspace="threeWorkspacesFail")
            self.assertTrue(False)
        except:
            self.assertTrue(True)

    def loadWorkspacesNotFound(self):
        try:
            PoldiLoadRuns(1990, 6903, OutputWorkspace="notFound")
            self.assertTrue(False)
        except:
            self.assertTrue(True)

    def loadWorkspacesAddToGroup(self):
        wsGroup = PoldiLoadRuns(2013, 6903)

        wsNames = wsGroup.getNames()
        self.assertEquals(len(wsNames), 1)
        self.assertEquals(wsNames[0], "wsGroup_data_6903")

        wsGroup = PoldiLoadRuns(2013, 6904, OverwriteExistingWorkspace=False)

        wsNames = wsGroup.getNames()
        self.assertEquals(len(wsNames), 2)
        self.assertEquals(wsNames[0], "wsGroup_data_6903")
        self.assertEquals(wsNames[1], "wsGroup_data_6904")

        self.clearAnalysisDataService()

    def loadWorkspacesOverwriteGroup(self):
        wsGroup = PoldiLoadRuns(2013, 6903)

        wsNames = wsGroup.getNames()
        self.assertEquals(len(wsNames), 1)
        self.assertEquals(wsNames[0], "wsGroup_data_6903")

        wsGroup = PoldiLoadRuns(2013, 6904, OverwriteExistingWorkspace=True)

        wsNames = wsGroup.getNames()
        self.assertEquals(len(wsNames), 1)
        self.assertEquals(wsNames[0], "wsGroup_data_6904")

    def loadWorkspacesOverwriteOther(self):
        otherWs = CreateWorkspace(1.0, 1.0)

        self.assertTrue(issubclass(type(otherWs), Workspace))

        otherWs = PoldiLoadRuns(2013, 6904, OverwriteExistingWorkspace=True)

        self.assertTrue(issubclass(type(otherWs), WorkspaceGroup))
        wsNames = otherWs.getNames()
        self.assertEquals(len(wsNames), 1)
        self.assertEquals(wsNames[0], "otherWs_data_6904")

    def loadWorkspacesDontOverwriteOther(self):
        otherWs = CreateWorkspace(1.0, 1.0)

        self.assertTrue(issubclass(type(otherWs), Workspace))

        otherWs = PoldiLoadRuns(2013, 6904, OverwriteExistingWorkspace=False)

        self.assertTrue(issubclass(type(otherWs), Workspace))

    def checkRemoveBadDetectors(self):
        # Determine bad detectors automatically
        PoldiLoadRuns(2013, 6903, 6904, 2, MaskBadDetectors=True,
                                            BadDetectorThreshold=2.5,
                                            OutputWorkspace='twoWorkspacesMerged')

        wsMerged = AnalysisDataService.retrieve("twoWorkspacesMerged_data_6904")
        self.assertEquals(len([True for x in range(wsMerged.getNumberHistograms()) if wsMerged.getDetector(
            x).isMasked()]), 36)

        self.clearAnalysisDataService()

        # Lower threshold, more excluded detectors
        PoldiLoadRuns(2013, 6903, 6904, 2, MaskBadDetectors=True,
                                            BadDetectorThreshold=2.0,
                                            OutputWorkspace='twoWorkspacesMerged')

        wsMerged = AnalysisDataService.retrieve("twoWorkspacesMerged_data_6904")
        self.assertEquals(len([True for x in range(wsMerged.getNumberHistograms()) if wsMerged.getDetector(
            x).isMasked()]), 49)

        self.clearAnalysisDataService()

        # Only use those from the IDF
        PoldiLoadRuns(2013, 6903, 6904, 2, MaskBadDetectors=False,
                                            OutputWorkspace='twoWorkspacesMerged')

        wsMerged = AnalysisDataService.retrieve("twoWorkspacesMerged_data_6904")
        self.assertEquals(len([True for x in range(wsMerged.getNumberHistograms()) if wsMerged.getDetector(
            x).isMasked()]), 12)

        self.clearAnalysisDataService()

    def check2015PoldiData(self):
        PoldiLoadRuns(2015, 977, OutputWorkspace='ws')

        ws2015 = AnalysisDataService.retrieve('ws_data_977')
        self.assertEquals(ws2015.getNumberHistograms(), 400)
        self.assertEquals(len(ws2015.readX(0)), 125)
        self.assertTrue(ws2015.run().hasProperty('chopperspeed'))

        self.clearAnalysisDataService()



    def compareWorkspaces(self, left, right):
        for i in range(left.getNumberHistograms()):
            self.assertTrue(np.array_equal(left.dataY(i), right.dataY(i)))

    def clearAnalysisDataService(self):
        AnalysisDataService.clear()
