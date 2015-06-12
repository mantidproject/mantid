# pylint: disable=no-init,invalid-name,too-many-locals
import stresstesting
from mantid.simpleapi import *
from mantid.api import *
import numpy as np


class POLDIDataAnalysisTestSi(stresstesting.MantidStressTest):
    """Base class that loads/generates data for the tests, which is identical.
    """

    def runTest(self):
        pass

    def prepareTest(self):
        self._loadData()
        self._createSi()

        return 'poldi_data_6904', 'Si'

    def _loadData(self):
        PoldiLoadRuns(2013, 6903, 6904, 2, OutputWorkspace='poldi', MaskBadDetectors=False)

    def _createSi(self):
        PoldiCreatePeaksFromCell(SpaceGroup='F d -3 m',
                                 a=5.431, LatticeSpacingMin=0.7,
                                 Atoms='Si 0 0 0 1.0 0.01',
                                 OutputWorkspace='Si')


class POLDIDataAnalysisTestSiIndividual(POLDIDataAnalysisTestSi):
    """This test runs PoldiDataAnalysis with Si data, using."""

    def runTest(self):
        data, expectedPeaks = self.prepareTest()

        output = PoldiDataAnalysis(InputWorkspace=data,
                                   MaximumPeakNumber=11,
                                   ExpectedPeaks=expectedPeaks,
                                   PlotResult=False)

        # Make sure that it's a workspace group
        self.assertTrue(isinstance(output, WorkspaceGroup))

        # check the refined peaks.
        refinedPeaks = AnalysisDataService.retrieve('poldi_data_6904_peaks_refined_2d')
        self.assertEquals(refinedPeaks.rowCount(), 11)

        # make sure there are no unindexed peaks
        self.assertFalse(AnalysisDataService.doesExist('poldi_data_6904_peaks_refined_1d_unindexed'))

        # get total spectrum
        totalSpectrum = AnalysisDataService.retrieve('poldi_data_6904_sum')
        # get residuals
        residuals = AnalysisDataService.retrieve('poldi_data_6904_residuals')

        sumData = totalSpectrum.dataY(0)
        residualData = residuals.dataY(0)

        maxSum = np.max(sumData)
        maxResidual = np.max(residualData)

        # Maximum residual should not be too large
        self.assertLessThan(maxResidual / maxSum, 0.075)

class POLDIDataAnalysisTestSiIndividualPseudoVoigtTied(POLDIDataAnalysisTestSi):
    """This test runs PoldiDataAnalysis with Si data, using."""

    def runTest(self):
        data, expectedPeaks = self.prepareTest()

        output = PoldiDataAnalysis(InputWorkspace=data,
                                   MaximumPeakNumber=11,
                                   ProfileFunction="PseudoVoigt",
                                   TieProfileParameters=True,
                                   ExpectedPeaks=expectedPeaks,
                                   PlotResult=False,
                                   OutputRawFitParameters=True)

        # Make sure that it's a workspace group
        self.assertTrue(isinstance(output, WorkspaceGroup))

        # check the refined peaks.
        refinedPeaks = AnalysisDataService.retrieve('poldi_data_6904_peaks_refined_2d')
        self.assertEquals(refinedPeaks.rowCount(), 11)

        # check that raw parameters exist
        self.assertTrue(AnalysisDataService.doesExist('poldi_data_6904_raw_fit_parameters'))

        # check that all parameters that have "Mixing" in the name are the same
        rawFitParameters = AnalysisDataService.retrieve('poldi_data_6904_raw_fit_parameters')

        mixingValues = set()
        for i in range(rawFitParameters.rowCount()):
            parameterName = rawFitParameters.cell(i, 0)

            if "Mixing" in parameterName:
                mixingValues.add(rawFitParameters.cell(i, 1))

        self.assertEquals(len(mixingValues), 1)

class POLDIDataAnalysisTestSiPawley(POLDIDataAnalysisTestSi):
    """This test runs PoldiDataAnalysis with Si data, using."""

    def runTest(self):
        data, expectedPeaks = self.prepareTest()

        PoldiDataAnalysis(InputWorkspace=data,
                                   MaximumPeakNumber=11,
                                   ExpectedPeaks=expectedPeaks,
                                   PawleyFit=True,
                                   PlotResult=False, OutputWorkspace='output')

        # inspect the cell
        cell = AnalysisDataService.retrieve('poldi_data_6904_cell_refined')

        # 2 rows for cubic cell
        self.assertTrue(cell.rowCount(), 2)

        a_val = cell.cell(0, 1)
        a_err = cell.cell(0, 2)

        self.assertLessThan(np.abs(a_err), 5.0e-5)
        self.assertLessThan(np.abs(a_val - 5.4311946) / a_err, 1.5)
