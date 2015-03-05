import stresstesting
from mantid.simpleapi import *
import numpy as np

'''The system test currently checks that the calculation of 2D spectra
works correctly.'''
class POLDIFitPeaks2DTest(stresstesting.MantidStressTest):
  def runTest(self):
    dataFiles = ["poldi2013n006904"]

    self.loadAndPrepareData(dataFiles)
    self.loadReferencePeakData(dataFiles)
    self.loadReferenceSpectrum(dataFiles)
    self.runCalculateSpectrum2D(dataFiles)
    self.analyseResults(dataFiles)

  def loadAndPrepareData(self, filenames):
    for dataFile in filenames:
      LoadSINQFile(Instrument='POLDI',Filename=dataFile + ".hdf",OutputWorkspace=dataFile)
      LoadInstrument(Workspace=dataFile, InstrumentName="POLDI", RewriteSpectraMap=True)
      PoldiTruncateData(InputWorkspace=dataFile, OutputWorkspace=dataFile)

  def loadReferencePeakData(self, filenames):
    for dataFile in filenames:
      Load(Filename="%s_2d_reference_Peaks.nxs" % (dataFile), OutputWorkspace="%s_reference_Peaks" % (dataFile))

  def loadReferenceSpectrum(self, filenames):
    for dataFile in filenames:
      Load(Filename="%s_2d_reference_Spectrum.nxs" % (dataFile), OutputWorkspace="%s_2d_reference_Spectrum" % (dataFile))
      Load(Filename="%s_1d_reference_Spectrum.nxs" % (dataFile), OutputWorkspace="%s_1d_reference_Spectrum" % (dataFile))

  def runCalculateSpectrum2D(self, filenames):
    for dataFile in filenames:
      PoldiFitPeaks2D(InputWorkspace=dataFile,
                               PoldiPeakWorkspace="%s_reference_Peaks" % (dataFile),
                               PeakProfileFunction="Gaussian",
                               RefinedPoldiPeakWorkspace="%s_refined_Peaks" % (dataFile),
                               OutputWorkspace="%s_2d_calculated_Spectrum" % (dataFile),
                               Calculated1DSpectrum="%s_1d_calculated_Spectrum" % (dataFile),
                               MaximumIterations=0)

  def analyseResults(self, filenames):
    for dataFile in filenames:
      calculatedSpectrum = mtd["%s_2d_calculated_Spectrum" % (dataFile)]
      referenceSpectrum = mtd["%s_2d_reference_Spectrum" % (dataFile)]

      self.assertEqual(calculatedSpectrum.getNumberHistograms(), referenceSpectrum.getNumberHistograms())

      for i in range(calculatedSpectrum.getNumberHistograms()):
        calHisto = calculatedSpectrum.readY(i)

        if not referenceSpectrum.getDetector(i).isMasked():
          refHisto = referenceSpectrum.readY(i)

          absDiff = np.fabs(refHisto - calHisto)
          self.assertTrue(np.all(absDiff < 7e-4))
        else:
          self.assertTrue(np.all(calHisto == 0.0))

      spectra1D = ["%s_1d_%s_Spectrum"]

      for wsName in spectra1D:
        calculatedSpectrum1D = mtd[wsName % (dataFile, "calculated")]
        referenceSpectrum1D = mtd[wsName % (dataFile, "reference")]

        xDataCalc = calculatedSpectrum1D.readX(0)
        yDataCalc = calculatedSpectrum1D.readY(0)

        xDataRef = referenceSpectrum1D.readX(0)
        yDataRef = referenceSpectrum1D.readY(0)

        indices = np.nonzero(yDataRef)
        maxDifference = np.abs(np.max((yDataCalc[indices] - yDataRef[indices]) / yDataCalc[indices]))

        self.assertTrue(np.all(xDataCalc == xDataRef))
        self.assertLessThan(maxDifference, 0.0031)
