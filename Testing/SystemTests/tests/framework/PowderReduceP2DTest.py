# Mantid Repository : https://github.com/mantidproject/mantid
#
# Copyright &copy; 2020 ISIS Rutherford Appleton Laboratory UKRI,
#   NScD Oak Ridge National Laboratory, European Spallation Source,
#   Institut Laue - Langevin & CSNS, Institute of High Energy Physics, CAS
# SPDX - License - Identifier: GPL - 3.0 +
from mantid.simpleapi import PowderReduceP2D
from mantid.api import FileFinder

import numpy as np
import os
import sys
import systemtesting


class PowderReduceP2DTest(systemtesting.MantidSystemTest):
    def __init__(self):
        systemtesting.MantidSystemTest.__init__(self)
        self.tolerance = 1e-6
        self.setUp()

    def setUp(self):
        self.sample = self._sampleEventData()
        self.vana = self._vanadiumEventData()
        self.empty = self._emptyEventData()
        self.calFile = self._calFile()
        self.twoThetaMin = self._twoThetaMin()
        self.twoThetaMax = self._twoThetaMax()
        self.lambdaMin = self._lambdaMin()
        self.lambdaMax = self._lambdaMax()
        self.dMin = self._dMin()
        self.dMax = self._dMax()
        self.dpMin = self._dpMin()
        self.dpMax = self._dpMax()
        self.dSpaceBinning = self._dSpaceBinning()
        self.dPerpendicularBinning = self._dPerpendicularBinning()
        self.FWHM = self._FWHM()
        self.tolerance = self._tolerance()

        self.reference = self._loadReference()
        self.outputFile = self._outputFile()

    def runTest(self):
        PowderReduceP2D(
            SampleData=self.sample,
            OutputFile=self.outputFile,
            DoIntensityCorrection=True,
            VanaData=self.vana,
            DoBackgroundCorrection=True,
            EmptyData=self.empty,
            DoEdgebinning=False,
            CalFile=self.calFile,
            TwoThetaMin=self.twoThetaMin,
            TwoThetaMax=self.twoThetaMax,
            LambdaMin=self.lambdaMin,
            LambdaMax=self.lambdaMax,
            DMin=self.dMin,
            DMax=self.dMax,
            DpMin=self.dpMin,
            DpMax=self.dpMax,
            dSpaceBinning=self.dSpaceBinning,
            dPerpendicularBinning=self.dPerpendicularBinning,
            FWHM=self.FWHM,
            Tolerance=self.tolerance,
            SystemTest=True,
        )

    def doValidation(self):
        """Overrides validation to handle .p2d file with tolerances"""
        measured = f"{self.outputFile}.p2d"
        expected = self.reference

        if not os.path.isabs(measured):
            measured = FileFinder.Instance().getFullPath(measured)
        if not os.path.isabs(expected):
            expected = FileFinder.Instance().getFullPath(expected)

        np_measured = np.loadtxt(measured)
        np_expected = np.loadtxt(expected)

        np.testing.assert_allclose(np_measured, np_expected, atol=0.25, rtol=0.65)

        # testing passed if this is reached
        return True

    def _sampleEventData(self):
        """path to sample event data used for testing the algorithm"""
        return "PG3_4844_event.nxs"

    def _vanadiumEventData(self):
        """path to vanadium event data used for testing the algorithm"""
        return "PG3_4866_event.nxs"

    def _emptyEventData(self):
        """path to empty event data used for testing the algorithm"""
        return "PG3_5226_event.nxs"

    def _calFile(self):
        """path to calibration File used for testing the algorithm"""
        return "PG3_FERNS_d4832_2011_08_24.cal"

    def _twoThetaMin(self):
        """2theta min used for testing the algorithm"""
        return 10

    def _twoThetaMax(self):
        """2theta max used for testing the algorithm"""
        return 150

    def _lambdaMin(self):
        """lambda min used for testing the algorithm"""
        return 0.1

    def _lambdaMax(self):
        """lambda max used for testing the algorithm"""
        return 2

    def _dMin(self):
        """d min used for testing the algorithm"""
        return 0.1

    def _dMax(self):
        """d max used for testing the algorithm"""
        return 2

    def _dpMin(self):
        """dp min used for testing the algorithm"""
        return 0.2

    def _dpMax(self):
        """dp max used for testing the algorithm"""
        return 2

    def _dSpaceBinning(self):
        """d binning used for testing the algorithm"""
        return [0.1 + 0.02, -0.02, 2 - 0.02]

    def _dPerpendicularBinning(self):
        """d perpendicular binning used for testing the algorithm"""
        return [0.4 - 0.1, 0.1, 2 + 0.1]

    def _FWHM(self):
        """FWHM used for testing the algorithm"""
        return 7

    def _tolerance(self):
        """tolerance used for testing the algorithm"""
        return 4

    def _loadReference(self):
        suffix = "" if sys.platform != "win32" else "_msvc"
        return f"PowderReduceP2D_reference{suffix}.p2d"

    def _outputFile(self):
        return "PowderReduceP2D_Test"
