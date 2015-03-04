import stresstesting
from mantid.simpleapi import *
import numpy as np

'''This test checks that the results of PoldiAutoCorrelation match the expected outcome.'''
class POLDIPeakSearchTest(stresstesting.MantidStressTest):  
  def runTest(self):
    dataFiles = ["poldi2013n006903", "poldi2013n006904"]
    
    self.loadReferenceCorrelationData(dataFiles)
    self.loadReferencePeakData(dataFiles)
    self.runPeakSearch(dataFiles)
    self.analyseResults(dataFiles)
      
  def loadReferenceCorrelationData(self, filenames):
    for dataFile in filenames:
      Load(Filename="%s_reference.nxs" % (dataFile), OutputWorkspace=dataFile)
      
  def loadReferencePeakData(self, filenames):
    for dataFile in filenames:
      Load(Filename="%s_reference_Peaks.nxs" % (dataFile), OutputWorkspace="%s_reference_Peaks" % (dataFile))

  def runPeakSearch(self, filenames):
    for dataFile in filenames:
      PoldiPeakSearch(InputWorkspace=dataFile, OutputWorkspace="%s_Peaks" % (dataFile))

  def analyseResults(self, filenames):
    for dataFile in filenames:
      calculatedPeaks = mtd["%s_Peaks" % (dataFile)]
      referencePeaks = mtd["%s_reference_Peaks" % (dataFile)]
      self.assertEqual(calculatedPeaks.rowCount(), referencePeaks.rowCount())
      
      positions = calculatedPeaks.column(2)
      referencePositions = referencePeaks.column(0)

      # In this test we only compare positions, because the height
      # and error estimates are derived differently than in the
      # original software, so the results are not exactly the same.
      #
      # Most important in this case are peak positions. Since the order
      # depends on height, it may be different, so the comparison can not
      # be done 1:1.
      for position in positions[:10]:
          deltas = [np.abs(float(position) - x) for x in referencePositions]

          self.assertDelta(min(deltas), 0.0, 1e-6)
