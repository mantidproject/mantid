import stresstesting
from mantid.simpleapi import *

class LoadMuonNexusTest(stresstesting.MantidStressTest):

    def runTest(self):
      # EMU03087 is an old data file produced by CONVERT_NEXUS from MCS binary files.
      # Checked specifically because stores resulution (used to calculate FirstGoodData)
      # as NX_FLOAT32 opposed to NX_INT32 in other Muon files.
      loadResult = LoadMuonNexus(Filename = "EMU03087.nxs", 
                                 OutputWorkspace = "EMU03087")

      firstGoodData = loadResult[3]
      self.assertDelta(firstGoodData, 0.416, 0.0001)
      
    def cleanup(self):
      mtd.remove("EMU03087")
