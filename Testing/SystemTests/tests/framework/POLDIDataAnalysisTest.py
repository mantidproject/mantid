# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2018 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
# pylint: disable=no-init,invalid-name,too-many-locals,too-few-public-methods
import systemtesting
from mantid.api import AnalysisDataService, WorkspaceGroup
from mantid.simpleapi import DeleteTableRows, PoldiCreatePeaksFromCell, PoldiDataAnalysis, PoldiLoadRuns
import numpy as np


class POLDIDataAnalysisTestSi(systemtesting.MantidSystemTest):
    """Base class that loads/generates data for the tests, which is identical."""

    def runTest(self):
        pass

    def prepareTest(self):
        self._loadData()
        self._createSi()

        return "poldi_data_6904", "Si"

    def _loadData(self):
        PoldiLoadRuns(2013, 6903, 6904, 2, OutputWorkspace="poldi", MaskBadDetectors=False)

    def _createSi(self):
        PoldiCreatePeaksFromCell(SpaceGroup="F d -3 m", a=5.431, LatticeSpacingMin=0.7, Atoms="Si 0 0 0 1.0 0.01", OutputWorkspace="Si")


class POLDIDataAnalysisTestSiIndividual(POLDIDataAnalysisTestSi):
    """This test runs PoldiDataAnalysis with Si data, using individual peaks."""

    def runTest(self):
        data, expectedPeaks = self.prepareTest()

        output = PoldiDataAnalysis(InputWorkspace=data, MaximumPeakNumber=11, ExpectedPeaks=expectedPeaks, PlotResult=False)

        # Make sure that it's a workspace group
        self.assertTrue(isinstance(output, WorkspaceGroup))

        # check the refined peaks.
        refinedPeaks = AnalysisDataService.retrieve("poldi_data_6904_peaks_refined_2d")
        self.assertEqual(refinedPeaks.rowCount(), 11)

        # make sure there are no unindexed peaks
        self.assertFalse(AnalysisDataService.doesExist("poldi_data_6904_peaks_refined_1d_unindexed"))

        # get total spectrum
        totalSpectrum = AnalysisDataService.retrieve("poldi_data_6904_sum")
        # get residuals
        residuals = AnalysisDataService.retrieve("poldi_data_6904_residuals")

        sumData = totalSpectrum.dataY(0)
        residualData = residuals.dataY(0)

        maxSum = np.max(sumData)
        maxResidual = np.max(residualData)

        # Maximum residual should not be too large
        self.assertLessThan(maxResidual / maxSum, 0.075)


class POLDIDataAnalysisTestSiIndividualDiscardUnindexed(POLDIDataAnalysisTestSi):
    def runTest(self):
        data, expectedPeaks = self.prepareTest()
        DeleteTableRows(expectedPeaks, "8-20")

        output_remove = PoldiDataAnalysis(
            InputWorkspace=data, MaximumPeakNumber=11, ExpectedPeaks=expectedPeaks, RemoveUnindexedPeaksFor2DFit=True, PlotResult=False
        )

        # Make sure that it's a workspace group
        self.assertTrue(isinstance(output_remove, WorkspaceGroup))

        # check that only one set of peaks has been refined
        refinedPeaks = AnalysisDataService.retrieve("poldi_data_6904_peaks_refined_2d")
        self.assertEqual(refinedPeaks.rowCount(), 8)

        # Run again with option to keep unindexed peaks.
        output_keep = PoldiDataAnalysis(
            InputWorkspace=data, MaximumPeakNumber=11, ExpectedPeaks=expectedPeaks, RemoveUnindexedPeaksFor2DFit=False, PlotResult=False
        )

        # Make sure that it's a workspace group
        self.assertTrue(isinstance(output_keep, WorkspaceGroup))

        # check that the output peaks are again a workspace group (Si and unindexed)
        refinedPeaks = AnalysisDataService.retrieve("poldi_data_6904_peaks_refined_2d")
        self.assertTrue(isinstance(refinedPeaks, WorkspaceGroup))


class POLDIDataAnalysisTestSiIndividualPseudoVoigtTied(POLDIDataAnalysisTestSi):
    """This test runs PoldiDataAnalysis with Si data, using PseudoVoigt with tied mixing parameter."""

    def runTest(self):
        data, expectedPeaks = self.prepareTest()

        output = PoldiDataAnalysis(
            InputWorkspace=data,
            MaximumPeakNumber=11,
            ProfileFunction="PseudoVoigt",
            TieProfileParameters=True,
            ExpectedPeaks=expectedPeaks,
            PlotResult=False,
            OutputRawFitParameters=True,
        )

        # Make sure that it's a workspace group
        self.assertTrue(isinstance(output, WorkspaceGroup))

        # check the refined peaks.
        refinedPeaks = AnalysisDataService.retrieve("poldi_data_6904_peaks_refined_2d")
        self.assertEqual(refinedPeaks.rowCount(), 10)

        # check that raw parameters exist
        self.assertTrue(AnalysisDataService.doesExist("poldi_data_6904_raw_fit_parameters"))

        # check that all parameters that have "Mixing" in the name are the same
        rawFitParameters = AnalysisDataService.retrieve("poldi_data_6904_raw_fit_parameters")

        mixingValues = set()
        for i in range(rawFitParameters.rowCount()):
            parameterName = rawFitParameters.cell(i, 0)

            if "Mixing" in parameterName:
                mixingValues.add(rawFitParameters.cell(i, 1))

        self.assertEqual(len(mixingValues), 1)


class POLDIDataAnalysisTestSiPawley(POLDIDataAnalysisTestSi):
    """This test runs PoldiDataAnalysis with Si data, using the PawleyFit-option."""

    def runTest(self):
        data, expectedPeaks = self.prepareTest()

        PoldiDataAnalysis(
            InputWorkspace=data,
            MaximumPeakNumber=11,
            ExpectedPeaks=expectedPeaks,
            PawleyFit=True,
            PlotResult=False,
            OutputWorkspace="output",
        )

        # inspect the cell
        cell = AnalysisDataService.retrieve("poldi_data_6904_cell_refined")

        # 2 rows for cubic cell
        self.assertTrue(cell.rowCount(), 2)

        a_val = cell.cell(0, 1)
        a_err = cell.cell(0, 2)

        self.assertLessThan(np.abs(a_err), 5.0e-5)
        self.assertLessThan(np.abs(a_val - 5.4311946) / a_err, 1.5)


class POLDIDataAnalysisEmptyFile(systemtesting.MantidSystemTest):
    """This test runs PoldiDataAnalysis with Si data, using an empty workspace."""

    def runTest(self):
        empty = PoldiLoadRuns(2015, 977)

        peaks = PoldiCreatePeaksFromCell(
            SpaceGroup="F d -3 m", a=5.431, LatticeSpacingMin=0.7, Atoms="Si 0 0 0 1.0 0.01", OutputWorkspace="Si"
        )
        try:
            PoldiDataAnalysis(
                InputWorkspace=empty.getItem(0), MaximumPeakNumber=11, ExpectedPeaks=peaks, PlotResult=False, OutputWorkspace="output"
            )
        except RuntimeError as error:
            self.assertTrue("Aborting analysis since workspace empty_data_977 does not contain any counts." in str(error))
