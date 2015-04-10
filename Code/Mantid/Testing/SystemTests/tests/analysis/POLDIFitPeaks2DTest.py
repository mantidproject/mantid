#pylint: disable=no-init,invalid-name,too-many-locals
import stresstesting
from mantid.simpleapi import *
import numpy as np

class POLDIFitPeaks2DTest(stresstesting.MantidStressTest):
    """The system test currently checks that the calculation of 2D spectra
    works correctly."""

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
            LoadInstrument(Workspace="%s_2d_reference_Spectrum" % (dataFile), InstrumentName="POLDI")
            Load(Filename="%s_1d_reference_Spectrum.nxs" % (dataFile), OutputWorkspace="%s_1d_reference_Spectrum" % (dataFile))

    def runCalculateSpectrum2D(self, filenames):
        for dataFile in filenames:
            PoldiFitPeaks2D(InputWorkspace="%s_2d_reference_Spectrum" % (dataFile),
                               PoldiPeakWorkspace="%s_reference_Peaks" % (dataFile),
                               FitConstantBackground=False, FitLinearBackground=False,
                               RefinedPoldiPeakWorkspace="%s_refined_Peaks" % (dataFile),
                               OutputWorkspace="%s_2d_calculated_Spectrum" % (dataFile),
                               Calculated1DSpectrum="%s_1d_calculated_Spectrum" % (dataFile),
                               MaximumIterations=100)

    def analyseResults(self, filenames):
        for dataFile in filenames:
            calculatedSpectrum = mtd["%s_2d_calculated_Spectrum" % (dataFile)]
            referenceSpectrum = mtd["%s_2d_reference_Spectrum" % (dataFile)]

            referencePeaks = mtd["%s_reference_Peaks" % (dataFile)]
            fittedPeaks = mtd["%s_refined_Peaks" % (dataFile)]

            self.assertEqual(calculatedSpectrum.getNumberHistograms(), referenceSpectrum.getNumberHistograms())

            columns = ["d", "Intensity"]

            for i in range(referencePeaks.rowCount()):
                referenceRow = referencePeaks.row(i)
                fittedRow = fittedPeaks.row(i)
                for c in columns:
                    fittedStr = fittedRow[c].split()
                    value, error = (float(fittedStr[0]), float(fittedStr[-1]))
                    reference = float(referenceRow[c])

                    self.assertLessThan(np.fabs(value - reference), error)


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
                self.assertLessThan(maxDifference, 0.07)

class POLDIFitPeaks2DPawleyTest(stresstesting.MantidStressTest):

    def runTest(self):
        si = PoldiLoadRuns(2013, 6903, 6904, 2)
        corr = PoldiAutoCorrelation('si_data_6904')
        peaks = PoldiPeakSearch(corr)
        peaks_ref, fit_plots = PoldiFitPeaks1D(corr, PoldiPeakTable='peaks')
        si_refs = PoldiCreatePeaksFromCell("F d -3 m", "Si 0 0 0", a=5.431, LatticeSpacingMin=0.7)
        indexed = PoldiIndexKnownCompounds(peaks_ref, "si_refs")

        DeleteTableRows("indexed_si_refs", "10-30")

        fit2d, fit1d, peaks_ref_2d, cell = PoldiFitPeaks2D('si_data_6904', 'indexed_si_refs',
                                                           PawleyFit=True,
                                                           InitialCell="5.431 5.431 5.431 90 90 90",
                                                           CrystalSystem="Cubic",
                                                           MaximumIterations=100)

        cell_a = cell.cell(0, 1)
        cell_a_err = cell.cell(0, 2)

        self.assertLessThan(np.abs(cell_a_err), 5.0e-5)
        self.assertLessThan(np.abs(cell_a - 5.4311946) / cell_a_err, 1.5)

