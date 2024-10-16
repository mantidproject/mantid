# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init
import systemtesting
from mantid.api import mtd
from mantid.simpleapi import GroupWorkspaces, Load, LoadInstrument, LoadSINQFile, PoldiMerge
import numpy as np


class POLDIMergeTest(systemtesting.MantidSystemTest):
    """This test checks that the results of PoldiMerge match the expected outcome."""

    def runTest(self):
        self.testHappyCase()
        self.testDifferentTimings()

    def testDifferentTimings(self):
        dataFiles = ["poldi2014n019874", "poldi2014n019881"]
        self.loadData(dataFiles)

        try:
            self.runPoldiMerge(dataFiles, "Dummy")
            self.fail()
        except RuntimeError:
            pass

    def testHappyCase(self):
        dataFiles = ["poldi2013n006903", "poldi2013n006904"]
        sumWorkspace = "poldi_sum_6903_6904"

        self.loadData(dataFiles)
        self.runPoldiMerge(dataFiles, sumWorkspace)

        self.loadReferenceData(sumWorkspace)
        self.analyseResults(sumWorkspace)

        sumWorkspaceGroup = GroupWorkspaces(dataFiles)
        workspaceGroupResult = self.testGroupWorkspace(sumWorkspaceGroup)

        # compare result of workspace group merging to previously checked results
        self.compareWorkspaces(workspaceGroupResult, mtd["poldi_sum_6903_6904"])

    def testGroupWorkspace(self, groupWorkspace):
        return PoldiMerge(groupWorkspace)

    def loadData(self, filenames):
        for dataFile in filenames:
            LoadSINQFile(Instrument="POLDI", Filename=dataFile + ".hdf", OutputWorkspace=dataFile)
            LoadInstrument(Workspace=dataFile, InstrumentName="POLDI", RewriteSpectraMap=True)

    def runPoldiMerge(self, workspaceNames, outputWorkspaceName):
        PoldiMerge(WorkspaceNames=workspaceNames, OutputWorkspace=outputWorkspaceName)

    def loadReferenceData(self, outputWorkspaceName):
        Load(Filename=outputWorkspaceName + "_reference.nxs", OutputWorkspace=outputWorkspaceName + "_reference")

    def analyseResults(self, outputWorkspaceName):
        for i in range(mtd[outputWorkspaceName + "_reference"].getNumberHistograms()):
            # reference spectrum is still in the "original order", so for one of the workspaces, the index has to be reversed.
            self.assertTrue(np.array_equal(mtd[outputWorkspaceName].dataY(i), mtd[outputWorkspaceName + "_reference"].dataY(399 - i)))

    def compareWorkspaces(self, left, right):
        for i in range(left.getNumberHistograms()):
            self.assertTrue(np.array_equal(left.dataY(i), right.dataY(i)))
