# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-many-locals,too-few-public-methods
import systemtesting
from mantid.api import mtd
from mantid.simpleapi import (
    DeleteTableRows,
    DeleteWorkspace,
    Load,
    LoadInstrument,
    LoadSINQFile,
    PoldiAutoCorrelation,
    PoldiCreatePeaksFromCell,
    PoldiFitPeaks1D,
    PoldiIndexKnownCompounds,
    PoldiFitPeaks2D,
    PoldiLoadRuns,
    PoldiPeakSearch,
    PoldiTruncateData,
)
import numpy as np


class POLDIFitPeaks2DTest(systemtesting.MantidSystemTest):
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
            LoadSINQFile(Instrument="POLDI", Filename=dataFile + ".hdf", OutputWorkspace=dataFile)
            LoadInstrument(Workspace=dataFile, InstrumentName="POLDI", RewriteSpectraMap=True)
            PoldiTruncateData(InputWorkspace=dataFile, OutputWorkspace=dataFile)

    def loadReferencePeakData(self, filenames):
        for dataFile in filenames:
            Load(Filename="%s_2d_reference_Peaks.nxs" % (dataFile), OutputWorkspace="%s_reference_Peaks" % (dataFile))

    def loadReferenceSpectrum(self, filenames):
        for dataFile in filenames:
            Load(Filename="%s_2d_reference_Spectrum.nxs" % (dataFile), OutputWorkspace="%s_2d_reference_Spectrum" % (dataFile))
            LoadInstrument(Workspace="%s_2d_reference_Spectrum" % (dataFile), InstrumentName="POLDI", RewriteSpectraMap=True)
            Load(Filename="%s_1d_reference_Spectrum.nxs" % (dataFile), OutputWorkspace="%s_1d_reference_Spectrum" % (dataFile))

    def runCalculateSpectrum2D(self, filenames):
        for dataFile in filenames:
            PoldiFitPeaks2D(
                InputWorkspace="%s_2d_reference_Spectrum" % (dataFile),
                PoldiPeakWorkspace="%s_reference_Peaks" % (dataFile),
                FitConstantBackground=False,
                FitLinearBackground=False,
                RefinedPoldiPeakWorkspace="%s_refined_Peaks" % (dataFile),
                OutputWorkspace="%s_2d_calculated_Spectrum" % (dataFile),
                Calculated1DSpectrum="%s_1d_calculated_Spectrum" % (dataFile),
                MaximumIterations=100,
            )

    def analyseResults(self, filenames):
        for dataFile in filenames:
            calculatedSpectrum = mtd["%s_2d_calculated_Spectrum" % (dataFile)]
            referenceSpectrum = mtd["%s_2d_reference_Spectrum" % (dataFile)]

            referencePeaks = mtd["%s_reference_Peaks" % (dataFile)]
            fittedPeaks = mtd["%s_refined_Peaks" % (dataFile)]

            self.assertEqual(calculatedSpectrum.getNumberHistograms(), referenceSpectrum.getNumberHistograms())

            columns = ["d", "Intensity"]

            print(fittedPeaks.rowCount(), referencePeaks.rowCount())

            for i in range(referencePeaks.rowCount()):
                referenceRow = referencePeaks.row(i)
                fittedRow = fittedPeaks.row(i)
                for c in columns:
                    value = fittedRow[c]
                    error = fittedRow["delta " + c]
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


class POLDIFitPeaks2DPawleyTest(systemtesting.MantidSystemTest):
    def runTest(self):
        si = PoldiLoadRuns(2013, 6903, 6904, 2)
        corr = PoldiAutoCorrelation("si_data_6904")
        peaks = PoldiPeakSearch(corr)
        peaks_ref, fit_plots = PoldiFitPeaks1D(corr, PoldiPeakTable="peaks")
        si_refs = PoldiCreatePeaksFromCell("F d -3 m", "Si 0 0 0", a=5.431, LatticeSpacingMin=0.7)
        indexed = PoldiIndexKnownCompounds(peaks_ref, "si_refs")

        DeleteTableRows("peaks_ref_indexed_si_refs", "8-30")

        fit2d, fit1d, peaks_ref_2d, cell = PoldiFitPeaks2D(
            "si_data_6904", "peaks_ref_indexed_si_refs", PawleyFit=True, MaximumIterations=100
        )

        # parameters a and ZeroShift
        self.assertEqual(cell.rowCount(), 2)

        cell_a = cell.cell(0, 1)
        cell_a_err = cell.cell(0, 2)

        self.assertLessThan(np.abs(cell_a_err), 5.0e-5)
        self.assertLessThan(np.abs(cell_a - 5.4311946) / cell_a_err, 2.0)

        DeleteWorkspace(si_refs)
        DeleteWorkspace(indexed)
        DeleteWorkspace(peaks)
        DeleteWorkspace(si)
        DeleteWorkspace(fit2d)
        DeleteWorkspace(fit1d)
        DeleteWorkspace(fit_plots)
        DeleteWorkspace(peaks_ref_2d)


class POLDIFitPeaks2DIntegratedIntensities(systemtesting.MantidSystemTest):
    def runTest(self):
        si = PoldiLoadRuns(2013, 6903, 6904, 2)
        corr = PoldiAutoCorrelation("si_data_6904")
        peaks = PoldiPeakSearch(corr, MaximumPeakNumber=8)
        peaks_ref, fit_plots = PoldiFitPeaks1D(corr, PoldiPeakTable="peaks")

        # Run the same analysis twice, once with integrated and once with maximum intensities
        # Since a Gaussian is used, the integration can be checked numerically.
        fit2d, fit1d, peaks_ref_2d = PoldiFitPeaks2D("si_data_6904", peaks_ref, OutputIntegratedIntensities=False, MaximumIterations=100)

        fit2d, fit1d, peaks_ref_2d_integrated = PoldiFitPeaks2D(
            "si_data_6904", peaks_ref, OutputIntegratedIntensities=True, MaximumIterations=100
        )

        self.assertEqual(peaks_ref_2d.rowCount(), peaks_ref_2d_integrated.rowCount())

        for i in range(peaks_ref_2d.rowCount()):
            rowHeight = peaks_ref_2d.row(i)

            sigmaGaussian = (rowHeight["FWHM (rel.)"] * rowHeight["d"]) / (2.0 * np.sqrt(2.0 * np.log(2.0)))
            integratedGaussian = rowHeight["Intensity"] * np.sqrt(np.pi * 2.0) * sigmaGaussian

            rowIntegrated = peaks_ref_2d_integrated.row(i)

            # The numerical peak integration is done with a precision of 1e-10
            self.assertDelta(integratedGaussian, rowIntegrated["Intensity"], 1e-10)

        DeleteWorkspace(fit2d)
        DeleteWorkspace(fit1d)
        DeleteWorkspace(si)
        DeleteWorkspace(peaks)
        DeleteWorkspace(fit_plots)
