#pylint: disable=no-init,invalid-name
import stresstesting
from mantid.simpleapi import *

'''This test checks that the results of PoldiAutoCorrelation match the expected outcome.'''
class POLDITruncateDataTest(stresstesting.MantidStressTest):
    def runTest(self):
        self.dataFileName = "poldi2013n006903"

        self.loadDataFiles()
        self.workingAnalysis()
        self.workspaceAlreadyCorrect()
        self.workspaceTooSmall()

    def loadDataFiles(self,):
        LoadSINQFile(Instrument='POLDI',Filename=self.dataFileName + ".hdf",OutputWorkspace=self.dataFileName)
        LoadInstrument(Workspace=self.dataFileName, InstrumentName="POLDI")

    def workingAnalysis(self):
    # In this method the "normal behavior" is tested, if everything is
    # running as expected.
        currentWs = mtd[self.dataFileName]

    # Input data has 10 extra bins
        self.assertEqual(len(currentWs.readX(0)), 510)

    # First without keeping the additional data
        truncated = PoldiTruncateData(currentWs)

        self.assertEqual(truncated.getNumberHistograms(), currentWs.getNumberHistograms())
        self.assertEqual(len(truncated.readX(0)), 500)

    # now keeping the additional data
        truncated = PoldiTruncateData(currentWs, ExtraCountsWorkspaceName="extra")

        self.assertTrue(mtd.doesExist("extra"))

        extraWs = mtd['extra']

        self.assertEqual(extraWs.getNumberHistograms(), 1)
        extraCounts = extraWs.readY(0)
        self.assertEqual(len(extraCounts), 10)

    # there are 13 counts in the first bin
        self.assertEqual(extraCounts[0], 13.0)

    # and none in the others
        for y in extraCounts[1:]:
            self.assertEqual(y, 0.0)

    def workspaceAlreadyCorrect(self):
    # This method tests expected behavior if the workspace
    # already has the correct size
        currentWs = mtd[self.dataFileName]

        cropped = CropWorkspace(currentWs, XMax=1497.0)
        self.assertEqual(len(cropped.readX(0)), 500)

        truncated = PoldiTruncateData(cropped)
        self.assertEqual(len(truncated.readX(0)), len(cropped.readX(0)))

    # Now there are no extra bins.
        truncated = PoldiTruncateData(cropped, ExtraCountsWorkspaceName="moreCounts")

    # "extraCounts" should not be in the analysis data service
        self.assertTrue(not mtd.doesExist("moreCounts"))

    def workspaceTooSmall(self):
    # When the workspace is too small, the whole analysis fails.
    # This is reasonable since the timing information is then
    # very likely to be incorrect, so that the data file is not usable
        currentWs = mtd[self.dataFileName]

        cropped = CropWorkspace(currentWs, XMax=1197.0)
        self.assertEqual(len(cropped.readX(0)), 400)

        truncated = PoldiTruncateData(cropped)
        self.assertTrue(truncated is None)

        PoldiTruncateData(InputWorkspace=cropped, OutputWorkspace="NamedWorkspaceTest")
        self.assertTrue(not mtd.doesExist("NamedWorkspaceTest"))

