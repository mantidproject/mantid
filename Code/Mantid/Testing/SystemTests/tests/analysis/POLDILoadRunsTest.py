# pylint: disable=no-init,invalid-name
import stresstesting
from mantid.simpleapi import *
from mantid.api import *
import numpy as np

'''This assembly of test cases checks that the behavior of PoldiLoadRuns is correct.'''
class POLDILoadRunsTest(stresstesting.MantidStressTest):
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

        twoWorkspacesMerged = PoldiLoadRuns(2013, 6903, 6904, 2)

        wsMergedReversed = AnalysisDataService.retrieve("twoWorkspacesMergedReversed_data_6904")
        wsMerged = AnalysisDataService.retrieve("twoWorkspacesMerged_data_6904")

        self.compareWorkspaces(wsMergedReversed, wsMerged)

        self.clearAnalysisDataService()

    def loadWorkspacesMergeThreeNotWorking(self):
        try:
            threeWorkspacesFail = PoldiLoadRuns(2013, 6903, 6904, 3)
            self.assertTrue(False)
        except:
            self.assertTrue(True)

    def loadWorkspacesNotFound(self):
        try:
            notFound = PoldiLoadRuns(1990, 6903)
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

    def compareWorkspaces(self, left, right):
        for i in range(left.getNumberHistograms()):
            self.assertTrue(np.array_equal(left.dataY(i), right.dataY(i)))

    def clearAnalysisDataService(self):
        AnalysisDataService.clear()