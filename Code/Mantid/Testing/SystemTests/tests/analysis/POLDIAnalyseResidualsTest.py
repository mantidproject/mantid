import stresstesting
from mantid.simpleapi import *
import numpy as np

'''This test checks that the residual analysis algorithm for POLDI works correctly.'''
class POLDIAnalyseResidualsTest(stresstesting.MantidStressTest):
  def runTest(self):
    dataFiles = ["poldi2014n019874"]

    self.loadReferenceData(dataFiles)
    self.runResidualAnalysis(dataFiles)
    self.analyseResults(dataFiles)

  def loadReferenceData(self, filenames):
    for dataFile in filenames:
      Load(Filename="%s_fortran_fit.nxs" % (dataFile), OutputWorkspace="%s_fortran_fit" % (dataFile))
      Load(Filename="%s_fortran_residuals.nxs" % (dataFile), OutputWorkspace="%s_fortran_residuals" % (dataFile))

  def runResidualAnalysis(self, filenames):
    for dataFile in filenames:
      LoadSINQFile(Instrument='POLDI',Filename=dataFile + ".hdf",OutputWorkspace=dataFile)
      LoadInstrument(Workspace=dataFile, InstrumentName="POLDI", RewriteSpectraMap=True)
      PoldiTruncateData(InputWorkspace=dataFile,OutputWorkspace=dataFile)
      PoldiAnalyseResiduals(MeasuredCountData=dataFile, FittedCountData="%s_fortran_fit" % (dataFile), MaxIterations=1, OutputWorkspace=dataFile + "Residuals")

  def analyseResults(self, filenames):
    for dataFile in filenames:
      workspaceNameTemplate = "Comparison_%s" % (dataFile)

      referenceData = mtd["%s_fortran_residuals" % (dataFile)].dataY(0)
      calculatedData = mtd["%sResiduals" % (dataFile)].dataY(0)

      self.assertEqual(calculatedData.shape[0], referenceData.shape[0], "Number of d-values does not match for %s (is: %i, should: %i)" % (dataFile, calculatedData.shape[0], referenceData.shape[0]))

      CreateWorkspace(referenceData, calculatedData, OutputWorkspace=workspaceNameTemplate)

      fitNameTemplate = "Fit_%s" % (dataFile)
      Fit("name=LinearBackground", mtd[workspaceNameTemplate], StartX=np.min(referenceData), EndX=np.max(referenceData), Output=fitNameTemplate)

      fitResult = mtd[fitNameTemplate + "_Parameters"]

      slope = fitResult.cell(1, 1)
      self.assertDelta(slope, 1.0, 1e-2, "Slope is larger than 1.0 for %s (is: %d)" % (dataFile, slope))

      relativeSlopeError = fitResult.cell(1, 2) / slope
      self.assertLessThan(relativeSlopeError, 5e-3, "Relative error of slope is too large for %s (is: %d)" % (dataFile, relativeSlopeError))

      intercept = fitResult.cell(0, 1)
      self.assertDelta(intercept, 0.0, 1e-3, "Intercept deviates too far from 0 %s (is: %d)" % (dataFile, intercept))

      residuals = mtd[fitNameTemplate + "_Workspace"].dataY(2)
      maxAbsoluteResidual = np.max(np.abs(residuals))
      self.assertLessThan(maxAbsoluteResidual, 1.0, "Maximum absolute residual is too large for %s (is: %d)" % (dataFile, maxAbsoluteResidual))
